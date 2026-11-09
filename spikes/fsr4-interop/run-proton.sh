#!/usr/bin/env bash
set -euo pipefail

proton_dir="${PROTON_DIR:-$HOME/.steam/steam/steamapps/common/Proton 10.0}"
prefix="${NEURALFX_PROTON_PREFIX:?Set NEURALFX_PROTON_PREFIX to a scratch compatdata directory}"
mkdir -p "$prefix"
export STEAM_COMPAT_DATA_PATH="$prefix"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$HOME/.steam/steam}"
exec "$proton_dir/proton" run "$@"
