# Signal Frames — Shape Morphing

Replace the fixed box/triangle alternation in Signal Frames with a general polygon SDF (`sdNgon`) supporting fractional side counts. Layers display a gradient of polygon counts across the stack, and the sweep ratchets each layer's side count forward. Four new parameters control base shape, spread, cycle range, and transition smoothness.

**Research**: `docs/research/signal-frames-morph.md`

## Design

### Types

**SignalFramesConfig** — add 4 fields (after `aspectRatio`):

```
float baseSides = 3.0f;    // Starting polygon count for innermost layer (3.0-8.0)
float sideSpread = 3.0f;   // Side count change across layer stack (inner→outer) (-5.0-5.0)
float morphRange = 4.0f;   // Shapes before sweep ratchet cycles (1.0-6.0)
float morphSmooth = 0.5f;  // Transition smoothness: 0=snap, 1=continuous (0.0-1.0)
```

**SignalFramesEffect** — add 4 uniform location fields:

```
int baseSidesLoc;
int sideSpreadLoc;
int morphRangeLoc;
int morphSmoothLoc;
```

**SIGNAL_FRAMES_CONFIG_FIELDS** — append: `baseSides, sideSpread, morphRange, morphSmooth`

### Algorithm

The shader replaces `sdBox()` and `sdEquilateralTriangle()` with a single `sdNgon()` function. Both old SDF functions are removed entirely.

#### sdNgon — polar-fold regular polygon SDF

```glsl
float sdNgon(vec2 p, float r, float n) {
    float a = atan(p.y, p.x);
    float sector = TAU / n;
    a = mod(a + sector * 0.5, sector) - sector * 0.5;
    return cos(a) * length(p) - r;
}
```

#### Per-layer side count computation

Inside the layer loop, after computing `size` and before the SDF call:

```glsl
// Layer gradient: inner layers fewer sides, outer layers more (or reversed)
float gradientSides = baseSides + sideSpread * t;

// Sweep ratchet: each sweep pass advances by 1, cycles within morphRange
float sweepAdvance = mod(floor(sweepAccum + t), morphRange);

// Combined raw sides
float rawSides = gradientSides + sweepAdvance;

// Configurable smoothness: 0=discrete snap, 1=continuous morph
float sides = mix(floor(rawSides), rawSides, morphSmooth);

// Clamp minimum to 3 (triangle is lowest valid polygon)
sides = max(sides, 3.0);
```

#### Shape SDF replacement

Replace the `if (i % 2 == 0)` branch with:

```glsl
vec2 sdfUV = vec2(uv.x / aspectRatio, uv.y);
float sdf = sdNgon(sdfUV, size, sides);
```

This replaces `sdBox(uv, vec2(size * aspectRatio, size))` and `sdEquilateralTriangle(uv, size)`. The aspect ratio is applied as a UV pre-scale so all polygons stretch uniformly.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseSides | float | 3.0-8.0 | 3.0 | Yes | `"Base Sides"` |
| sideSpread | float | -5.0-5.0 | 3.0 | Yes | `"Side Spread"` |
| morphRange | float | 1.0-6.0 | 4.0 | Yes | `"Morph Range"` |
| morphSmooth | float | 0.0-1.0 | 0.5 | Yes | `"Morph Smooth"` |

**UI placement**: In the "Geometry" section, after `Aspect Ratio` slider.

**Slider details**:
- `baseSides`: `ModulatableSlider`, `"%.1f"` format
- `sideSpread`: `ModulatableSlider`, `"%.1f"` format
- `morphRange`: `ModulatableSlider`, `"%.1f"` format
- `morphSmooth`: `ModulatableSlider`, `"%.2f"` format

### Shader Uniforms

Add 4 new uniforms to `signal_frames.fs`:

```glsl
uniform float baseSides;
uniform float sideSpread;
uniform float morphRange;
uniform float morphSmooth;
```

### Removals

- `sdBox()` function — removed from shader
- `sdEquilateralTriangle()` function — removed from shader
- `if (i % 2 == 0)` shape branch — replaced by `sdNgon()` call

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Add morph params to SignalFramesConfig

**Files**: `src/effects/signal_frames.h`
**Creates**: New config fields and uniform locations that .cpp and .fs depend on

**Do**:
1. Add 4 new fields to `SignalFramesConfig` after `aspectRatio`: `baseSides`, `sideSpread`, `morphRange`, `morphSmooth` with types, defaults, and range comments per Design section
2. Append the 4 field names to `SIGNAL_FRAMES_CONFIG_FIELDS` macro
3. Add 4 uniform location fields to `SignalFramesEffect`: `baseSidesLoc`, `sideSpreadLoc`, `morphRangeLoc`, `morphSmoothLoc`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update shader with sdNgon and morph logic

**Files**: `shaders/signal_frames.fs`
**Depends on**: Wave 1 complete (needs to know uniform names)

**Do**:
1. Remove `sdBox()` function
2. Remove `sdEquilateralTriangle()` function
3. Add `sdNgon()` function per Algorithm section
4. Add 4 new uniform declarations: `baseSides`, `sideSpread`, `morphRange`, `morphSmooth`
5. Inside the layer loop, after `float size = mix(sizeMin, sizeMax, t);`, add the side count computation per Algorithm section (uses existing `sweepAccum` and `t`)
6. Replace the `if (i % 2 == 0)` shape branch with the `sdfUV`/`sdNgon` call per Algorithm section
7. Keep all other shader code (FFT sampling, glow, sweep, gradient, tonemap) unchanged

**Verify**: Shader file has no `sdBox`, no `sdEquilateralTriangle`, no `if (i % 2 == 0)`.

#### Task 2.2: Update C++ — uniform binding, param registration, UI

**Files**: `src/effects/signal_frames.cpp`
**Depends on**: Wave 1 complete (needs header fields)

**Do**:
1. In `SignalFramesEffectInit()`: add 4 `GetShaderLocation` calls for `baseSides`, `sideSpread`, `morphRange`, `morphSmooth`
2. In `SignalFramesEffectSetup()`: add 4 `SetShaderValue` calls binding the config fields to the new uniform locations
3. In `SignalFramesRegisterParams()`: register all 4 new params with ranges matching the Design section:
   - `"signalFrames.baseSides"`, 3.0-8.0
   - `"signalFrames.sideSpread"`, -5.0-5.0
   - `"signalFrames.morphRange"`, 1.0-6.0
   - `"signalFrames.morphSmooth"`, 0.0-1.0
4. In `DrawSignalFramesParams()`, in the Geometry section after the `Aspect Ratio` slider: add 4 `ModulatableSlider` calls per Design section (Base Sides, Side Spread, Morph Range, Morph Smooth)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Shader has `sdNgon` function, no `sdBox` or `sdEquilateralTriangle`
- [ ] All 4 new uniforms declared and bound
- [ ] All 4 new config fields serialized via CONFIG_FIELDS macro
- [ ] All 4 new params registered for modulation
- [ ] UI shows 4 new sliders in Geometry section
- [ ] `morphSmooth = 0`, `morphRange = 2`, `baseSides = 3` recreates approximate triangle/square alternation
- [ ] `sideSpread = 0` makes all layers the same shape
