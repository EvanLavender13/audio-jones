# AudioJones

Real-time audio visualizer. Captures system audio and renders reactive visuals.

## Features

- **System Audio Capture** — WASAPI loopback grabs any audio playing on Windows
- **Beat Detection** — 2048-point FFT with spectral flux analysis drives visual intensity
- **Multi-Layer Drawables** — Up to 16 concurrent elements: waveforms, spectrum bars, and textured shapes
- **GPU Post-Processing** — Reorderable transform effects: kaleidoscope, fractals, stylization, color grading
- **GPU Simulations** — Physarum, Curl Flow, Boids, Cymatics, and more deposit audio-reactive trails
- **Modulation System** — LFOs and audio bands automate effects and drawable properties
- **Preset Save/Load** — JSON presets preserve all settings

## Architecture

See [docs/architecture.md](docs/architecture.md) for system design.

## Effects

See [docs/effects.md](docs/effects.md) for effects list.

## Controls

| Key | Action |
|-----|--------|
| Tab | Toggle UI |

## Demo Videos

[![Demo Videos](https://img.youtube.com/vi/Kk54yCAFdgg/maxresdefault.jpg)](https://youtube.com/playlist?list=PLIx-1pDk0ThFTiljj-aod7HjEa1SnpIvt)

## Stack

C++20, raylib, Dear ImGui, miniaudio, KissFFT

## Requirements

- Windows 10/11
- CMake 3.20+
- C++ compiler (MSYS2 UCRT64 recommended)
- Ninja (optional)

## Quick Start

```bash
git clone git@github.com:EvanLavender13/audio-jones.git
cd audio-jones

# With Ninja
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release

# Or default generator
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

cmake --build build
./build/AudioJones.exe
```
