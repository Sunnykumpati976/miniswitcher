# Roadmap

Each stage leaves the thing runnable; nothing is a branch that has to land
before the demo works again.

## 1. Frame plumbing — done

Frame pool, lock-free SPSC ring, monotonic frame cadence, synthetic sources,
raw UYVY output, PNG previewer.

## 2. Transition engine — done

`Transition` interface, cut / mix / wipe, scripted demo, per-frame timing
report against the frame budget.

## 3. Keyers

Luma key and chroma key, then a downstream keyer carrying an RGBA logo over the
program output. Needs an alpha-aware composite path and a PNG loader on the
input side. The keyer runs after the transition, as it does in real hardware.

## 4. Multiviewer

Tile every input plus program and preview into one output frame, with a red
tally border on whatever is on program and green on preview. Scaling is the new
work here — box-filtered downscale of 4:2:2 without wrecking the chroma.

## 5. Control surface

- Frame-accurate command queue: commands are parsed whenever they arrive and
  applied at the next frame boundary, never mid-composite.
- JSON-over-TCP protocol, plus `switcher-ctl` as a scriptable client.
- A text protocol in the spirit of RossTalk (`CUT`, `AUTO`, `KEY`, `MEM`), so
  existing control gear can drive it. Check the published spec for exact syntax.
- Macro record and playback, serialising the same command stream.
- bash regression harness: drive the CLI, render to file, diff against golden
  frames.

## 6. Web panel

React control panel over WebSocket — PGM/PVW crosspoints, transition select, a
T-bar that drives `position` directly, DSK toggles. Multiviewer streamed to the
browser as MJPEG and drawn on a canvas.

## Stretch

- SDL2 output window for local monitoring.
- `v4l2loopback` sink on Linux, so the switcher shows up as a webcam.
- SIMD compositing (NEON / AVX2) measured against the autovectorised baseline.
- Capture over the SPSC ring from a real source instead of synthetic patterns.
