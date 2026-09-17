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

It generates CNFs with `../count_fuzzer/cnf-fuzz-brummayer.py`, runs the binary
with randomised options, and checks the decomposition with `fuzz/verify_td.py`
(coverage, edge coverage, connected subtree per vertex, tree shape, reported
width). Failing cases are moved to `fuzz/out/` and the repro commands printed.
Some runs are `skipped` because a random cutoff makes the tool bail before
emitting a decomposition; that is expected, but if nothing gets `checked` the
fuzzer fails. It also fails above `--max-timeouts` (2), since a hang shows up as
a timeout first.

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
`-DSLOW_DEBUG` build, which also cross-checks the `back_capacity`/`UnitFlow::back`
identities against the real `back_arc` lookup.

## Building

```sh
cd build && make -j$(nproc)
```
