#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if (( $# > 1 )) || { (( $# == 1 )) && [[ ! $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; }; then
    printf 'Usage: %s [major.minor.patch]\n' "$0" >&2
    exit 2
fi

unset NEURALFX_PACKAGE_VERSION
if (( $# == 1 )); then
    export NEURALFX_PACKAGE_VERSION="$1"
fi

"$script_dir/build.sh" --target neuralfx_package
check_args=()
if (( $# == 1 )); then
    check_args+=("dist/FSR-ets2-ats-$1.zip")
fi
(
    cd "$script_dir/.."
    python3 -B tests/package_check.py "${check_args[@]}" --binary build/windows-x64/dxgi.dll
)
