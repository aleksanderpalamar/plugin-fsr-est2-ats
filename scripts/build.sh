#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$project_dir/build/windows-x64"
export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-/tmp/neuralfx-zig-cache}"

configure_args=(-S "$project_dir" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release)
if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
    configure_args+=("-DCMAKE_TOOLCHAIN_FILE=$project_dir/cmake/zig-windows-x64.cmake")
fi

cmake "${configure_args[@]}"
cmake --build "$build_dir" --parallel "${NEURALFX_BUILD_JOBS:-4}" "$@"
if [[ ! -f "$build_dir/dxgi.dll" ]]; then
    printf 'Windows x64 build did not produce dxgi.dll in %s\n' "$build_dir" >&2
    exit 1
fi
