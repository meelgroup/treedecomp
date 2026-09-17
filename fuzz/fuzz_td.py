#!/usr/bin/env python3
# Fuzz treedecomp: generate a CNF with the brummayer fuzzer, run treedecomp with
# random options, and check the decomposition it emits with verify_td.py.
# Usage: fuzz/fuzz_td.py [--only 100] [--seed S] [-t 20] [-v]

import optparse
import os
import random
import shlex
import signal
import stat
import subprocess
import sys

RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
NC = "\033[0m"

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

current_proc = None


def _cleanup_and_exit(_signum, _frame):
    if current_proc is not None and current_proc.poll() is None:
        current_proc.kill()
        current_proc.wait()
    sys.exit(0)


signal.signal(signal.SIGHUP, _cleanup_and_exit)
signal.signal(signal.SIGTERM, _cleanup_and_exit)
signal.signal(signal.SIGINT, _cleanup_and_exit)


def set_up_parser():
    parser = optparse.OptionParser(
        usage="usage: %prog [options]",
        description="Fuzz treedecomp and check that its output is a tree decomposition")
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--seed", dest="rnd_seed", type=int,
                      help="Start seed. Otherwise a random seed is picked")
    parser.add_option("--only", type=int, dest="only", default=100,
                      help="Run N tests. Default: %default")
    parser.add_option("--tout", "-t", dest="maxtime", type=int, default=20,
                      help="Max seconds per treedecomp run. Default: %default")
    parser.add_option("--exe", dest="exe", default=os.path.join(ROOT, "build", "treedecomp"),
                      help="treedecomp binary. Default: %default")
    parser.add_option("--fuzzer", dest="fuzzer",
                      default=os.path.join(ROOT, "..", "count_fuzzer", "cnf-fuzz-brummayer.py"),
                      help="CNF generator. Default: %default")
    parser.add_option("--outdir", dest="outdir", default=os.path.join(HERE, "out"),
                      help="Where generated files live. Default: %default")
    return parser


def unique_file(prefix, suffix, max_num_files=100000):
    """A path no other process will pick, so several fuzzers can share outdir.

    O_CREAT|O_EXCL is the whole point: the check and the claim are one atomic
    step, unlike testing os.path.exists first. Same mechanism as
    ../count_fuzzer/fuzz.py.
    """
    os.makedirs(options.outdir, exist_ok=True)
    counter = 1
    while counter <= max_num_files:
        path = os.path.join(options.outdir, f"{prefix}_{counter}{suffix}")
        try:
            fd = os.open(path, os.O_CREAT | os.O_EXCL, stat.S_IREAD | stat.S_IWRITE)
            os.close(fd)
            return path
        except FileExistsError:
            counter += 1
    print(f"{RED}ERROR: no free filename under {options.outdir}{NC}")
    sys.exit(-1)


def cmd_str(command):
    return " ".join(shlex.quote(str(x)) for x in command)


def run(command, timeout):
    """Returns (output, returncode, timed_out)."""
    global current_proc
    if options.verbose:
        print(f"{CYAN}--> {NC}{cmd_str(command)}")
    proc = subprocess.Popen(command, stderr=subprocess.STDOUT, stdout=subprocess.PIPE,
                            universal_newlines=True)
    current_proc = proc
    try:
        out, _ = proc.communicate(timeout=timeout)
        timed_out = False
    except subprocess.TimeoutExpired:
        proc.kill()
        out, _ = proc.communicate()
        timed_out = True
    current_proc = None
    return out, proc.returncode, timed_out


