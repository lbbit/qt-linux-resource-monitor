#!/usr/bin/env bash
set -euo pipefail

QT_MAIN_VERSION="${QT_MAIN_VERSION:-5.15}"
QT_VERSION="${QT_VERSION:-5.15.2}"
QT_ROOT="/opt/Qt${QT_MAIN_VERSION}/${QT_VERSION}"
QT_HOST_DIR="${QT_ROOT}/gcc_64"
QT_AARCH64_DIR="${QT_ROOT}/aarch64"
QT_SRC_PARENT="/opt/src"
QT_SRC_DIR="${QT_SRC_PARENT}/qt-everywhere-src-${QT_VERSION}"
QT_ARCHIVE="qt-everywhere-src-${QT_VERSION}.tar.xz"
QT_URL="https://download.qt.io/archive/qt/${QT_MAIN_VERSION}/${QT_VERSION}/single/${QT_ARCHIVE}"

mkdir -p "${QT_ROOT}" "${QT_SRC_PARENT}"

if [[ -x "${QT_AARCH64_DIR}/bin/qmake" && -x "${QT_HOST_DIR}/bin/qmake" ]]; then
  echo "Qt cross toolchain already exists at ${QT_ROOT}"
  exit 0
fi

cd "${QT_SRC_PARENT}"
if [[ ! -f "${QT_ARCHIVE}" ]]; then
  wget -O "${QT_ARCHIVE}" "${QT_URL}"
fi
if [[ ! -d "${QT_SRC_DIR}" ]]; then
  tar -xf "${QT_ARCHIVE}"
fi

cd "${QT_SRC_DIR}"

if [[ ! -x "${QT_HOST_DIR}/bin/qmake" ]]; then
  mkdir -p build-host && cd build-host
  ../configure -prefix "${QT_HOST_DIR}" -opensource -confirm-license -release -nomake examples -nomake tests
  make -j"$(nproc)"
  make install
  cd ..
fi

if [[ ! -x "${QT_AARCH64_DIR}/bin/qmake" ]]; then
  mkdir -p build-aarch64 && cd build-aarch64
  ../configure \
    -prefix "${QT_AARCH64_DIR}" \
    -opensource -confirm-license -release \
    -nomake examples -nomake tests \
    -xplatform linux-aarch64-gnu-g++ \
    -device-option CROSS_COMPILE=aarch64-linux-gnu- \
    -sysroot /usr/aarch64-linux-gnu \
    -qt-host-path "${QT_HOST_DIR}"
  make -j"$(nproc)"
  make install
  cd ..
fi
