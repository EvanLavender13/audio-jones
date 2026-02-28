# Motherboard Rewrite

Rewrite the motherboard generator to use one unified Kali-family fractal algorithm where every difference between the three Circuits references (Kali's Circuits I/II/III) is a continuous parameter. Each reference shader becomes a specific point in the parameter space. No mode combos, no branching between separate functions.

**Research**: `docs/research/motherboard-rewrite.md`

## Design

### Types

```c
struct MotherboardConfig {
  bool enabled = false;

  // Geometry
  int iterations = 10;           // Fold depth (4-16)
  float zoom = 0.5f;             // Scale before tiling (0.1-4.0)
  float inversionBlend = 0.0f;   // 0=dot(p,p), 1=abs(p.x*p.y) (0.0-1.0)
  float clampLo = 0.1f;          // Inversion lower bound (0.0-1.0)
  float clampHi = 3.0f;          // Inversion upper bound (0.5-5.0)
  float foldConstantX = 1.0f;    // Post-inversion X translation (0.5-2.0)
  float foldConstantY = 1.0f;    // Post-inversion Y translation (0.5-2.0)
  float iterRotation = 0.0f;     // Per-iteration fold rotation, radians (-PI..PI)
  float foldOffset = 0.0f;       // Y-fold asymmetry offset (0.0-0.5)

  // Orbit Traps
  float trapOffset = 0.0f;       // Primary trap Y offset (0.0-2.0)
  float ringRadius = 0.0f;       // Ring trap radius, 0=disabled (0.0-0.5)
  float junctionBox = 0.0f;      // Junction shape: 0=point, higher=box (0.0-1.0)

  // Rendering
  float traceWidth = 0.005f;     // Trace glow thickness (0.001-0.05)
  float depthScale = 0.0f;       // Width scaling by iteration depth (0.0-2.0)
  float junctionHardness = 0.0f; // Junction edge: 0=soft, 1=hard (0.0-1.0)
  float bgLevel = 0.0f;          // Background level for compositing (0.0-0.3)

  // Animation
  float panSpeed = 0.3f;         // Drift speed (-2.0..2.0)
  float flowSpeed = 0.3f;        // Streaming speed (0.0-2.0)
  float flowIntensity = 0.5f;    // Streaming visibility (0.0-1.0)
  float rotationSpeed = 0.0f;    // Pattern rotation rate, radians/second

  // Audio
  float baseFreq = 55.0f;        // Lowest frequency band Hz (27.5-440.0)
  float maxFreq = 14000.0f;      // Highest frequency band Hz (1000-16000)
  float gain = 2.0f;             // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;            // Contrast exponent (0.1-3.0)
  float baseBright = 0.15f;      // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

`MotherboardEffect` struct adds uniform locations for all new params: `inversionBlendLoc`, `foldConstantXLoc`, `foldConstantYLoc`, `iterRotationLoc`, `foldOffsetLoc`, `trapOffsetLoc`, `ringRadiusLoc`, `junctionBoxLoc`, `traceWidthLoc`, `depthScaleLoc`, `junctionHardnessLoc`, `bgLevelLoc`. Remove old: `foldConstantLoc`, `rotAngleLoc`, `glowIntensityLoc`, `accentIntensityLoc`.

### Algorithm

The shader implements one unified fractal loop with parameterized inversion, orbit traps, and rendering.

#### Core Loop

```glsl
// Pre-loop rotation matrix (per-iteration fold)
float cf = cos(iterRotation), sf = sin(iterRotation);
mat2 foldRot = mat2(cf, -sf, sf, cf);

float ot1 = 1000.0, ot2 = 1000.0;
int minit = 0;

for (int i = 0; i < iterations; i++) {
    p = abs(p);
    p *= foldRot;
    p.y = abs(p.y - foldOffset);

    // Hybrid inversion: blend between dot-product and axis-product
    float divisor = mix(dot(p, p), abs(p.x * p.y), inversionBlend);
    p = p / clamp(divisor, clampLo, clampHi) - vec2(foldConstantX, foldConstantY);

    // Primary trap: min of axis distances with flow modulation
    float primaryTrap = min(abs(p.x), abs(p.y - trapOffset));
    primaryTrap += fract(p.x + flowAccum + float(i) * 0.2) * flowIntensity;

    // Ring trap overlay (Ref 1 compound geometry, disabled when ringRadius == 0)
    if (ringRadius > 0.0) {
        float ringDist = abs(length(p) - ringRadius);
        primaryTrap = min(max(primaryTrap, -length(p) + ringRadius), ringDist);
    }

    if (primaryTrap < ot1) {
        ot1 = primaryTrap;
        minit = i;
    }

    // Junction trap: point distance or box distance
    float jd = length(max(vec2(0.0), abs(p) - junctionBox));
    ot2 = min(ot2, jd);
}
```

#### Rendering

```glsl
// Trace glow with depth scaling
float w = traceWidth * (1.0 + float(minit) * depthScale);
float trace = exp(-ot1 / max(w, 0.001));

// Junction glow: blend between soft exp and hard smoothstep
float junctionSoft = exp(-ot2 / max(traceWidth, 0.001));
float junctionHard = 1.0 - smoothstep(traceWidth, traceWidth + 0.01, ot2);
float junction = mix(junctionSoft, junctionHard, junctionHardness);

// Shape accumulation (Ref 1 model)
float shape = trace + junction;
```

#### Compositing

```glsl
// FFT: winning iteration -> frequency band (band-averaged, 4 samples)
float t0 = float(minit) / float(iterations);
float t1 = float(minit + 1) / float(iterations);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float energy = 0.0;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
}
energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + energy;

