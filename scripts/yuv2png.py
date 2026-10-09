#!/usr/bin/env python3
"""Extract one frame of raw UYVY 4:2:2 as a PNG.

Stdlib only -- no ffmpeg, no Pillow -- so a reviewer can eyeball output on any
machine that has Python.

    scripts/yuv2png.py demo.yuv 640 360 --frame 60 -o frame.png
"""
import argparse
import struct
import sys
import zlib

# BT.709 limited range, inverse of the conversion in libframe/src/pattern.c.
KR, KG, KB = 0.2126, 0.7152, 0.0722


def clamp8(value: float) -> int:
    return 0 if value < 0 else 255 if value > 255 else int(value + 0.5)


def uyvy_to_rgb_rows(raw: bytes, width: int, height: int) -> list[bytearray]:
    rows = []
    stride = width * 2
    for y in range(height):
        line = raw[y * stride:(y + 1) * stride]
        row = bytearray()
        for pair in range(width // 2):
            cb, y0, cr, y1 = line[pair * 4:pair * 4 + 4]
            cb_n = (cb - 128) * 255.0 / 224.0
            cr_n = (cr - 128) * 255.0 / 224.0
            for luma in (y0, y1):
                y_n = (luma - 16) * 255.0 / 219.0
                r = y_n + 1.5748 * cr_n
                b = y_n + 1.8556 * cb_n
                g = y_n - (KR * 1.5748 * cr_n + KB * 1.8556 * cb_n) / KG
                row += bytes((clamp8(r), clamp8(g), clamp8(b)))
        rows.append(row)
    return rows


def write_png(path: str, rows: list[bytearray], width: int, height: int) -> None:
    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    # Filter type 0 (none) in front of every scanline.
    scanlines = b"".join(b"\x00" + bytes(row) for row in rows)
    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)

    with open(path, "wb") as out:
        out.write(b"\x89PNG\r\n\x1a\n")
        out.write(chunk(b"IHDR", header))
        out.write(chunk(b"IDAT", zlib.compress(scanlines, 9)))
        out.write(chunk(b"IEND", b""))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input")
    parser.add_argument("width", type=int)
    parser.add_argument("height", type=int)
    parser.add_argument("--frame", type=int, default=0, help="0-based frame index")
    parser.add_argument("-o", "--out", default="frame.png")
    args = parser.parse_args()

    if args.width % 2 != 0:
        print("width must be even for 4:2:2", file=sys.stderr)
        return 2

    frame_bytes = args.width * args.height * 2
    with open(args.input, "rb") as src:
        src.seek(args.frame * frame_bytes)
        raw = src.read(frame_bytes)

    if len(raw) < frame_bytes:
        print(f"frame {args.frame} is past the end of {args.input}", file=sys.stderr)
        return 1

    write_png(args.out, uyvy_to_rgb_rows(raw, args.width, args.height),
              args.width, args.height)
    print(f"frame {args.frame} -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
