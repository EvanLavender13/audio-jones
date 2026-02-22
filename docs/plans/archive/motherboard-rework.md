# Motherboard Rework

Replace Motherboard's box-fold iteration with Kali hyperbolic inversion (`abs(p) / clamp(p.x*p.y, lo, hi) - c`) to create neon circuit-board traces with branching pathways. Switch rendering from `smoothstep * 1/dist` to `exp(-k * dist)` for sharper neon glow. Add data streaming animation via `fract()` modulated orbit traps and infinite tiling via `fract()` coordinate wrapping. This is a full internal rewrite; the module name, enum entry, and integration points all stay the same.

**Research**: `docs/research/motherboard-rework.md`

## Design

### Types

**MotherboardConfig** (replaces all existing fields except standard ones):

```c
struct MotherboardConfig {
  bool enabled = false;

  // Geometry
  int iterations = 12;         // Fold depth; each iteration = one frequency band (4-16)
  float zoom = 2.0f;           // Scale factor before tiling (0.5-4.0)
  float clampLo = 0.15f;       // Inversion lower bound (0.01-1.0)
  float clampHi = 2.0f;        // Inversion upper bound (0.5-5.0)
  float foldConstant = 1.0f;   // Post-inversion translation (0.5-2.0)
  float rotAngle = 0.0f;       // Per-iteration fold rotation, radians (-PI..PI)

  // Animation
  float panSpeed = 0.3f;       // Drift speed through fractal space (-2.0..2.0)
  float flowSpeed = 0.3f;      // Data streaming speed (0.0-2.0)
  float flowIntensity = 0.3f;  // Streaming visibility (0.0-1.0)
  float rotationSpeed = 0.0f;  // Pattern rotation rate, radians/second

  // Rendering
  float glowIntensity = 0.033f;   // Trace glow width: exp sharpness = 1/glowIntensity (0.001-0.1)
  float accentIntensity = 0.033f; // Junction glow width: exp sharpness = 1/accentIntensity (0.0-0.1)

  // Audio
  float baseFreq = 55.0f;      // Lowest frequency band Hz (27.5-440.0)
  float maxFreq = 14000.0f;    // Highest frequency band Hz (1000-16000)
  float gain = 2.0f;           // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;          // Contrast exponent (0.1-3.0)
  float baseBright = 0.15f;    // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

**MotherboardEffect** (new accumulators, new uniform locations):

```c
typedef struct MotherboardEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float panAccum;        // CPU-accumulated pan offset
  float flowAccum;       // CPU-accumulated flow phase
  float rotationAccum;   // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc, maxFreqLoc, gainLoc, curveLoc, baseBrightLoc;
  int iterationsLoc, zoomLoc, clampLoLoc, clampHiLoc, foldConstantLoc, rotAngleLoc;
  int panAccumLoc, flowAccumLoc, flowIntensityLoc, rotationAccumLoc;
  int glowIntensityLoc, accentIntensityLoc;
  int gradientLUTLoc;
} MotherboardEffect;
```

**CONFIG_FIELDS macro:**

```c
#define MOTHERBOARD_CONFIG_FIELDS                                              \
  enabled, iterations, zoom, clampLo, clampHi, foldConstant, rotAngle,        \
      panSpeed, flowSpeed, flowIntensity, rotationSpeed, glowIntensity,       \
      accentIntensity, baseFreq, maxFreq, gain, curve, baseBright, gradient,  \
      blendMode, blendIntensity
```

### Algorithm

The shader replaces the entire `main()` function.

**Coordinate setup** (flat overhead, no perspective):

```glsl
// Center + aspect correct
vec2 p = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

// Rotate by accumulated angle
float cr = cos(rotationAccum), sr = sin(rotationAccum);
p *= mat2(cr, -sr, sr, cr);

// Pan (Y-axis drift; rotation above controls effective direction)
p.y += panAccum;

// Scale + infinite tile
p *= zoom;
p = fract(p) - 0.5;
```

**Kali inversion iteration with dual orbit traps:**

```glsl
float cf = cos(rotAngle), sf = sin(rotAngle);
mat2 foldRot = mat2(cf, -sf, sf, cf);

float ot1 = 1000.0, ot2 = 1000.0;
int minit = 0;

for (int i = 0; i < iterations; i++) {
    p = abs(p);
    p = p / clamp(abs(p.x * p.y), clampLo, clampHi) - foldConstant;
    p *= foldRot;

    // Primary trap: trace distance (determines winning layer)
    float m = abs(p.x);
    // Data streaming modulation (Shader B technique)
    m += fract(p.x + flowAccum + float(i) * 0.2) * flowIntensity;
    if (m < ot1) {
        ot1 = m;
        minit = i;
    }

    // Secondary trap: junction distance
    ot2 = min(ot2, length(p));
}
```

**Rendering via exp() glow:**

```glsl
// Exp sharpness controlled by glowIntensity/accentIntensity
// Smaller value = sharper/thinner traces, larger = broader glow
float traceSharp = 1.0 / max(glowIntensity, 0.001);
float trace = exp(-traceSharp * ot1);

