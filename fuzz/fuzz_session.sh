#!/bin/bash
#
# Create a tmux session with multiple fuzzing windows running fuzz_td.py
#
# Usage:
#   ./fuzz_session.sh [--num N] [fuzz_td.py options]
#
# Options:
#   --num N    Number of tmux windows to create (default: 24)
#
# Examples:
#   ./fuzz_session.sh                        # 24 windows, default options
#   ./fuzz_session.sh --num 8                # 8 windows
#   ./fuzz_session.sh --num 4 --only 1000    # 4 windows, 1000 tests each
#   ./fuzz_session.sh --exe ../build/treedecomp
#
# All arguments except --num are forwarded to fuzz_td.py in each window. The
# windows share fuzz/out/, which is safe: every file is claimed through
# unique_file(). If the session already exists this attaches to it instead.

set -u

SESSION="fuzzing-treedecomp"
DIR="$(dirname "$(realpath "$0")")"
NUM_WINDOWS=24

if ! command -v tmux >/dev/null 2>&1; then
  echo "Error: tmux is not installed"
  exit 1
fi

if [ "${1-}" = "--num" ]; then
  if [ -z "${2-}" ] || ! [[ "$2" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: --num requires a positive integer argument"
    exit 1
  fi
  NUM_WINDOWS=$2
  shift 2
fi

# quote each argument so paths with spaces survive send-keys
CMD="./fuzz_td.py"
for arg in "$@"; do
  CMD="$CMD $(printf '%q' "$arg")"
done

attach() {
  # switch-client instead of attach when we are already inside tmux
  if [ -n "${TMUX-}" ]; then
    tmux switch-client -t "$SESSION"
  else
    tmux attach -t "$SESSION"
  fi
}

if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "Session '$SESSION' already exists, attaching..."
  attach
  exit 0
fi

echo "Creating tmux session with $NUM_WINDOWS windows..."

tmux new-session -d -s "$SESSION" -c "$DIR" -n "fuzz-1"
tmux send-keys -t "$SESSION:1" "$CMD" Enter

for i in $(seq 2 "$NUM_WINDOWS"); do
  tmux new-window -t "$SESSION" -c "$DIR" -n "fuzz-$i"
  tmux send-keys -t "$SESSION:$i" "$CMD" Enter
done

tmux select-window -t "$SESSION:1"
attach
