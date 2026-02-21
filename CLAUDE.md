# AudioJones

Real-time audio visualizer with shader effects, GPU simulations, modulation engine, and preset system. WASAPI loopback capture → FFT analysis → reactive visuals.

## Stack

C++20, raylib 5.5, miniaudio, Dear ImGui (docking) via rlImGui (Windows primary, WSL2 dev OK)

## Build

```bash
cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake.exe --build build
./build/AudioJones.exe
```

## Architecture

See [docs/architecture.md](docs/architecture.md) for layers, data flow, key abstractions, and entry points.

## Reference Documents

@docs/structure.md

@docs/conventions.md

@docs/effects.md

@docs/workflow.md