# Each option gets an "off / small / relatively large" set with weights. The
# graph cutoffs make treedecomp skip the TD entirely, so their restrictive
# values are rare -- otherwise most runs would have nothing to check.
CHOICE_OPTS = [
    # FlowCutter budget. 0 only disables the flowcutter loop; the min-degree
    # heuristic still produces a decomposition
    ("tdsteps",        ["0", "1", "500", "20000"],          [1, 1, 3, 10]),
    ("tditers",        ["0", "1", "5", "40"],               [1, 1, 3, 10]),
    # Graph cutoffs
    ("tdmaxedges",     ["0", "20", "70000"],                [1, 1, 14]),
    ("tdmaxdensity",   ["0.0", "0.01", "0.3", "1.0"],       [1, 1, 2, 12]),
    ("tdmaxedgeratio", ["0", "2", "30", "1000"],            [1, 1, 4, 10]),
    ("tdvarlim",       ["0", "10", "150000"],               [1, 1, 14]),
    # TD selection tunables
    ("tdband",         ["0", "10", "100", "1000"],          [2, 4, 4, 2]),
    ("tddense",        ["0", "1", "30", "100"],             [2, 3, 4, 3]),
    # Separator selection. node_min_expansion (0) is the production path, but the
    # edge ones (1, 3) build a graph whose arcs have capacity in both directions
    # and so exercise a different flow representation
    ("tdsepsel",       ["0", "1", "2", "3"],                [10, 3, 3, 3]),
    ("v",              ["0", "1"],                          [1, 1]),
]

BINARY_OPTS = ["tdcontract", "tdoptindep"]


def gen_treedecomp_opts():
    parts = []
    for flag, choices, weights in CHOICE_OPTS:
        name = ("--" if len(flag) > 1 else "-") + flag
        parts.extend([name, random.choices(choices, weights)[0]])
    for flag in BINARY_OPTS:
        parts.extend(["--" + flag, random.choice(["0", "1"])])
    return parts


