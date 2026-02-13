# AudioJones

Real-time audio visualizer. Captures system audio and renders reactive visuals.

## Features

- **System Audio Capture** - WASAPI loopback grabs any audio playing on Windows
- **Audio Analysis** - FFT-driven beat detection, per-band energy, and spectral features feed every visual layer
- **Layered Drawables** - Stack waveforms, spectrum bars, and textured shapes into composite scenes
- **Generators** - Procedural content engines: spirals, constellations, plasma, glyphs, nebulae, and more
- **Transforms** - Reorderable effect chain spanning symmetry, warp, cellular, artistic, graphic, retro, optical, motion, and color categories
- **Simulations** - GPU compute agents (slime mold, flocking, curl flow, cymatics, attractors) deposit audio-reactive trails
- **Feedback** - Frame-to-frame blur, flow fields, and edge-following smear build up over time
- **Modulation** - LFOs and audio bands automate any parameter, compounding into vast combinatorial space
- **Presets** - Save and load full configurations as JSON

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