float junction = 0.0;
if (accentIntensity > 0.0) {
    float accentSharp = 1.0 / accentIntensity;
    junction = exp(-accentSharp * ot2);
}
```

**FFT mapping (winning iteration -> frequency band):**

```glsl
float t0 = float(minit) / float(iterations);
float t1 = float(minit + 1) / float(iterations);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float energy = 0.0;
const int BAND_SAMPLES = 4;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
}
energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + energy;
```

**Final color composite:**

```glsl
vec3 layerColor = texture(gradientLUT, vec2((float(minit) + 0.5) / float(iterations), 0.5)).rgb;
vec3 color = trace * layerColor * brightness + junction;
finalColor = vec4(color, 1.0);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| iterations | int | 4-16 | 12 | No | Iterations |
| zoom | float | 0.5-4.0 | 2.0 | Yes | Zoom |
| clampLo | float | 0.01-1.0 | 0.15 | Yes | Clamp Lo |
| clampHi | float | 0.5-5.0 | 2.0 | Yes | Clamp Hi |
| foldConstant | float | 0.5-2.0 | 1.0 | Yes | Fold Constant |
| rotAngle | float | -PI..PI | 0.0 | Yes | Rotation (angle) |
| panSpeed | float | -2.0..2.0 | 0.3 | Yes | Pan Speed (speed) |
| flowSpeed | float | 0.0-2.0 | 0.3 | Yes | Flow Speed (speed) |
| flowIntensity | float | 0.0-1.0 | 0.3 | Yes | Flow Intensity |
| rotationSpeed | float | -PI..PI | 0.0 | Yes | Rotation Speed (speed) |
| glowIntensity | float | 0.001-0.1 | 0.033 | Yes | Glow Intensity (log) |
| accentIntensity | float | 0.0-0.1 | 0.033 | Yes | Accent |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 0.7 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

UI notes:
- `rotAngle` uses `ModulatableSliderAngleDeg`
- `panSpeed`, `flowSpeed`, `rotationSpeed` use `ModulatableSliderSpeedDeg`
- `glowIntensity` uses `ModulatableSliderLog`
- `iterations` uses `ImGui::SliderInt` (not modulatable)
- Standard Audio section order: Base Freq, Max Freq, Gain, Contrast, Base Bright

### Constants

<!-- Intentional deviation: glowIntensity/accentIntensity parameterize the exp() sharpness as 1/value
     instead of the research doc's hardcoded 30.0, because the research doc's range (0.001-0.1) was
     carried from the old smoothstep*1/dist formula and doesn't work as a direct multiplier on exp()
     output. Default 0.033 gives sharpness ~30, matching the reference shaders. -->
