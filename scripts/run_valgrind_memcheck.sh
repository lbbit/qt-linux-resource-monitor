#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
TEST_BIN="${1:-./tests/tst_monitor}"

if ! command -v valgrind >/dev/null 2>&1; then
  echo "valgrind is required" >&2
  exit 1
fi

if command -v xvfb-run >/dev/null 2>&1; then
  xvfb-run -a valgrind \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --error-exitcode=101 \
    "$TEST_BIN" -txt
else
  valgrind \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --error-exitcode=101 \
    "$TEST_BIN" -txt
fi
