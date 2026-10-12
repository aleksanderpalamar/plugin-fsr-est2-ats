#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
package_version=""
if (( $# == 1 )); then
    package_version="${1#v}"
fi
if (( $# > 1 )) || { (( $# == 1 )) && [[ ! $package_version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; }; then
    printf 'Usage: %s [[v]major.minor.patch]\n' "$0" >&2
    exit 2
fi

unset NEURALFX_PACKAGE_VERSION
if (( $# == 1 )); then
    export NEURALFX_PACKAGE_VERSION="$package_version"
fi

"$script_dir/build.sh" --target neuralfx_package
check_args=()
if (( $# == 1 )); then
    check_args+=("dist/FSR-ets2-ats-$package_version.zip")
fi
(
    cd "$script_dir/.."
    python3 -B tests/package_check.py "${check_args[@]}" --binary build/windows-x64/dxgi.dll
)