<!-- Intentional deviation: blendIntensity range 0-5 (not research doc's 0-1) matches existing
     Motherboard registration and allows overdrive. -->

- Enum: `TRANSFORM_MOTHERBOARD_BLEND` (existing, unchanged)
- Display name: `"Motherboard Blend"` (existing, unchanged)
- Macro: `REGISTER_GENERATOR` (existing, unchanged)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Rewrite MotherboardConfig and MotherboardEffect structs

**Files**: `src/effects/motherboard.h`
**Creates**: New config struct and effect struct that Wave 2 tasks depend on

**Do**: Replace the entire contents of `motherboard.h`. Use the Types section above for the exact struct layouts, field names, defaults, and range comments. The function signatures stay the same except remove the `time` field from `MotherboardEffect` and replace it with `panAccum`, `flowAccum`, `rotationAccum`. Remove uniform location fields for deleted params (`rangeXLoc`, `rangeYLoc`, `sizeLoc`, `fallOffLoc`, `timeLoc`) and add locations for new params (`zoomLoc`, `clampLoLoc`, `clampHiLoc`, `foldConstantLoc`, `panAccumLoc`, `flowAccumLoc`, `flowIntensityLoc`). Keep the same includes, include guard, forward declaration of `ColorLUT`, and function declarations. Follow the existing header as a pattern for style.

**Verify**: Header compiles in isolation (no syntax errors).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Rewrite Motherboard effect module

**Files**: `src/effects/motherboard.cpp`
**Depends on**: Wave 1 complete

**Do**: Replace the contents of `motherboard.cpp`. Follow the existing file as a structural pattern (same includes, same function signatures, same registration macro at bottom). Changes:

- **Init**: Cache uniform locations for new params (zoom, clampLo, clampHi, foldConstant, panAccum, flowAccum, flowIntensity). Remove old param locations. Initialize `panAccum = 0`, `flowAccum = 0`, `rotationAccum = 0`.
- **Setup**: Accumulate `panAccum += cfg->panSpeed * deltaTime`, `flowAccum += cfg->flowSpeed * deltaTime`, `rotationAccum += cfg->rotationSpeed * deltaTime`. Bind all new uniforms. Remove old uniform binds for deleted params.
- **Uninit**: Unchanged.
- **RegisterParams**: Register all modulatable params from the Parameters table above. Use `ROTATION_OFFSET_MAX` for `rotAngle` bounds. Use `-ROTATION_SPEED_MAX` to `ROTATION_SPEED_MAX` for `rotationSpeed` only. Other speed params (`panSpeed`, `flowSpeed`) use their own literal ranges from the Parameters table (panSpeed: -2.0 to 2.0, flowSpeed: 0.0 to 2.0) — they are linear speeds, not angular.
- **SetupMotherboard / SetupMotherboardBlend / REGISTER_GENERATOR**: Unchanged structure, just the inner calls update.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Rewrite Motherboard shader

**Files**: `shaders/motherboard.fs`
**Depends on**: Wave 1 complete (for uniform names)

**Do**: Replace the entire shader. Implement the Algorithm section from this plan. The shader declares all uniforms matching the new config fields plus accumulators (`panAccum`, `flowAccum`, `rotationAccum`). No defines needed except the constant `BAND_SAMPLES = 4`. Follow this exact sequence:

1. Coordinate setup: center, aspect-correct, rotate by `rotationAccum`, pan Y by `panAccum`, scale by `zoom`, tile with `fract(p) - 0.5`
2. Kali iteration loop up to `iterations`: `abs(p)`, hyperbolic inversion with clamp, subtract `foldConstant`, apply per-iteration rotation by `rotAngle`. Track dual orbit traps and `minit`.
3. Data streaming: add `fract(p.x + flowAccum + float(i) * 0.2) * flowIntensity` to primary trap distance
4. Exp rendering: `trace = exp(-ot1 / max(glowIntensity, 0.001))`, `junction = exp(-ot2 / max(accentIntensity, 0.001))` (skip junction if `accentIntensity == 0`)
5. FFT lookup: 4-sample band average for winning iteration's frequency range
6. Color: gradient LUT sample at `minit / iterations`, multiply by `brightness * trace`, add `junction`

Keep the top-of-file comment updated to describe the new algorithm.

**Verify**: Shader compiles at runtime (check TraceLog output for GLSL errors on app launch).

---

#### Task 2.3: Update Motherboard UI controls

**Files**: `src/ui/imgui_effects_gen_texture.cpp`
**Depends on**: Wave 1 complete (for config field names)

**Do**: Replace the body of `DrawGeneratorsMotherboard()` (the section inside `if (e->motherboard.enabled)`). Keep the outer structure (DrawSectionBegin, enabled checkbox, MoveTransformToEnd, DrawSectionEnd) identical. Replace the inner sliders to match the new config fields. Section layout:

- **Audio** section: Base Freq, Max Freq, Gain, Contrast, Base Bright (same as current, standard convention)
- **Geometry** section: Iterations (`SliderInt`), Zoom, Clamp Lo, Clamp Hi, Fold Constant, Rotation (`ModulatableSliderAngleDeg`)
- **Rendering** section: Glow Intensity (`ModulatableSliderLog`), Accent
- **Animation** section: Pan Speed (`ModulatableSliderSpeedDeg`), Flow Speed (`ModulatableSliderSpeedDeg`), Flow Intensity, Rotation Speed (`ModulatableSliderSpeedDeg`)
- Color / Output sections: unchanged (ImGuiDrawColorMode, Blend Intensity, Blend Mode)

Use parameter IDs matching RegisterParams: `"motherboard.zoom"`, `"motherboard.clampLo"`, etc. Slider formats: `"%.2f"` for most, `"%.1f"` for baseFreq/gain, `"%.0f"` for maxFreq, `"%.3f"` for glowIntensity/accentIntensity.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Update preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete (for CONFIG_FIELDS macro)

**Do**: The `MOTHERBOARD_CONFIG_FIELDS` macro in the header already lists the new fields. No include changes needed (motherboard.h is already included via effect_config.h). The `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro and `EFFECT_CONFIG_FIELDS(X)` entry for `motherboard` are already present. Verify that these references still compile with the new field list. If the fields macro name hasn't changed, no edits are needed in this file.

**Verify**: `cmake.exe --build build` compiles. Existing presets load without crash (old Motherboard fields silently ignored, new fields get defaults).

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] App launches without shader compile errors in TraceLog
- [ ] Motherboard appears in generator list, can be enabled
- [ ] Circuit trace pattern visible with default settings
- [ ] Audio reactivity works (traces brighten with music)
- [ ] Data streaming visible when flowIntensity > 0 and music is quiet
- [ ] All sliders respond in real-time
- [ ] Preset save/load works (new fields persist, old presets load defaults)
- [ ] Modulation routes work on registered params
