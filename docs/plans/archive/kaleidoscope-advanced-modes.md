# Kaleidoscope Advanced Modes: KIFS

Add KIFS (Kaleidoscopic IFS) mode to AudioJones for fractal folding symmetry. Modes are mutually exclusive, controlled by a dropdown in the existing Kaleidoscope UI section.

> **Note**: Droste mode was removed during implementation. The log-polar tiling required for infinite spiral zoom inherently creates hard circular boundaries at repeat seams, which conflicted with the organic visual aesthetic goal. Only Polar and KIFS modes remain.

## Current State

The kaleidoscope system uses polar coordinate mirroring with radial twist and fBM warping:

- `shaders/kaleidoscope.fs:67-114` - Fragment shader with polar transform, early bypass when `segments <= 1`
- `src/config/kaleidoscope_config.h:4-14` - 9 parameters with in-class defaults
- `src/render/post_effect.h:35-42` - 8 uniform location members
- `src/render/post_effect.cpp:52-59` - `GetShaderLocation` calls for uniforms
- `src/render/render_pipeline.cpp:104-123` - `SetupKaleido()` uploads uniforms
- `src/render/render_pipeline.cpp:234-238` - Conditional pass execution
- `src/ui/imgui_effects.cpp:33-44` - UI section with sliders
- `src/config/preset.cpp:82-84` - JSON serialization macro

## Phase 1: Config and Enum

**Goal**: Define mode enum and add parameters for Droste and KIFS modes.

**Build**:
- Add `KaleidoscopeMode` enum to `kaleidoscope_config.h` with values `KALEIDO_POLAR`, `KALEIDO_DROSTE`, `KALEIDO_KIFS`
- Add new fields to `KaleidoscopeConfig`:
  - `mode` (KaleidoscopeMode, default KALEIDO_POLAR)
  - `drosteScale` (float, default 0.5) - Log-polar scale factor
  - `drosteSpiral` (float, default 0.0) - Spiral twist rate in radians
  - `kifsIterations` (int, default 4) - Folding iterations 1-8
  - `kifsScale` (float, default 2.0) - Per-iteration scale factor
  - `kifsOffsetX` (float, default 1.0) - X translation after fold
  - `kifsOffsetY` (float, default 1.0) - Y translation after fold
- Update JSON macro in `preset.cpp:82-84` to include new fields

**Done when**: Project builds, `KaleidoscopeConfig` contains mode and 6 new params.

---

## Phase 2: Uniform Plumbing

**Goal**: Wire new uniforms from C++ to shader.

**Build**:
- Add 6 uniform location members to `PostEffect` struct after `kaleidoNoiseScaleLoc`:
  - `kaleidoModeLoc`, `kaleidoDrosteScaleLoc`, `kaleidoDrosteSpiralLoc`
  - `kaleidoKifsIterationsLoc`, `kaleidoKifsScaleLoc`, `kaleidoKifsOffsetLoc`
- Add `GetShaderLocation` calls in `post_effect.cpp` for each new uniform
- Extend `SetupKaleido` in `render_pipeline.cpp:104-123` to upload:
  - `mode` as int
  - `drosteScale`, `drosteSpiral` as floats
  - `kifsIterations` as int, `kifsScale` as float
  - `kifsOffset` as vec2 (from `kifsOffsetX`, `kifsOffsetY`)

**Done when**: Project builds, uniform locations cached and values uploaded each frame.

---

## Phase 3: Shader Implementation

**Goal**: Implement Droste and KIFS transforms in the fragment shader.

**Build**:
- Add 6 new uniforms to `kaleidoscope.fs` after `noiseScale`:
  - `uniform int mode;`
  - `uniform float drosteScale;`, `uniform float drosteSpiral;`
  - `uniform int kifsIterations;`, `uniform float kifsScale;`, `uniform vec2 kifsOffset;`
- Restructure `main()` after bypass check and UV centering:
  - `mode == 0` (Polar): Existing polar transform (lines 79-103)
  - `mode == 1` (Droste): Log-polar spiral transform per `docs/research/kaleidoscope-techniques.md:25-32`
  - `mode == 2` (KIFS): Iterative folding per `docs/research/kaleidoscope-techniques.md:93-103`
- Common supersampled fetch (existing lines 107-113) executes after any mode

**Droste algorithm**:
```glsl
float lnr = log(max(length(uv), 0.0001));
float th = atan(uv.y, uv.x);
float spiralFactor = -log(drosteScale) / TWO_PI;
angle = th + drosteSpiral * lnr + rotation;
radius = exp(mod(lnr - th * spiralFactor + time * 0.1, log(1.0/drosteScale)));
```

**KIFS algorithm** (polar folding variant - replaces original abs/swap approach for better visual results):
```glsl
vec2 p = uv * 2.0;
p = rotate2d(p, rotation);  // Base rotation

vec3 colorAccum = vec3(0.0);
float weightAccum = 0.0;
vec2 pIter = p;

for (int i = 0; i < kifsIterations; i++) {
    // Polar fold: fold angle into segment for radial symmetry
    float r = length(pIter);
    float a = atan(pIter.y, pIter.x);
    float foldAngle = TWO_PI / float(segments);
    a = abs(mod(a + foldAngle * 0.5, foldAngle) - foldAngle * 0.5);
    pIter = vec2(cos(a), sin(a)) * r;

    // IFS contraction and per-iteration rotation
    pIter = pIter * kifsScale - kifsOffset;
    pIter = rotate2d(pIter, float(i) * 0.3 + time * 0.05);

    // Accumulate weighted samples from each iteration depth
    vec2 iterUV = 0.5 + 0.4 * sin((pIter * 0.15 + 0.5) * TWO_PI * 0.5);
    colorAccum += texture(texture0, iterUV).rgb / float(i + 2);
    weightAccum += 1.0 / float(i + 2);
}

// Blend deepest iteration with accumulated samples
vec2 newUV = 0.5 + 0.4 * sin(pIter * 0.3);
// Apply fBM warp if enabled...
finalColor = vec4(mix(texture(texture0, newUV).rgb, colorAccum / weightAccum, 0.5), 1.0);
```

**Done when**: Each mode produces distinct visual output. KIFS creates organic fractal folding patterns with depth accumulation.

---

## Phase 4: UI Integration

**Goal**: Add mode selection dropdown and conditional parameter display.

**Build**:
- Replace Kaleidoscope section body in `imgui_effects.cpp:33-44`:
  - Add mode dropdown using `ImGui::Combo` with labels "Polar", "Droste", "KIFS"
  - Keep universal params visible for all modes: Segments, Spin, Focal Amplitude/Freq, Warp Strength/Speed/Noise Scale
  - Wrap Twist slider in `if (mode == KALEIDO_POLAR)` (radial twist is polar-specific)
  - Add Droste params in `if (mode == KALEIDO_DROSTE)`:
    - "Scale" slider (0.1 - 0.9)
    - "Spiral" slider using `SliderAngleDeg` (-360° to 360°)
  - Add KIFS params in `if (mode == KALEIDO_KIFS)`:
    - "Iterations" slider (1 - 8)
    - "Scale" slider (1.1 - 4.0)
    - "Offset X" and "Offset Y" sliders (0.0 - 2.0)

**Done when**: Mode dropdown switches visible params. Preset save/load preserves mode and all parameters.
