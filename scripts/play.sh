#!/usr/bin/env bash
# Play a raw UYVY file. Raw video carries no header, so geometry is an argument.
#   scripts/play.sh demo.yuv 640 360 [fps]
set -euo pipefail

file="${1:?usage: play.sh FILE WIDTH HEIGHT [FPS]}"
width="${2:?width required}"
height="${3:?height required}"
fps="${4:-60}"

if ! command -v ffplay >/dev/null 2>&1; then
    echo "ffplay not found (brew install ffmpeg / apt install ffmpeg)" >&2
    exit 1
fi

exec ffplay -loop 0 -f rawvideo -pixel_format uyvy422 \
    -video_size "${width}x${height}" -framerate "$fps" "$file"
