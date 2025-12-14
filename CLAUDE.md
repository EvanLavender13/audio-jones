# AudioJones

Real-time audio visualizer: WASAPI loopback → FFT beat detection → reactive circular waveforms with bloom/trails.

## Stack

C++20, raylib 5.5, miniaudio, raygui 4.0 (Windows primary, WSL2 dev OK)

## Build

```bash
cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake.exe --build build
./build/AudioJones.exe
```

## Code Style

C++20 with C-style conventions (matches raylib/miniaudio APIs).

**Structures:** Public fields with direct access. In-class defaults for config structs.

**Functions:** Init/Uninit pairs for resources. PascalCase with module prefix (e.g., `FFTProcessorInit`).

**Types:** Explicit types, NULL, raw pointers, fixed arrays, C-style casts, `const` for unmodified values.

**Formatting:** Braces `{}` on all control flow, even single statements.

**Comments:** Only when logic isn't self-evident. Explain "why", never "what". No comments on unchanged code.

**Headers:** `.h`/`.cpp` split. C headers only (`stdbool.h`, `stdint.h`). Isolate STL to `.cpp` files.

**Naming:** PascalCase functions, camelCase locals, UPPER_SNAKE_CASE constants.

**Known deviations:** `preset.cpp` uses STL for JSON serialization.

## Architecture

See [docs/architecture.md](docs/architecture.md) for system diagram, module index, and data flow.
