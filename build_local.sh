#!/usr/bin/env bash
# Local Nintendo Switch build. Run this from an MSYS2 UCRT64 shell.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
JOBS="${JOBS:-$(nproc)}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build_nx_standalone}"
TMPDIR="${TMPDIR:-$ROOT/.build-tmp}"
PYTHON3="${PYTHON3:-}"
CLEAN=0

if [[ "$(uname -o 2>/dev/null || true)" == "Msys" ]]; then
  export PATH="/usr/bin:/bin:$PATH"
  if ! command -v python3 >/dev/null 2>&1; then
    for python_dir in /ucrt64/bin /mingw64/bin; do
      [[ -x "$python_dir/python3.exe" ]] || continue
      PYTHON3="$python_dir/python3.exe"
      break
    done
  fi
fi
if [[ -z "$PYTHON3" ]]; then
  PYTHON3=$(command -v python3 || true)
fi

usage() {
  cat <<'EOF'
Usage: ./build_local.sh [-j JOBS] [--clean]

Requires MSYS2, devkitPro and a switchVK SDK. Set SWITCH_NVK_ROOT to
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
[[ -n "$PYTHON3" ]] || { echo "Missing Python 3 in MSYS2." >&2; exit 1; }
PYTHON_BIN_DIR="$TMPDIR/python-bin"
mkdir -p "$PYTHON_BIN_DIR"
ln -sf "$PYTHON3" "$PYTHON_BIN_DIR/python3"
# Do not prepend the whole UCRT Python directory: it also contains native
# build tools, which make the MSYS CMake generator emit unusable drive paths.
export PATH="$PYTHON_BIN_DIR:$PATH"
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
# CMake treats a relative static-library path as a -l argument. Canonicalize
# the SDK location so both local invocations and CI can link libvulkan.a.
SWITCH_NVK_ROOT="$(cd -- "$SWITCH_NVK_ROOT" && pwd -P)"

export MESA_NVK_DIR="$SWITCH_NVK_ROOT"
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"
mkdir -p "$TMPDIR"
# devkitA64's Windows-hosted GCC reads TMP/TEMP before TMPDIR.
export TMPDIR
export TMP="$TMPDIR"
export TEMP="$TMPDIR"
if [[ "$CLEAN" == 1 ]]; then
  exec bash "$ROOT/build_dolphin_standalone_nro.sh" clean
fi
exec bash "$ROOT/build_dolphin_standalone_nro.sh"
