# Ripple Tank Merge

Absorb the Interference generator's features into Ripple Tank, creating a unified wave interference effect with two source modes: audio waveform (original Ripple Tank) and parametric sine (original Interference). Then delete the standalone Interference effect.

**Research**: `docs/research/cymatics-category.md` (Section 3: Merge)
**Depends on**: `docs/plans/ripple-tank-rename.md` (must be implemented first)

## Design

### Types

#### RippleTankConfig (merged)

Fields carried from Ripple Tank (renamed):
```
bool enabled = false;
int sourceCount = 5;           // (1-8)
float baseRadius = 0.4f;       // (0.0-1.0)
DualLissajousConfig lissajous;
bool boundaries = false;
float reflectionGain = 0.5f;   // (0.0-1.0)
float visualGain = 1.0f;       // (0.5-5.0)
float decayHalfLife = 0.5f;    // (0.1-5.0)
int diffusionScale = 4;        // (0-8)
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
float blendIntensity = 1.0f;   // (0.0-5.0)
ColorConfig gradient;
```

Fields renamed from Ripple Tank:
```
float falloffStrength = 0.5f;  // was "falloff" (0.0-5.0)
float waveScale = 50.0f;       // kept, used in audio mode (1-50)
```

Fields added from Interference:
```
int waveSource = 0;            // 0=audio waveform, 1=parametric sine (NEW)
float waveFreq = 30.0f;        // sine mode spatial frequency (5.0-100.0)
float waveSpeed = 2.0f;        // sine mode animation speed (0.0-10.0)
int falloffType = 3;           // 0=none, 1=inverse, 2=inv-square, 3=gaussian
int colorMode = 0;             // 0=intensity, 1=per-source, 2=chromatic
float chromaSpread = 0.03f;    // RGB wavelength spread (0.0-0.1)
```

<!-- Intentional deviation from research: research has 3-value visualMode (raw/absolute/contour).
     Plan unifies Cymatics contourMode + Interference visualMode into a single 4-value enum
     (raw/absolute/bands/iso-lines). Approved during planning. -->
Fields merged (unified visualMode):
```
int visualMode = 0;            // 0=raw, 1=absolute, 2=bands, 3=iso-lines
int contourCount = 5;          // band/line count (1-20)
```

Fields removed: `contourMode` (replaced by `visualMode`)

#### RippleTankEffect (merged)

Gains from InterferenceEffect:
```
float time;           // accumulator for sine mode animation
int timeLoc;
int waveSourceLoc;
int waveFreqLoc;
int falloffTypeLoc;
int visualModeLoc;
int colorModeLoc;
int chromaSpreadLoc;
int phasesLoc;
```

Renamed: `falloffLoc` → `falloffStrengthLoc`

### Algorithm

The merged shader combines both wave generation strategies in a single ping-pong trail system.

#### Wave generation (per source)

```glsl
uniform int waveSource;    // 0=audio, 1=sine
uniform float waveScale;   // audio: delay scaling
uniform float waveFreq;    // sine: spatial frequency
uniform float time;        // sine: accumulated time

float waveFromSource(vec2 uv, vec2 src, float phase) {
    float dist = length(uv - src);
    float atten = falloff(dist, falloffType, falloffStrength);

    if (waveSource == 0) {
        // Audio: sample waveform ring buffer with distance-based delay
        float delay = dist * waveScale;
        float spreading = 1.0 / sqrt(dist + 0.01);
        return fetchWaveform(delay) * spreading * atten;
    } else {
        // Sine: parametric standing wave
        return sin(dist * waveFreq - time + phase) * atten;
    }
}
```

#### Falloff (from interference.fs)

```glsl
float falloff(float d, int type, float strength) {
    float safeDist = max(d, 0.001);
    if (type == 0) return 1.0;                                         // None
    if (type == 1) return 1.0 / (1.0 + safeDist * strength);           // Inverse
    if (type == 2) return 1.0 / (1.0 + safeDist * safeDist * strength); // Inv-square
    return exp(-safeDist * safeDist * strength);                        // Gaussian
}
```

#### Visual mode (unified)

```glsl
float visualize(float wave, int mode, int bands) {
    if (mode == 1) return abs(wave);                                    // Absolute
    if (mode == 2) return floor(wave * float(bands) + 0.5) / float(bands); // Bands
    if (mode == 3) {                                                    // Iso-lines
        float contoured = fract(abs(wave) * float(bands));
        float lineWidth = 0.1;
        return wave * smoothstep(0.0, lineWidth, contoured)
                     * smoothstep(lineWidth * 2.0, lineWidth, contoured);
    }
    return wave;                                                        // Raw
}
```

#### Color mode (from interference.fs)

