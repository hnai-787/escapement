#!/usr/bin/env bash
# Configures and builds cpu16 with CMake + MinGW g++, reusing the vcpkg
# instance at C:/vcpkg (Catch2 only -- this project has no JSON output,
# so no nlohmann-json dependency) set up for the sibling C++ projects.
set -euo pipefail

cd "$(dirname "$0")"

VCPKG_ROOT="${VCPKG_ROOT:-C:/vcpkg}"

cmake -S . -B build \
    -G "MinGW Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
    -DVCPKG_HOST_TRIPLET=x64-mingw-static \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel

echo ""
echo "Built: build/cpu16.exe"
echo "Test binary: build/cpu16_tests.exe"