def gen_cnf(path):
    """Generate a CNF and give it a projection set some of the time."""
    seed = random.randint(0, 1000 * 1000 * 1000)
    max_vars = random.choice([10, 20, 40, 60])
    cmd = [sys.executable, options.fuzzer,
           "-s", str(seed),
           "-T", str(random.randint(0, 3)),
           "-i", str(random.randint(1, max(1, max_vars // 2))),
           "-I", str(max_vars),
           "-l", str(random.choice([2, 3])),
           "-L", str(random.choice([3, 6, 10])),
           "-p", str(random.choice([1, 5])),
           "-P", str(random.choice([10, 30]))]
    if random.random() < 0.3:
        cmd.append("-t")
    if random.random() < 0.3:
        cmd.append("-m")

    out, rc, timed_out = run(cmd, options.maxtime)
    if rc != 0 or timed_out:
        print(f"{RED}ERROR: the CNF generator failed{NC}")
        print(f"      {cmd_str(cmd)}")
        sys.exit(-1)

    nvars = 0
    for line in out.splitlines():
        if line.startswith("p cnf"):
            nvars = int(line.split()[2])
            break
    if nvars == 0:
        return None, None

    # treedecomp's contraction-by-index assumes the projection set is {1..k}
    extra = ""
    if random.random() < 0.6:
        k = random.randint(1, nvars)
        lits = " ".join(str(i) for i in range(1, k + 1))
        header = random.choice(["c p optshow", "c p show"])
        extra = f"{header} {lits} 0\n"

    with open(path, "w") as f:
        f.write(out if out.endswith("\n") else out + "\n")
        f.write(extra)
    return nvars, cmd_str(cmd)


BAD_OUTPUT = ["Assertion", "assertion", "Sanitizer", "runtime error:",
              "terminate called", "std::bad_alloc", "Segmentation fault"]


def cleanup(paths):
    for path in paths:
        try:
            os.unlink(path)
        except FileNotFoundError:
            pass


def reported_width(out):
    for line in out.splitlines():
        if line.startswith("c TD width:"):
            return int(line.split(":")[1])
    return None


def one_test(seed):
    # Every path treedecomp reads or writes is claimed through unique_file, so
    # any number of fuzzers can run side by side over one outdir.
    cnf_path = unique_file("fuzz", ".cnf")
    dot_path = unique_file("fuzz", ".dot")
    graph_path = unique_file("fuzz", ".graph")
    paths = [cnf_path, dot_path, graph_path]

    nvars, gen_cmd = gen_cnf(cnf_path)
    if nvars is None:
        cleanup(paths)
        return "skipped"

    cmd = ([options.exe] + gen_treedecomp_opts()
           + ["--tdvis", dot_path, "--tdgraphout", graph_path, cnf_path])
    out, rc, timed_out = run(cmd, options.maxtime)

    def die(msg):
        # the files stay behind on failure, which is what makes them reproducible
        print(f"{RED}ERROR: {msg}{NC}")
        print(f"{YELLOW}--> seed {seed}, to re-run by hand:{NC}")
        print(f"      {gen_cmd}")
        print(f"      {cmd_str(cmd)}")
        print(f"{YELLOW}--> kept: {' '.join(paths)}{NC}")
        print(out[-4000:])
        sys.exit(-1)

    if timed_out:
        # Not a failure: the fuzzer judges correctness, not speed, and a correct
        # build genuinely takes over 10s on some of these option sets. A build
        # that fails to terminate trips an assertion long before it would hang.
        print(f"{YELLOW}TIMEOUT after {options.maxtime}s, seed {seed}{NC}")
        timeouts.append(cmd_str(cmd))
        cleanup(paths)
        return "timeout"
    if rc < 0:
        die(f"treedecomp died on signal {-rc}")
    if rc != 0:
        die(f"treedecomp exited with {rc}")
    for bad in BAD_OUTPUT:
        if bad in out:
            die(f"treedecomp output contains '{bad}'")

    # unique_file already created both, so emptiness is what says treedecomp
    # bailed on a cutoff before writing them
    if os.path.getsize(graph_path) == 0 or os.path.getsize(dot_path) == 0:
        cleanup(paths)
        return "skipped"

    vcmd = [sys.executable, os.path.join(HERE, "verify_td.py"), graph_path, dot_path]
    width = reported_width(out)
    if width is not None:
        vcmd += ["--width", str(width)]
    vout, vrc, vtimed_out = run(vcmd, max(options.maxtime, 60))
    if vtimed_out or vrc != 0:
        print(vout)
        die("the decomposition is not a valid tree decomposition")
    if options.verbose:
        print(f"{GREEN}{vout.strip()}{NC}")
    cleanup(paths)
    return "checked"


if __name__ == "__main__":
    parser = set_up_parser()
    (options, args) = parser.parse_args()

    if not os.path.exists(options.exe):
        print(f"ERROR: no treedecomp binary at '{options.exe}', pass --exe")
        sys.exit(-1)
    if not os.path.exists(options.fuzzer):
        print(f"ERROR: no CNF generator at '{options.fuzzer}', pass --fuzzer")
        sys.exit(-1)

    if options.rnd_seed is None:
        rnd_seed = int.from_bytes(os.urandom(8))
        print(f"Using seed: {rnd_seed}")
    else:
        rnd_seed = options.rnd_seed
    random.seed(rnd_seed)

    timeouts = []
    tally = {"checked": 0, "skipped": 0, "timeout": 0}
    for i in range(options.only):
        seed = options.rnd_seed if options.rnd_seed is not None else int.from_bytes(os.urandom(8))
        random.seed(seed)
        tally[one_test(seed)] += 1
        print(f"[{i + 1}/{options.only}] checked {tally['checked']} "
              f"skipped {tally['skipped']} timeout {tally['timeout']}")

    print(f"{GREEN}=== {tally['checked']} decompositions verified, "
          f"{tally['skipped']} skipped (cutoffs), {tally['timeout']} timed out ==={NC}")
    if tally["checked"] == 0:
        print(f"{RED}ERROR: not a single decomposition got checked{NC}")
        sys.exit(-1)
    if timeouts:
        print(f"{YELLOW}{len(timeouts)} run(s) timed out; not a failure, but worth a look:{NC}")
        for cmd in timeouts:
            print(f"      {cmd}")