```glsl
uniform int colorMode;       // 0=intensity, 1=per-source, 2=chromatic
uniform float chromaSpread;

// colorMode == 0 (Intensity): map visualized wave → LUT
//   Raw mode: t = wave * 0.5 + 0.5
//   Other modes: t = wave (already in useful range)
//   color = texture(colorLUT, vec2(t, 0.5)).rgb

// colorMode == 1 (PerSource): proximity-weighted LUT shift
//   Compute shift = weighted average of (sourceIndex / (count-1) - 0.5) by 1/(dist+0.1)
//   t = abs(visualWave) * 0.5 + 0.5 + shift, clamped
//   color = texture(colorLUT, vec2(t, 0.5)).rgb

// colorMode == 2 (Chromatic): separate RGB wavelengths
//   For each channel c in {R,G,B}:
//     chromaScale[c] = {1.0 - chromaSpread, 1.0, 1.0 + chromaSpread}
//     Re-sum all sources with freq * chromaScale[c]
//     Apply visualize() per channel
//   color = tanh(chromaWave * visualGain)
//   brightness = 1.0 (color IS brightness)
```

#### Trail blending (from cymatics.fs, active for both modes)

```glsl
// Same ping-pong trail as current: diffuse + decay previous frame, max(old, new)
vec4 existing = (diffusionScale == 0)
    ? texture(texture0, fragTexCoord)
    : separableGaussianBlur(texture0, fragTexCoord, diffusionScale, resolution);
existing *= decayFactor;

// Brightness: gain-independent color, gain controls visibility
float t = tanh(visualWave) * 0.5 + 0.5;
vec3 color = /* from colorMode logic */;
float brightness = abs(tanh(visualWave * visualGain)) * value;
vec4 newColor = vec4(color * brightness, brightness);

finalColor = max(existing, newColor);
```

Note: When `waveSource == 1` (sine mode), the `value` uniform should be 1.0 since the sine wave doesn't need HSV brightness extraction. The Setup function handles this.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveSource | int | 0-1 | 0 | No | "Wave Source" (Combo: "Audio\0Sine\0") |
| waveScale | float | 1-50 | 50.0 | Yes | "Wave Scale##rt" |
| waveFreq | float | 5-100 | 30.0 | Yes | "Wave Freq##rt" |
| waveSpeed | float | 0-10 | 2.0 | Yes | "Wave Speed##rt" |
| falloffType | int | 0-3 | 3 | No | "Falloff##rt" (Combo) |
| falloffStrength | float | 0-5 | 0.5 | Yes | "Falloff Strength##rt" |
| visualMode | int | 0-3 | 0 | No | "Visual Mode##rt" (Combo: "Raw\0Absolute\0Bands\0Lines\0") |
| contourCount | int | 1-20 | 5 | No | "Contours##rt" (visible when visualMode >= 2) |
| visualGain | float | 0.5-5 | 1.0 | Yes | "Gain##rt" |
| colorMode | int | 0-2 | 0 | No | "Color Mode##rt" (Combo: "Intensity\0PerSource\0Chromatic\0") |
| chromaSpread | float | 0-0.1 | 0.03 | Yes | "Chroma Spread##rt" (visible when colorMode == 2) |
| sourceCount | int | 1-8 | 5 | No | "Source Count##rt" |
| baseRadius | float | 0-1.0 | 0.4 | Yes | "Base Radius##rt" |
| boundaries | bool | - | false | No | "Boundaries##rt" |
| reflectionGain | float | 0-1 | 0.5 | Yes | "Reflection Gain##rt" |
| decayHalfLife | float | 0.1-5 | 0.5 | No | "Decay##rt" |
| diffusionScale | int | 0-8 | 4 | No | "Diffusion##rt" |

### Conditional UI

- `waveSource == 0` (Audio): show `waveScale`. Hide `waveFreq`, `waveSpeed`.
- `waveSource == 1` (Sine): show `waveFreq`, `waveSpeed`. Hide `waveScale`.
- `visualMode >= 2` (Bands/Lines): show `contourCount`.
- `colorMode == 2` (Chromatic): show `chromaSpread`.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Update ripple_tank.h

**Files**: `src/effects/ripple_tank.h` (modify)

**Do**: Add new fields to `RippleTankConfig` per Design section. Rename `falloff` → `falloffStrength`. Replace `contourMode` with `visualMode`. Add new fields to `RippleTankEffect` struct (time accumulator, new uniform locations). Rename `falloffLoc` → `falloffStrengthLoc`. Update `RIPPLE_TANK_CONFIG_FIELDS` macro to match. Update function signatures: `RippleTankEffectSetup` gains no new params (time is internal; waveformTexture + writeIndex still passed for audio mode).

**Verify**: Header compiles standalone.

