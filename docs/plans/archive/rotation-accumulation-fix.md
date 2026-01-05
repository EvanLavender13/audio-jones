# Rotation Accumulation Fix

## Problem

Shaders compute rotation as `time * rotationSpeed`, causing instant position jumps when speed changes. Changing speed from 0.1 to 0.2 at t=10s jumps offset from 1.0 to 2.0.

## Solution

Accumulate rotation on CPU: `rotation += deltaTime * rotationSpeed`. Pass accumulated value to shader, not speed. Speed changes only affect future increments.

## Affected Effects

Audit each for `time * speed` or `time * rotationSpeed` patterns:

- `tunnel.fs` - `time * rotationSpeed` in texX calculation
- `mobius.fs` - `time * rotationSpeed` in pole angle
- `turbulence.fs` - `time` in rotation angle calculation
- `infinite_zoom.fs` - check spiral rotation
- `radial_streak.fs` - check spiral twist
- `voronoi.fs` - `time * speed` in cell animation
- `multi_inversion.fs` - check time usage

## Pattern

**Before (broken):**
```glsl
// Shader
float angle = time * rotationSpeed;
```

**After (correct):**
```cpp
// CPU (render_pipeline.cpp)
pe->effectRotation += deltaTime * config.rotationSpeed;
```
```glsl
// Shader
uniform float rotation;  // accumulated value
float angle = rotation;
```

## Reference

Kaleidoscope already uses correct pattern:
- `render_pipeline.cpp:516`: `pe->currentKaleidoRotation += pe->effects.kaleidoscope.rotationSpeed;`
- Shader receives accumulated rotation, not speed
