#!/usr/bin/env bash
# Configure, build and test. Pass --asan for a sanitizer build.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build"
cmake_args=()

if [[ "${1:-}" == "--asan" ]]; then
    build="$root/build-asan"
    cmake_args+=(-DMS_SANITIZE=ON)
fi

cmake -S "$root" -B "$build" "${cmake_args[@]}"
cmake --build "$build" -j "$(getconf _NPROCESSORS_ONLN)"
ctest --test-dir "$build" --output-on-failure

echo "binaries in $build/tools"
