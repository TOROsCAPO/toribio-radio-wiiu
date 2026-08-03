#!/usr/bin/env bash
set -euo pipefail
: "${DEVKITPRO:?Abre devkitPro MSYS2; falta DEVKITPRO}"
"$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-cmake" --fresh -S . -B build
cmake --build build --parallel
