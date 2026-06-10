#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

build_dir="${BUILD_DIR:-${repo_root}/build}"
build_type="${CMAKE_BUILD_TYPE:-Debug}"

cmake -S "${repo_root}" -B "${build_dir}" -DCMAKE_BUILD_TYPE="${build_type}"
cmake --build "${build_dir}"
ctest --test-dir "${build_dir}" --output-on-failure
