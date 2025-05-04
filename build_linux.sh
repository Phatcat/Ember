# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# DO NOT USE THIS SCRIPT FOR BUILDING LOCALLY! FOLLOW THE GUIDE AND LET VCPKG HANDLE THE DIRTY WORK!

#!/bin/bash
set -e

#####################################################################################################
# Install build tools
# Already pre-installed: (software-properties-common, wget, gcc-14, g++-14, libstdc++-14-dev, git)
# GCC is already the default compiler and up-to-date - libtirpc-dev is for libmysqlconncpp via vcpkg
#####################################################################################################
echo "Updating system and installing apt-get dependencies..."
sudo apt-get update -y
sudo apt-get upgrade -y
sudo apt-get dist-upgrade -y
sudo apt-get install -y build-essential cmake
sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-14 100
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-14 100
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

# Project specific dependencies needed not proivided by vcpkg
sudo apt-get install -y libtirpc-dev
#############################################################
# Install dependencies through vcpkg
# Just grab and bootstrap the vcpkg and the toolchain file will handle the rest
#############################################################
echo "Cloning vcpkg and boot-strapping"
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg integrate install

###############################
# Configure and Build Ember
###############################
echo "=== Configuring project with CMake ==="

BUILD_OPTIONAL_TOOLS=-1
DISABLE_THREADS=0
BUILD_DIR="build"
INSTALL_DIR="./build/bin"
TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake"
BUILD_TYPE="Debug"

cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
  -DBUILD_OPT_TOOLS=${BUILD_OPTIONAL_TOOLS} \
  -DDISABLE_EMBER_THREADS=${DISABLE_THREADS} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}

echo "Building and installing the project..."
cmake --build ${BUILD_DIR} --target install --config ${BUILD_TYPE}

###############################################
# Run the unit_tests for regression control
###############################################
echo "=== Switching to installed directory and running tests ==="
cd ${INSTALL_DIR}
if [ -x "./unit_tests" ]; then
  echo "Running installed test executable..."
  ./unit_tests
else
  echo "Error: Installed test executable not found in ${INSTALL_DIR}. Aborting."
  exit 1
fi

echo "=== Build, install, and test complete ==="

# For caching apt-get packages
sudo rm -rf /var/cache/apt/archives/partial
sudo rm -f /var/cache/apt/archives/lock
