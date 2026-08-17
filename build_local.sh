#!/usr/bin/env bash
# Local Nintendo Switch build. Run this from an MSYS2 UCRT64 shell.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
JOBS="${JOBS:-$(nproc)}"
CLEAN=0

usage() {
  cat <<'EOF'
Usage: ./build_local.sh [-j JOBS] [--clean]

Requires MSYS2 UCRT64, devkitPro and a switchVK SDK. Set SWITCH_NVK_ROOT to
the SDK directory (containing include/vulkan/vulkan.h and lib/libvulkan.a),
or place it under ../switchVK/nvk-switch-*.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--jobs) JOBS="${2:?missing job count}"; shift 2 ;;
    --clean) CLEAN=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
if [[ -z "${SWITCH_NVK_ROOT:-}" ]]; then
  for candidate in "$ROOT/../switchVK"/nvk-switch-* "$ROOT/../switchVK/.ci-build"/nvk-switch-*; do
    [[ -f "$candidate/lib/libvulkan.a" ]] || continue
    SWITCH_NVK_ROOT="$candidate"
    break
  done
fi
if [[ ! -f "${SWITCH_NVK_ROOT:-}/lib/libvulkan.a" || ! -f "${SWITCH_NVK_ROOT:-}/include/vulkan/vulkan.h" ]]; then
  echo "Missing switchVK SDK. Set SWITCH_NVK_ROOT to its SDK directory." >&2
  exit 1
fi

export MESA_NVK_DIR="$SWITCH_NVK_ROOT"
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"
if [[ "$CLEAN" == 1 ]]; then
  exec bash "$ROOT/build_dolphin_standalone_nro.sh" clean
fi
exec bash "$ROOT/build_dolphin_standalone_nro.sh"
