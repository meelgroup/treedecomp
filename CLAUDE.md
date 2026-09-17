# treedecomp

Standalone tree-decomposition tool: reads a DIMACS CNF, builds the primal graph,
runs FlowCutter (`src/flow-cutter-pace17/`, vendored), prints per-variable scores.

## Determinism

The tool must be deterministic: same input and options, same output. Nothing may
branch on wall-clock or CPU time. The FlowCutter loop in
`IFlowCutter::constructTD` is bounded by the `--tditers` counter and the
`--tdsteps` budget, which decreases by a fixed amount per iteration. `cpu_time()`
is only ever printed, never compared against a cutoff. Keep it that way.

## Fuzzing

Run after every major change, 100 iterations at minimum:

```sh
./fuzz/fuzz_td.py --only 100 -t 30
```

`fuzz/fuzz_session.sh [--num N]` runs N of them in a tmux session, forwarding
its other arguments. Any number can share `fuzz/out/`, because every file either
side of the run -- the CNF and the `--tdvis`/`--tdgraphout` paths -- is claimed
with `unique_file()`, which creates it with `O_CREAT|O_EXCL` so the check and
the claim cannot race. Add nothing to that directory by a fixed name.

It generates CNFs with `../count_fuzzer/cnf-fuzz-brummayer.py`, runs the binary
with randomised options, and checks the decomposition with `fuzz/verify_td.py`
(coverage, edge coverage, connected subtree per vertex, tree shape, reported
width). A failing case is left in `fuzz/out/` and its repro commands printed;
everything else is deleted as it goes.
Some runs are `skipped` because a random cutoff makes the tool bail before
emitting a decomposition; that is expected, but if nothing gets `checked` the
fuzzer fails. Timeouts are reported but are not failures: a correct build takes
over 10s on some of the option sets, so there is no threshold that separates slow
from broken. Run it against a build with assertions on -- the default -- and
non-termination trips an assertion instead.

`--tdsepsel` picks the separator selection, and the fuzzer randomises it.
Production only ever uses 0, `node_min_expansion`, but 1 and 3 are the
edge-cutting ones, whose graph gives every arc capacity in *both* directions.
That makes the flow three-valued, which the node graph never does. Keep those in
the fuzz set: a flow representation that only works for the node graph passes
every other test in this repo.

`fuzz/verify_td.py` needs the graph that was actually decomposed, which
`--tdgraphout` writes. Do not reimplement the primal-graph construction and
contraction in the checker.

## Assertions

`RelWithDebInfo` (the default) builds with assertions on, so keep `assert` cheap
enough for that. The id-func accessors in
`src/flow-cutter-pace17/src/{array,tiny,id_multi}*.hpp` are the innermost
FlowCutter loops and use `SLOW_DEBUG_DO(assert(...))` instead: their bounds
checks alone cost ~25% of the runtime. Validate changes to those loops with a
`-DSLOW_DEBUG` build, and run it with `--tdsepsel 3` as well: that is what checks
`UnitFlow`'s residual bookkeeping on a three-valued flow.

## Building

```sh
cd build && make -j$(nproc)
```
