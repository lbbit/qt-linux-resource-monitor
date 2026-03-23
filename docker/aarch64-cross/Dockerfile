FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_MAIN_VERSION=5.15
ENV QT_VERSION=5.15.2
ENV QT_ROOT=/opt/Qt${QT_MAIN_VERSION}/${QT_VERSION}
ENV QT_HOST_DIR=${QT_ROOT}/gcc_64
ENV QT_AARCH64_DIR=${QT_ROOT}/aarch64
ENV AARCH64_SYSROOT=/usr/aarch64-linux-gnu

RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    bc \
    wget \
    curl \
    git \
    zip \
    unzip \
    perl \
    make \
    ninja-build \
    patchelf \
    ca-certificates \
    xz-utils \
    g++ \
    gcc \
    g++-aarch64-linux-gnu \
    gcc-aarch64-linux-gnu \
    libc6-arm64-cross \
    libc6-dev-arm64-cross \
    qtbase5-dev \
    qtbase5-dev-tools \
    qtchooser \
    qt5-qmake \
    libfontconfig1-dev \
    libfreetype6-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxrender-dev \
    libxcb1-dev \
    libxcb-glx0-dev \
    libxcb-keysyms1-dev \
    libxcb-image0-dev \
    libxcb-shm0-dev \
    libxcb-icccm4-dev \
    libxcb-sync-dev \
    libxcb-xfixes0-dev \
    libxcb-shape0-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libxcb-xtest0-dev \
    libxcb-xinerama0-dev \
    libxcb-xkb-dev \
    libglu1-mesa-dev \
    libxcb-util-dev \
    xvfb \
    valgrind \
  && rm -rf /var/lib/apt/lists/*

COPY docker/aarch64-cross/build_qt.sh /usr/local/bin/build_qt_cross.sh
RUN chmod +x /usr/local/bin/build_qt_cross.sh

WORKDIR /workspace
