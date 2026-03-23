#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
MODE="${1:-normal}"
DIST_DIR="${2:-$ROOT_DIR/dist-aarch64}"
QT_MAIN_VERSION="${QT_MAIN_VERSION:-5.15}"
QT_VERSION="${QT_VERSION:-5.15.2}"
QT_ROOT="/opt/Qt${QT_MAIN_VERSION}/${QT_VERSION}"
QT_AARCH64_DIR="${QT_ROOT}/aarch64"
AARCH64_LD="${AARCH64_LD:-/usr/aarch64-linux-gnu/lib/ld-linux-aarch64.so.1}"

cd "$ROOT_DIR"
rm -rf build-aarch64 "$DIST_DIR"
mkdir -p build-aarch64 "$DIST_DIR/bin" "$DIST_DIR/runtime" "$DIST_DIR/scripts"

if [[ ! -x "$QT_AARCH64_DIR/bin/qmake" ]]; then
  echo "Missing aarch64 Qt toolchain: $QT_AARCH64_DIR/bin/qmake" >&2
  exit 2
fi

pushd build-aarch64 >/dev/null
if [[ "$MODE" == "asan" ]]; then
  "$QT_AARCH64_DIR/bin/qmake" \
    QMAKE_CXXFLAGS+="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined" \
    QMAKE_LFLAGS+="-fsanitize=address,undefined" \
    ../qt-linux-resource-monitor.pro
else
  "$QT_AARCH64_DIR/bin/qmake" CONFIG+="release" ../qt-linux-resource-monitor.pro
fi
make -j"$(nproc)"
popd >/dev/null

if [[ "$MODE" == "asan" ]]; then
  cp build-aarch64/app/qt_linux_resource_monitor "$DIST_DIR/bin/qt_linux_resource_monitor_asan"
else
  cp build-aarch64/app/qt_linux_resource_monitor "$DIST_DIR/bin/qt_linux_resource_monitor"
fi

cp README.md "$DIST_DIR/"
cp scripts/run_memory_check.sh "$DIST_DIR/scripts/"
cp scripts/run_valgrind_memcheck.sh "$DIST_DIR/scripts/"
chmod +x "$DIST_DIR/scripts/run_memory_check.sh" "$DIST_DIR/scripts/run_valgrind_memcheck.sh"

if [[ "$MODE" == "normal" ]]; then
  if [[ ! -f "$AARCH64_LD" ]]; then
    echo "Missing unstripped aarch64 loader: $AARCH64_LD" >&2
    exit 3
  fi
  cp "$AARCH64_LD" "$DIST_DIR/runtime/ld-linux-aarch64.so.1"
  printf '%s\n' \
    'This package is intended for ARM64 Linux boards and Valgrind-based diagnostics.' \
    'It includes runtime/ld-linux-aarch64.so.1 because many target boards ship a stripped loader.' \
    'For Valgrind symbol quality, prefer this unstripped loader over the stripped system one.' \
    > "$DIST_DIR/AARCH64_RUNTIME_NOTES.txt"
else
  printf '%s\n' \
    'This package contains an ARM64 ASan/UBSan build.' \
    'Run it on an ARM64 target with a compatible sanitizer runtime and Qt runtime libraries.' \
    'Do not use this ASan binary with Valgrind; use the normal ARM64 package for Valgrind-based diagnostics.' \
    > "$DIST_DIR/AARCH64_ASAN_NOTES.txt"
fi