---

### Wave 2: Shader + C++ + Integration (parallel — no file overlap)

#### Task 2.1: Merge shader

**Files**: `shaders/ripple_tank.fs` (modify)

**Do**: Implement the Algorithm section. The shader already has the audio waveform path. Add:
1. `waveSource`, `waveFreq`, `time`, `falloffType`, `visualMode`, `colorMode`, `chromaSpread`, `phases[8]` uniforms
2. `falloff()` function (4-way, from interference.fs)
3. Replace inline Gaussian falloff with `falloff()` call
4. `waveFromSource()` function that branches on `waveSource`
5. `visualize()` function replacing the inline contourMode logic
6. Color mode logic (intensity/per-source/chromatic from interference.fs)
7. Rename `falloff` uniform → `falloffStrength`
8. Keep trail blending (diffusion + decay + max) unchanged
9. Rename `contourMode` uniform → `visualMode`

**Verify**: Shader file has no syntax errors (checked at runtime).

---

#### Task 2.2: Update ripple_tank.cpp

**Files**: `src/effects/ripple_tank.cpp` (modify)

**Do**:
1. In `RippleTankEffectInit`: cache new uniform locations (`timeLoc`, `waveSourceLoc`, `waveFreqLoc`, `falloffTypeLoc`, `visualModeLoc`, `colorModeLoc`, `chromaSpreadLoc`, `phasesLoc`). Rename `falloffLoc` → `falloffStrengthLoc`. Initialize `e->time = 0.0f`.
2. In `RippleTankEffectSetup`: bind new uniforms. When `waveSource == 1`: accumulate `e->time += cfg->waveSpeed * deltaTime`, compute per-source phases as `i/count * TWO_PI_F`, bind phases array and time. When `waveSource == 0`: bind waveScale, bufferSize, writeIndex as before. Always bind: falloffType, falloffStrength, visualMode, contourCount, visualGain, colorMode, chromaSpread. Set `value = 1.0` when `waveSource == 1`.
3. In `RippleTankRegisterParams`: add new modulatable params (`rippleTank.waveFreq`, `rippleTank.waveSpeed`, `rippleTank.chromaSpread`). Rename `rippleTank.falloff` → `rippleTank.falloffStrength`.
4. UI: Restructure `DrawRippleTankParams` per conditional UI rules. Add Wave Source combo at top. Show/hide waveScale vs waveFreq/waveSpeed based on waveSource. Add Falloff Type combo. Replace contourMode combo with visualMode combo (4 options). Add Color Mode combo with conditional chromaSpread slider.

**Verify**: Compiles.

---

#### Task 2.3: Update effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp` (modify)

**Do**:
1. Remove `#include "effects/interference.h"` and `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterferenceConfig, ...)` registration.
2. Remove `X(interference)` from the `EFFECT_CONFIG_FIELDS` macro.
3. The `RippleTankConfig` serialization macro already covers new fields since `RIPPLE_TANK_CONFIG_FIELDS` was updated in Wave 1.
4. Add backwards-compatible deserialization in `from_json` for `EffectConfig`: after the main deserialize block, add migration for old `"cymatics"` presets that have `contourMode` field — map `contourMode` values: 0→`visualMode=0`, 1→`visualMode=2`, 2→`visualMode=3`.

**Verify**: Compiles.

---

#### Task 2.4: Update effect_config.h

**Files**: `src/config/effect_config.h` (modify)

**Do**:
1. Remove `#include "effects/interference.h"`
2. Remove `TRANSFORM_INTERFERENCE_BLEND` from `TransformEffectType` enum
3. Remove `InterferenceConfig interference` field from `EffectConfig`

**Verify**: Compiles.

---

#### Task 2.5: Update post_effect.h

**Files**: `src/render/post_effect.h` (modify)

**Do**:
1. Remove `#include "effects/interference.h"`
2. Remove `InterferenceEffect interference` field from `PostEffect`

**Verify**: Compiles.

---

### Wave 3: Cleanup

#### Task 3.1: Delete Interference files

**Files**: `src/effects/interference.h` (delete), `src/effects/interference.cpp` (delete), `shaders/interference.fs` (delete)

**Do**: Delete the three Interference files. Verify no remaining references to `InterferenceConfig`, `InterferenceEffect`, `TRANSFORM_INTERFERENCE_BLEND`, `interference.h`, or `interference.fs` in source files.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Ripple Tank appears in UI with Wave Source toggle
- [ ] Audio mode (waveSource=0) renders same as before rename
- [ ] Sine mode (waveSource=1) renders same as old Interference
- [ ] No references to Interference remain in source (excluding docs)
- [ ] CYMATICBOB preset still loads (cymatics fallback + contourMode migration)
