#!/usr/bin/env bash
set -euo pipefail
: "${DEVKITPRO:?Abre devkitPro MSYS2; falta DEVKITPRO}"
WUHBTOOL="${WUHBTOOL:-$DEVKITPRO/tools/bin/wuhbtool}"
mkdir -p dist
package() {
  local rpx="$1" out="$2" name="$3" icon="$4"
  "$WUHBTOOL" "$rpx" "$out" --name="$name" --short-name="$name" --author="Toribio Tecnologic" --icon="$icon"
}
"$WUHBTOOL" build/wiiu-radio.rpx dist/toribio-radio.wuhb --name="Toribio Radio para Wii U" --short-name="Toribio Radio" --author="Toribio Tecnologic" --icon=apps/radio/assets/icon.png --tv-image=apps/radio/assets/radio_bg_tv.jpg --drc-image=apps/radio/assets/radio_bg_drc.jpg
package build/tapo-wiiu-viewer.rpx dist/tapo-wiiu-viewer.wuhb "Tapo Wii U Viewer" apps/tapo-viewer/assets/icon.png
package build/toribio-browser.rpx dist/toribio-browser.wuhb "Toribio Browser" apps/toribio-browser/assets/icon.png
