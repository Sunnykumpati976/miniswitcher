#!/usr/bin/env bash
# Build, render the scripted show, and play it back.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
width="${WIDTH:-640}"
height="${HEIGHT:-360}"

"$root/scripts/build.sh"
"$root/build/tools/switch_demo" --width "$width" --height "$height" \
    -o "$root/demo.yuv"
"$root/scripts/play.sh" "$root/demo.yuv" "$width" "$height"
