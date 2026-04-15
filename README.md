# Tree Decomposition library

This is a library that's useful for doing tree decompositions. I have been
using these tools for my tool [Ganak](https://github.com/meelgroup/ganak/), but
I want to use it in more projects. Now it's a library.

## Authors

* Ben Strasser -- original FlowCutter library
* Tuukka Korhonen and Matti Jarvisalo -- graph libraries
* Kenji Hashimoto -- graph libraries

## Standalone `treedecomp` binary

Alongside the library, `build/treedecomp` is a CLI that reads a CNF on stdin
(or a file), builds the primal graph (one edge between every pair of variables
that share a clause), runs FlowCutter, and prints a per-variable tree
decomposition score — the same score ganak uses internally to bias branching.

### Input

Standard DIMACS CNF:

```
p cnf <nvars> <nclauses>
2 30 0
-1 4 5 0
...
```

### Output

Comment lines prefixed with `c` (parse stats and the TD width), followed by
one line per variable on stdout in the form:

```
<var> <tdscore>
```

The **first column is the variable** (1-indexed, matching the CNF numbering).
The **second column is its tree-decomposition score, in the range `0..100`** —
higher means the variable is closer to the centroid of the tree
decomposition (this is the `compute_td_score_using_raw` path from ganak's
`counter.cpp`).

### Example

A small path-like CNF:

```
$ cat example.cnf
p cnf 5 4
1 2 0
2 3 0
3 4 0
4 5 0

$ ./build/treedecomp example.cnf
c parsed nvars=5 clauses=4
c primal nodes=5 edges=4 density=0.16 edge/var=0.8
c TD width: 1
1 0
2 50
3 100
4 100
5 50
```

Each output line is `<variable> <score>`: column 1 is the CNF variable id,
column 2 is its TD score in `0..100`. Here variables 3 and 4 sit at the
centroid of the decomposition (score 100); the leaves 1 and 5 are farthest
(score 0 and 50).

### Usage

```
./treedecomp input.cnf              # read from file
./treedecomp < input.cnf            # read from stdin
./treedecomp --tditers 300 input.cnf
```

### Options

Cutoffs and tuning knobs mirror ganak's `conf.td_*` settings so results line
up with what ganak computes on the same instance:

| flag | meaning |
| --- | --- |
| `--tdsteps N`        | FlowCutter max steps (default 100000) |
| `--tditers N`        | FlowCutter iterations / restarts (default 900) |
| `--tdmaxedges N`     | skip TD if primal has more than N edges |
| `--tdmaxdensity F`   | skip TD if primal density > F |
| `--tdmaxedgeratio N` | skip TD if edge/var ratio > N |
| `--tdvarlim N`       | skip TD if #vars > N |
| `--tdcontract 0/1`   | contract high-numbered vars before running TD |
| `-v N`               | verbosity |

The binary links against the `treedecomp` library and uses `argparse.hpp` for
option parsing.

# Notes

Basically, if you find a bug, it's likely been introduced by me. Just open an
issue and I'll fix.