// Gradient color from winning iteration
vec3 layerColor = texture(gradientLUT, vec2((float(minit) + 0.5) / float(iterations), 0.5)).rgb;

// Shape * color compositing with background level
vec3 color = layerColor * shape * brightness;
finalColor = vec4(mix(vec3(bgLevel), color, clamp(shape, 0.0, 1.0)), 1.0);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| iterations | int | 4-16 | 10 | No | Iterations |
| zoom | float | 0.1-4.0 | 0.5 | Yes | Zoom |
| inversionBlend | float | 0.0-1.0 | 0.0 | Yes | Inversion Blend |
| clampLo | float | 0.0-1.0 | 0.1 | Yes | Clamp Lo |
| clampHi | float | 0.5-5.0 | 3.0 | Yes | Clamp Hi |
| foldConstantX | float | 0.5-2.0 | 1.0 | Yes | Fold X |
| foldConstantY | float | 0.5-2.0 | 1.0 | Yes | Fold Y |
| iterRotation | float | -PI..PI | 0.0 | Yes | Rotation (angle) |
| foldOffset | float | 0.0-0.5 | 0.0 | Yes | Fold Offset |
| trapOffset | float | 0.0-2.0 | 0.0 | Yes | Trap Offset |
| ringRadius | float | 0.0-0.5 | 0.0 | Yes | Ring Radius |
| junctionBox | float | 0.0-1.0 | 0.0 | Yes | Junction Box |
| traceWidth | float | 0.001-0.05 | 0.005 | Yes | Trace Width |
| depthScale | float | 0.0-2.0 | 0.0 | Yes | Depth Scale |
| junctionHardness | float | 0.0-1.0 | 0.0 | Yes | Junction Hardness |
| bgLevel | float | 0.0-0.3 | 0.0 | Yes | Background |
| panSpeed | float | -2.0..2.0 | 0.3 | Yes | Pan Speed |
| flowSpeed | float | 0.0-2.0 | 0.3 | Yes | Flow Speed |
| flowIntensity | float | 0.0-1.0 | 0.5 | Yes | Flow Intensity |
| rotationSpeed | float | -PI..PI | 0.0 | Yes | Rotation Speed (/s) |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 0.7 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |

### Constants

- Enum: `TRANSFORM_MOTHERBOARD_BLEND` (unchanged)
- Display name: `"Motherboard"` (unchanged)
- Category section: 12 (Texture) (unchanged)

### UI Sections

- **Audio**: Base Freq, Max Freq, Gain, Contrast, Base Bright
- **Geometry**: Iterations, Zoom, Inversion Blend, Clamp Lo, Clamp Hi, Fold X, Fold Y, Rotation (angle), Fold Offset
- **Orbit Traps**: Trap Offset, Ring Radius, Junction Box
- **Rendering**: Trace Width (`ModulatableSliderLog`), Depth Scale, Junction Hardness, Background
- **Animation**: Pan Speed, Flow Speed, Flow Intensity, Rotation Speed (/s)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Rewrite motherboard header

**Files**: `src/effects/motherboard.h`
**Creates**: Updated `MotherboardConfig` and `MotherboardEffect` structs

