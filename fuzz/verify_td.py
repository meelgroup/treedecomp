#!/usr/bin/env python3
# Check that treedecomp's --tdvis output is a tree decomposition of its
# --tdgraphout graph.
# Usage: verify_td.py <graph-file> <dot-file> [--width W]

import argparse
import re
import sys
from collections import deque


def read_graph(path):
    n = None
    edges = set()
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("p "):
                parts = line.split()
                n = int(parts[2])
            elif line.startswith("e "):
                _, a, b = line.split()
                a, b = int(a), int(b)
                edges.add((min(a, b), max(a, b)))
    if n is None:
        raise ValueError(f"{path}: no 'p tdgraph' line")
    return n, edges


BAG_RE = re.compile(r'^\s*b(\d+)\s*\[label="bag \d+(?: \(centroid\))?\\nsize (\d+)\\n\{([^}]*)\}"')
EDGE_RE = re.compile(r'^\s*b(\d+)\s*--\s*b(\d+)\s*;')


def read_dot(path):
    bags = {}
    tree = {}
    with open(path) as f:
        for line in f:
            m = BAG_RE.match(line)
            if m:
                idx, size, body = int(m.group(1)), int(m.group(2)), m.group(3).strip()
                verts = [int(x) for x in body.split(",")] if body else []
                if len(verts) != size:
                    raise ValueError(f"bag {idx}: label says size {size} but lists {len(verts)}")
                bags[idx] = verts
                tree.setdefault(idx, [])
                continue
            m = EDGE_RE.match(line)
            if m:
                a, b = int(m.group(1)), int(m.group(2))
                tree.setdefault(a, []).append(b)
                tree.setdefault(b, []).append(a)
    if set(bags) != set(range(len(bags))):
        raise ValueError("bag ids are not 0..nBags-1")
    if not set(tree) <= set(bags):
        raise ValueError("a tree edge refers to a bag that was never declared")
    return bags, tree


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def reachable(tree, start, allowed):
    seen = {start}
    q = deque([start])
    while q:
        x = q.popleft()
        for y in tree.get(x, ()):
            if y in allowed and y not in seen:
                seen.add(y)
                q.append(y)
    return seen


def check(n, edges, bags, tree):
    nbags = len(bags)
    if nbags == 0:
        fail("the decomposition has no bags")

    for i, verts in bags.items():
        if len(set(verts)) != len(verts):
            fail(f"bag {i} has a repeated vertex: {verts}")
        for v in verts:
            if not 1 <= v <= n:
                fail(f"bag {i} contains vertex {v}, outside 1..{n}")

    # a tree: n-1 edges and connected
    num_edges = sum(len(v) for v in tree.values()) // 2
    if num_edges != nbags - 1:
        fail(f"{nbags} bags but {num_edges} tree edges, expected {nbags - 1}")
    if len(reachable(tree, 0, set(bags))) != nbags:
        fail("the bag graph is not connected")

    # every vertex is in some bag
    bags_of = {v: [] for v in range(1, n + 1)}
    for i, verts in bags.items():
        for v in verts:
            bags_of[v].append(i)
    missing = [v for v, bs in bags_of.items() if not bs]
    if missing:
        fail(f"{len(missing)} vertices are in no bag, e.g. {missing[:10]}")

    # every edge is inside some bag
    for (a, b) in edges:
        if not (set(bags_of[a]) & set(bags_of[b])):
            fail(f"edge ({a},{b}) is not covered by any bag")

    # the bags holding a vertex form a connected subtree
    for v, bs in bags_of.items():
        allowed = set(bs)
        if len(reachable(tree, bs[0], allowed)) != len(allowed):
            fail(f"the {len(allowed)} bags containing vertex {v} are not connected")


def main():
    ap = argparse.ArgumentParser(description=
        "Check that a --tdvis DOT file is a tree decomposition of a --tdgraphout graph")
    ap.add_argument("graph")
    ap.add_argument("dot")
    ap.add_argument("--width", type=int, default=None,
                    help="Width treedecomp reported, cross-checked against the bags")
    args = ap.parse_args()

    n, edges = read_graph(args.graph)
    bags, tree = read_dot(args.dot)
    check(n, edges, bags, tree)
    width = max(len(b) for b in bags.values()) - 1
    if args.width is not None and args.width != width:
        fail(f"reported width {args.width} but the widest bag gives {width}")
    print(f"OK: valid tree decomposition, {n} vertices, {len(edges)} edges, "
          f"{len(bags)} bags, width {width}")


if __name__ == "__main__":
    main()
