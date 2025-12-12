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

**Use:** `.h`/`.cpp` split, Init/Uninit pairs, fixed arrays, raw pointers, NULL, in-class defaults

**Avoid:** auto, nullptr, STL in headers, exceptions, templates, RAII wrappers, smart pointers

**Naming:** raylib PascalCase
