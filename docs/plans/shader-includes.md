# Shader Include System

## Purpose

Eliminate ~455 lines of duplicated GLSL code across 40+ shader files by implementing a C-side `#include` preprocessor in `shader_utils.cpp` and extracting shared functions into reusable include files under `shaders/common/`.

## Problem

Raylib passes shader source directly to `glShaderSource()` with no preprocessing. The project duplicates identical functions across many shaders — color conversions, noise, edge detection, transforms, constants, and simulation utilities. Some copies have drifted (e.g., inconsistent PI precision, BT.601 vs BT.709 luma weights), creating subtle correctness issues.

## Scope

**Preprocessor:** Extend `SimLoadShaderSource()` to resolve `#include "path"` directives by loading referenced files and concatenating into a single source string before compilation.

**Shared libraries to extract:**

| File | Contents | Consumers |
|------|----------|-----------|
| `common/constants.glsl` | PI, TWO_PI, LUMA_WEIGHTS | 18+ shaders |
| `common/color.glsl` | hsv2rgb, rgb2hsv, luminance | 7 shaders |
| `common/transforms.glsl` | rotate2d, mirror, wrapPosition | 11 shaders |
| `common/noise.glsl` | hash variants, valueNoise, fbm, gnoise | 10 shaders |
| `common/folding.glsl` | pmin, pabs, doPolarFold | 4 shaders |
| `common/edge.glsl` | Sobel 3x3 (vec4 and scalar) | 4 shaders |
| `common/dither.glsl` | Bayer 8x8 matrix | 2 shaders |
| `common/trail.glsl` | deposit + overflow clamp | 4 compute shaders |
| `common/spatial.glsl` | positionToCell | 2 compute shaders |
