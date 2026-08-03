#!/usr/bin/env bash
set -euo pipefail
: "${DEVKITPRO:?Abre devkitPro MSYS2; falta DEVKITPRO}"
TORIBIO_BUILD_TMP="${TORIBIO_BUILD_TMP:-.codex-build-tmp}"
mkdir -p "$TORIBIO_BUILD_TMP"
TORIBIO_BUILD_TMP="$(cd "$TORIBIO_BUILD_TMP" && pwd)"
export TMP="$TORIBIO_BUILD_TMP"
export TEMP="$TORIBIO_BUILD_TMP"
export TMPDIR="$TORIBIO_BUILD_TMP"
"$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-cmake" --fresh -S . -B build
cmake --build build --parallel
