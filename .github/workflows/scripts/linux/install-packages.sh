#!/bin/bash

set -e

# Packages - Build Environment
declare -a BUILD_PACKAGES=(
  "ccache"
  "cmake"
  "ninja-build"
)

# Packages - PCSX2
declare -a PCSX2_PACKAGES=(
  "libaio-dev"
  "libbz2-dev"
  "libegl1-mesa-dev"
  "libgdk-pixbuf2.0-dev"
  "libgl1-mesa-dev"
  "libgtk-3-dev"
  "libharfbuzz-dev"
  "libjpeg-dev"
  "liblzma-dev"
  "libpcap0.8-dev"
  "libpng-dev"
  "libpulse-dev"
  "librsvg2-dev"
  "libsamplerate0-dev"
  "libsoundtouch-dev"
  "libudev-dev"
  "libwayland-dev"
  "libwxgtk3.0-gtk3-dev"
  "libx11-xcb-dev"
  "pkg-config"
  "portaudio19-dev"
  "zlib1g-dev"
)

BUILD_PACKAGES+=("libclang-11-dev")
PCSX2_PACKAGES+=("libstdc++-10-dev")


# - https://github.com/actions/virtual-environments/blob/main/images/linux/Ubuntu2004-README.md
ARCH=""

sudo apt-get -qq update

# Install packages needed for building
echo "Will install the following packages for building - ${BUILD_PACKAGES[*]}"
sudo apt-get -y install "${BUILD_PACKAGES[@]}"
sudo apt install -y '^libxcb.*-dev' '^libxcb-util.*-dev' '^libxkbcommon.*-dev'

# Install packages needed by pcsx2
PCSX2_PACKAGES=("${PCSX2_PACKAGES[@]/%/"${ARCH}"}")
echo "Will install the following packages for pcsx2 - ${PCSX2_PACKAGES[*]}"
sudo apt-get -y install "${PCSX2_PACKAGES[@]}"