**Do**: Replace config and effect structs per the Design section above. Remove old fields (`foldConstant`, `rotAngle`, `glowIntensity`, `accentIntensity`). Add all new fields with defaults and range comments. Update `MOTHERBOARD_CONFIG_FIELDS` macro to list all new field names. Update `MotherboardEffect` uniform location `int` fields to match new uniforms. Function signatures unchanged.

**Verify**: `cmake.exe --build build` compiles (will have linker errors until Wave 2).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Rewrite motherboard shader

**Files**: `shaders/motherboard.fs`
**Depends on**: Wave 1

**Do**: Rewrite the shader implementing the Algorithm section above. Keep the existing attribution comment at top, add Circuits I/II/III attribution per the research doc's Attribution section. Uniforms must match the new config field names exactly. Keep `BAND_SAMPLES = 4` FFT approach. Coordinate setup, rotation, pan, zoom, and tiling unchanged from existing shader.

**Verify**: File parses as valid GLSL 330.

#### Task 2.2: Rewrite motherboard C++ implementation

**Files**: `src/effects/motherboard.cpp`
**Depends on**: Wave 1

**Do**: Update `MotherboardEffectInit` to cache all new uniform locations (remove old ones). Update `MotherboardEffectSetup` to bind all new uniforms. Update `MotherboardRegisterParams` with all new param IDs and ranges per the Parameters table. Update colocated UI (`DrawMotherboardParams`) with the UI Sections layout above — add "Orbit Traps" and update "Rendering" section. Use `ModulatableSliderLog` for `traceWidth`, `ModulatableSliderAngleDeg` for `iterRotation`, `ModulatableSliderSpeedDeg` for `rotationSpeed`. Follow existing motherboard.cpp as the pattern for everything else. Setup bridge and registration macro unchanged.

**Verify**: `cmake.exe --build build` compiles with no warnings.

#### Task 2.3: Update serialization fields

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1

**Do**: The `MOTHERBOARD_CONFIG_FIELDS` macro in the header provides the field list. No code change needed in this file — the macro expansion handles it. Verify `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MotherboardConfig, MOTHERBOARD_CONFIG_FIELDS)` still references the correct macro. If the macro name hasn't changed (it hasn't), no edit needed.

**Verify**: `cmake.exe --build build` compiles.

---

## Implementation Notes

The plan's unified continuous-parameter algorithm was abandoned. The implementation uses three separate fractal functions (`fractalCircuits`, `fractalCircuitsII`, `fractalCircuitsIII`) selected by a discrete mode combo, each faithfully reproducing its Kali reference shader.

Key deviations from plan:
- **No unified loop**: Three independent fractal functions instead of one parameterized loop. Each mode has its own inversion, orbit trap, and rendering logic matching the original Shadertoy references.
- **Mode combo replaces continuous params**: `inversionBlend`, `iterRotation`, `foldOffset`, `trapOffset`, `ringRadius`, `junctionBox`, `depthScale`, `junctionHardness`, `bgLevel` were all dropped. A single `int mode` selects between the three variants.
- **`foldConstant` stays scalar**: Plan split it into `foldConstantX`/`foldConstantY`. Implementation keeps one `float foldConstant`; Mode 1 applies the anisotropic split internally (`foldConstant * 1.5, foldConstant`).
- **`traceWidth` replaces `glowIntensity`/`accentIntensity`**: Single width param used differently per mode (depth-scaled pow in Mode 0, exp divisor in Mode 1, hardcoded `exp(-5*o)` in Mode 2).
- **Mode 2 compositing**: Uses `o` (post-exp, continuous) to index gradient LUT for smooth color, matching how the reference uses `o` to drive HSV hue. Junction color sampled from fixed LUT position. Other modes use discrete `winIt`/`minit` for LUT indexing.
- **Zoom range**: Lowered minimum from 0.5 to 0.1 to accommodate Ref 3's lower zoom values.
- **Removed params**: `rotAngle`, `accentIntensity`, `glowIntensity` (from old unified shader). Plan's `foldConstantX`, `foldConstantY`, `inversionBlend`, `iterRotation`, `foldOffset`, `trapOffset`, `ringRadius`, `junctionBox`, `depthScale`, `junctionHardness`, `bgLevel` were never added.
- **Added**: `int mode`, 7x7 AA supersampling, proper three-shader attribution block.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Texture generator category
- [ ] All new sliders visible and functional
- [ ] Default parameters produce visible output
- [ ] Preset save/load round-trips all new fields
- [ ] Modulation routes to all registered parameters
