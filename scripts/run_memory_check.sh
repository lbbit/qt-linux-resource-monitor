#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:abort_on_error=1:strict_string_checks=1:check_initialization_order=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}"

TEST_BIN="${1:-./tests/tst_monitor}"

if command -v xvfb-run >/dev/null 2>&1; then
  xvfb-run -a "$TEST_BIN" -txt
else
  "$TEST_BIN" -txt
fi
