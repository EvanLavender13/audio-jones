# Arc Strobe

Glowing line segments connecting points on a 3D dual-Lissajous knot, forming a tangled web of electrical arcs that strobe in sequence. Each segment maps to a semitone frequency — FFT energy drives brightness while a rotating strobe sweep gates which segments fire, creating a musical frequency scanner that pulses around the knot.

**Research**: `docs/research/arc_strobe.md`

## Design

### Types

**DualLissajousConfig extensions** (add to existing struct in `dual_lissajous_config.h`):

```
// New Z-axis fields (default 0 preserves existing 2D behavior)
float amplitudeZ = 0.0f  // Depth amplitude (0 = flat)
float freqZ1 = 0.0f      // Primary Z frequency
float freqZ2 = 0.0f      // Secondary Z frequency (0 = disabled)
float offsetZ2 = 0.0f    // Phase offset for secondary Z
```

Existing fields unchanged: `amplitude`, `motionSpeed`, `freqX1`, `freqY1`, `freqX2`, `freqY2`, `offsetX2`, `offsetY2`, `phase`. Rename nothing — `amplitude` remains the XY amplitude.

**ArcStrobeConfig** (user-facing parameters):

```
bool enabled = false

// Lissajous motion (reuse existing config, extended with Z)
DualLissajousConfig lissajous  // Override defaults: amplitude=0.3, amplitudeZ=0.15,
                               // freqX1=0.05, freqY1=0.08, freqZ1=0.03,
                               // offsetY2=3.48, offsetZ2=1.57

// Shape
float orbitOffset = 1.0f       // 0.1–PI, chord length along curve
float lineThickness = 0.005f   // 0.001–0.02, segment width

// Glow
float glowIntensity = 0.01f    // 0.001–0.05
float falloffExponent = 1.2f   // 0.8–2.0
float depthNear = 0.3f         // 0.1–1.0

// Strobe
float strobeSpeed = 1.0f       // 0.0–10.0
float strobeDecay = 20.0f      // 5.0–40.0
float noiseStrength = 0.3f     // 0.0–1.0

// FFT
float baseFreq = 220.0f        // 20–880
int numOctaves = 5             // 1–8
int segmentsPerOctave = 12     // 4–48
float gain = 5.0f              // 1–20
float curve = 2.0f             // 0.5–4.0
float baseBright = 0.05f       // 0.0–0.5

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}

// Blend compositing
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**ArcStrobeEffect** (runtime state + shader handles):

```
Shader shader
ColorLUT* gradientLUT
float strobeTime         // CPU-accumulated strobe time

// Uniform locations — one int per uniform
resolutionLoc, fftTextureLoc, sampleRateLoc, phaseLoc,
amplitudeLoc, amplitudeZLoc, orbitOffsetLoc, lineThicknessLoc,
freqX1Loc, freqY1Loc, freqZ1Loc, freqX2Loc, freqY2Loc, freqZ2Loc,
offsetX2Loc, offsetY2Loc, offsetZ2Loc,
glowIntensityLoc, falloffExponentLoc, depthNearLoc,
strobeTimeLoc, strobeDecayLoc, noiseStrengthLoc,
baseFreqLoc, numOctavesLoc, segmentsPerOctaveLoc,
gainLoc, curveLoc, baseBrightLoc,
gradientLUTLoc
```

Note: `phase` lives on `lissajous.phase` — no separate accumulator needed. `ArcStrobeEffectSetup` calls the Lissajous phase accumulation, then reads `lissajous.phase` for the uniform.

### Algorithm

The shader evaluates all segments per pixel. Each segment connects two points on a 3D dual-Lissajous knot.

**Lissajous evaluation** (per segment, in shader): Given parameter `t` and CPU-accumulated `phase`, compute a 3D point using dual sinusoids per axis — matching the `DualLissajousConfig` pattern extended to Z. When a secondary frequency is 0, that term drops out. Normalize by number of active terms per axis. Scale x,y by `amplitude`, z by `amplitudeZ`.

**Segment construction**: Point P at `t_i = TWO_PI * i / totalSegments`, point Q at `t_i + orbitOffset`. Project to 2D using x,y. Z reserved for depth modulation.

**Glow rendering**: `sdSegment(uv, P.xy, Q.xy)` minus `lineThickness`, then inverse-distance glow `glowIntensity / (pow(dist, 1/falloffExponent) + EPSILON)`. Depth brightness scales glow by `depthNear + (1 - depthNear) * depthFactor` where depthFactor maps Z front-to-back into 0..1.

**Sequential strobe**: `exp(-strobeDecay * fract(strobeTime + i / totalSegments))`. CPU accumulates `strobeTime += strobeSpeed * deltaTime` so the shader uses `strobeTime` directly — no second multiplication by `strobeSpeed` in GLSL (prevents phase jumps on live speed changes). Noise shimmer modulates the decay exponent via analytic grain: `dot(sin(uv * 7000.0), cos(uv.yx * 500.0))` scaled by `noiseStrength`.

**FFT lookup**: Semitone index from segment position, frequency from `baseFreq * 2^(semitone/12)`, sample `fftTexture`, shape with `pow(clamp(raw * gain, 0, 1), curve)`. Final brightness: `glow * strobeEnvelope * (baseBright + mag)`. Note: when `segmentsPerOctave > 12`, multiple adjacent segments map to the same semitone bin — intentional, produces wider glowing arcs per active note.

**Color**: Gradient LUT indexed by pitch class `fract(semitoneIndex / 12)`.

**Tonemap**: `tanh(result)` soft clipping (matches Filaments).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| lissajous.amplitude | float | 0.05–0.8 | 0.3 | Yes | Amplitude |
| lissajous.amplitudeZ | float | 0.0–0.5 | 0.15 | Yes | Amplitude Z |
| lissajous.motionSpeed | float | 0.0–5.0 | 1.0 | Yes | Motion Speed |
| lissajous.freqX1 | float | 0.01–0.5 | 0.05 | No | Freq X |
| lissajous.freqY1 | float | 0.01–0.5 | 0.08 | No | Freq Y |
| lissajous.freqZ1 | float | 0.01–0.5 | 0.03 | No | Freq Z |
| lissajous.freqX2 | float | 0.0–0.5 | 0.0 | No | Freq X2 |
| lissajous.freqY2 | float | 0.0–0.5 | 0.0 | No | Freq Y2 |
| lissajous.freqZ2 | float | 0.0–0.5 | 0.0 | No | Freq Z2 |
| lissajous.offsetX2 | float | 0–TWO_PI | 0.3 | Yes | Offset X2 |
| lissajous.offsetY2 | float | 0–TWO_PI | 3.48 | Yes | Offset Y2 |
| lissajous.offsetZ2 | float | 0–TWO_PI | 1.57 | Yes | Offset Z2 |
| orbitOffset | float | 0.1–PI | 1.0 | Yes | Orbit Offset |
| lineThickness | float | 0.001–0.02 | 0.005 | Yes | Line Thickness |
| glowIntensity | float | 0.001–0.05 | 0.01 | Yes | Glow Intensity |
| falloffExponent | float | 0.8–2.0 | 1.2 | Yes | Falloff |
| depthNear | float | 0.1–1.0 | 0.3 | Yes | Depth Near |
| strobeSpeed | float | 0.0–10.0 | 1.0 | Yes | Strobe Speed |
| strobeDecay | float | 5.0–40.0 | 20.0 | Yes | Strobe Decay |
| noiseStrength | float | 0.0–1.0 | 0.3 | Yes | Noise Strength |
| baseFreq | float | 20–880 | 220 | Yes | Base Freq (Hz) |
| numOctaves | int | 1–8 | 5 | No | Octaves |
| segmentsPerOctave | int | 4–48 | 12 | No | Segments/Octave |
| gain | float | 1–20 | 5.0 | Yes | Gain |
| curve | float | 0.5–4.0 | 2.0 | Yes | Contrast |
| baseBright | float | 0.0–0.5 | 0.05 | Yes | Base Bright |
| gradient | ColorConfig | — | GRADIENT | No | Color |
| blendMode | EffectBlendMode | — | SCREEN | No | Blend Mode |
| blendIntensity | float | 0.0–5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_ARC_STROBE_BLEND`
- Display name: `"Arc Strobe Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS`
- Config member: `arcStrobe`
- Effect member: `arcStrobe`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Extend DualLissajousConfig with Z-axis support

**Files**: `src/config/dual_lissajous_config.h` (modify), `src/ui/ui_units.h` (modify)
**Creates**: 3D Lissajous fields and UI helper that Arc Strobe and future effects use

**Do**:

In `dual_lissajous_config.h`: Add four fields after `offsetY2`: `amplitudeZ = 0.0f`, `freqZ1 = 0.0f`, `freqZ2 = 0.0f`, `offsetZ2 = 0.0f`. All default to 0 so existing 2D callers are unaffected. Do NOT modify `DualLissajousUpdate` or `DualLissajousUpdateCircular` — Arc Strobe evaluates positions in GLSL, not on CPU.

In `ui_units.h`: Extend `DrawLissajousControls` signature with `bool show3D = false`. When `show3D` is true, draw additional sliders after the existing Y controls: Freq Z, Freq Z2, Offset Z2 (following the same style as existing X/Y sliders), plus Amplitude Z as a modulatable slider. Also add a `freqMin` parameter defaulting to 0.0f — Arc Strobe needs a nonzero minimum (0.01) for its primary frequencies. When offsets are shown and `paramPrefix` is provided, use `ModulatableSliderAngleDeg` for all offset sliders (X2, Y2, Z2) so they're modulatable for free.

**Verify**: `cmake.exe --build build` compiles. Existing effects using `DrawLissajousControls` unchanged (default `show3D = false`).

#### Task 1.2: Create ArcStrobeConfig and ArcStrobeEffect header

**Files**: `src/effects/arc_strobe.h` (create)
**Creates**: Config struct and Effect struct that all other tasks depend on
**Depends on**: Task 1.1 (needs extended DualLissajousConfig)

**Do**: Create header following Filaments pattern (`src/effects/filaments.h`). `ArcStrobeConfig` embeds `DualLissajousConfig lissajous` with overridden defaults (amplitude=0.3, amplitudeZ=0.15, freqX1=0.05, freqY1=0.08, freqZ1=0.03, offsetY2=3.48, offsetZ2=1.57). Remaining fields per the Types section above. `ArcStrobeEffect` has Shader, ColorLUT pointer, one float accumulator (`strobeTime`), and one `int` location per uniform. `phase` lives on `lissajous.phase`. Declare `ArcStrobeEffectInit`, `ArcStrobeEffectSetup` (takes fftTexture like Filaments), `ArcStrobeEffectUninit`, `ArcStrobeConfigDefault`, `ArcStrobeRegisterParams`. Include `config/dual_lissajous_config.h`, `render/blend_mode.h`, and `render/color_config.h`.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All tasks in this wave depend on Wave 1 but have zero file overlap with each other.

#### Task 2.1: Implement ArcStrobe effect module

**Files**: `src/effects/arc_strobe.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement following Filaments pattern (`src/effects/filaments.cpp`). `ArcStrobeEffectInit` loads shader `shaders/arc_strobe.fs`, caches all uniform locations, inits ColorLUT from `cfg->gradient`, sets `strobeTime = 0`. `ArcStrobeEffectSetup` accumulates `cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime` and `strobeTime += cfg->strobeSpeed * deltaTime`, updates ColorLUT, binds all uniforms including resolution, fftTexture, sampleRate, gradientLUT. Read Lissajous fields from `cfg->lissajous.*` for uniform binding. `ArcStrobeEffectUninit` unloads shader and frees LUT. `ArcStrobeConfigDefault` returns default-initialized struct. `ArcStrobeRegisterParams` registers all modulatable params from the Parameters table using `ModEngineRegisterParam` — Lissajous params use `"arcStrobe.lissajous.*"` prefix, own params use `"arcStrobe.*"` prefix. Include `audio/audio.h` for `AUDIO_SAMPLE_RATE`, `automation/modulation_engine.h`, `render/color_lut.h`, `config/constants.h`.

**Verify**: Compiles.

#### Task 2.2: Create fragment shader

**Files**: `shaders/arc_strobe.fs` (create)
**Depends on**: Wave 1 (needs to know uniform names)

**Do**: Write GLSL 330 fragment shader implementing the Algorithm section. Follow `shaders/filaments.fs` structure: centered UV, `sdSegment` helper, per-segment loop over `segmentsPerOctave * numOctaves` iterations. Inside loop: evaluate dual-Lissajous 3D point P at `t_i` and Q at `t_i + orbitOffset` using `phase` uniform + 6 freq uniforms + 3 offset uniforms. When secondary freq is 0, that term drops out. Normalize by active terms per axis. Scale x,y by `amplitude`, z by `amplitudeZ`. Project to 2D (x,y), compute depth factor from z. Compute `sdSegment` distance, glow with depth scaling. Compute strobe envelope with noise shimmer in the decay exponent (analytic grain: `dot(sin(uv*7000.0), cos(uv.yx*500.0))`). FFT semitone lookup matching Filaments pattern. Gradient LUT color by pitch class. Accumulate `result += color * glow * strobeEnv * (baseBright + mag)`. Final: `tanh(result)`.

**Verify**: Compiles with glslangValidator if available, otherwise visual test.

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/arc_strobe.h"` with other effect includes. Add `TRANSFORM_ARC_STROBE_BLEND` enum entry before `TRANSFORM_EFFECT_COUNT` (place near other generator blends like FILAMENTS_BLEND). Add name case `"Arc Strobe Blend"` in `TransformEffectName()`. Add `ArcStrobeConfig arcStrobe;` member to `EffectConfig`. Add `IsTransformEnabled` case: `return e->arcStrobe.enabled && e->arcStrobe.blendIntensity > 0.0f;` (generator blend pattern). Do NOT touch order array — it auto-populates from enum.

**Verify**: Compiles.

#### Task 2.4: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1

**Do**: In `post_effect.h`: add `#include "effects/arc_strobe.h"`, add `ArcStrobeEffect arcStrobe;` member, add `bool arcStrobeBlendActive;` alongside other generator blend booleans. In `post_effect.cpp`: add `ArcStrobeEffectInit(&pe->arcStrobe, &pe->effects.arcStrobe)` call in `PostEffectInit` (after Filaments init, before next init — cascade cleanup on failure). Add `ArcStrobeRegisterParams(&pe->effects.arcStrobe)` in `PostEffectRegisterParams`. Add `ArcStrobeEffectUninit(&pe->arcStrobe)` in `PostEffectUninit`.

**Verify**: Compiles.

#### Task 2.5: Shader setup — generators module

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify)
**Depends on**: Wave 1

**Do**: In header: declare `void SetupArcStrobe(PostEffect *pe);` and `void SetupArcStrobeBlend(PostEffect *pe);`. In cpp: add `#include "effects/arc_strobe.h"`. Implement `SetupArcStrobe` delegating to `ArcStrobeEffectSetup(&pe->arcStrobe, &pe->effects.arcStrobe, pe->currentDeltaTime, pe->fftTexture)`. Implement `SetupArcStrobeBlend` calling `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.arcStrobe.blendIntensity, pe->effects.arcStrobe.blendMode)`. Add `TRANSFORM_ARC_STROBE_BLEND` case in `GetGeneratorScratchPass` returning `{pe->arcStrobe.shader, SetupArcStrobe}`.

**Verify**: Compiles.

#### Task 2.6: Shader dispatch and render pipeline

**Files**: `src/render/shader_setup.cpp` (modify), `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1

**Do**: In `shader_setup.cpp`: add `TRANSFORM_ARC_STROBE_BLEND` case in `GetTransformEffect` returning `{&pe->blendCompositor->shader, SetupArcStrobeBlend, &pe->arcStrobeBlendActive}` (follows Filaments blend pattern). In `render_pipeline.cpp`: add `pe->arcStrobeBlendActive = IsTransformEnabled(&pe->effects, TRANSFORM_ARC_STROBE_BLEND);` alongside other generator blend active assignments. Add `type == TRANSFORM_ARC_STROBE_BLEND` to `IsGeneratorBlendEffect()`.

**Verify**: Compiles.

#### Task 2.7: UI panel

**Files**: `src/ui/imgui_effects.cpp` (modify), `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1

**Do**: In `imgui_effects.cpp`: add `case TRANSFORM_ARC_STROBE_BLEND:` to `GetTransformCategory` returning `TRANSFORM_CATEGORY_GENERATORS`.

In `imgui_effects_generators.cpp`: add `#include "effects/arc_strobe.h"`. Add `static bool sectionArcStrobe = false;`. Create helpers `DrawArcStrobeParams` and `DrawArcStrobeOutput` following Filaments UI pattern. Sections: FFT (Octaves int slider, Segments/Octave int slider, Base Freq, Gain, Contrast modulatable), Shape (Orbit Offset, Line Thickness — modulatable), Lissajous (call `DrawLissajousControls(&cfg->lissajous, "arcstrobe", "arcStrobe.lissajous", modSources, 0.5f)` with `show3D = true` and `freqMin = 0.01f`), Glow (Glow Intensity, Falloff, Depth Near — modulatable), Strobe (Strobe Speed, Strobe Decay, Noise Strength — all modulatable), Color (gradient), Output (Blend Intensity, Blend Mode combo). Create `DrawGeneratorsArcStrobe` orchestrator. Add call in `DrawGeneratorsCategory` with spacing before Filaments.

**Verify**: Compiles.

#### Task 2.8: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for the extended `DualLissajousConfig` — add `amplitudeZ`, `freqZ1`, `freqZ2`, `offsetZ2` to the existing field list (if a macro already exists for `DualLissajousConfig`, extend it; if not, add one). Add macro for `ArcStrobeConfig` listing: `enabled`, `lissajous`, `orbitOffset`, `lineThickness`, `glowIntensity`, `falloffExponent`, `depthNear`, `strobeSpeed`, `strobeDecay`, `noiseStrength`, `baseFreq`, `numOctaves`, `segmentsPerOctave`, `gain`, `curve`, `baseBright`, `gradient`, `blendMode`, `blendIntensity`. Do NOT include `strobeTime` — that lives on the Effect struct. Add `to_json` entry: `if (e.arcStrobe.enabled) { j["arcStrobe"] = e.arcStrobe; }`. Add `from_json` entry: `e.arcStrobe = j.value("arcStrobe", e.arcStrobe);`.

**Verify**: Compiles.

#### Task 2.9: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/arc_strobe.cpp` to `EFFECTS_SOURCES` list.

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Implementation Notes

Major deviations from the original plan, documented for reviewers.

### Renamed from Spark Web

Originally designed as "Spark Web." Renamed to "Arc Strobe" after the strobe behavior became the dominant visual identity. All identifiers updated: files, structs, enums, shader, preset keys, UI labels.

### 3D Z-axis dropped

`DualLissajousConfig` was NOT extended with Z-axis fields. The effect stays 2D — `amplitudeZ`, `freqZ1`, `freqZ2`, `offsetZ2` do not exist. The `depthNear` parameter and all depth-related rendering were removed. Corresponding uniform locations (`amplitudeZLoc`, `freqZ1Loc`, etc.) do not exist in `ArcStrobeEffect`.

### Glow formula replaced

The planned glow formula `glowIntensity / (pow(dist, 1/falloffExponent) + EPSILON)` caused screen-filling diffuse haze from 120+ overlapping segments. Replaced with a Lorentzian profile:

```
GLOW_WIDTH = 0.002  (shader constant)
glow = GLOW_WIDTH * GLOW_WIDTH / (GLOW_WIDTH * GLOW_WIDTH + d * d)
```

This gives a tight 1/d² falloff tail that prevents background accumulation. `glowIntensity` became a pure brightness multiplier (default 2.0, range 0.5–10.0) applied after the glow calculation. `falloffExponent` was removed entirely.

### Strobe changed from multiplicative gate to additive accent

Plan specified `glow * strobeEnv * (baseBright + mag)` — strobe gates all brightness including FFT. This made FFT reactivity invisible between strobe flashes. Changed to additive:

```
brightness = baseBright + mag + strobeEnv * strobeBoost
```

`baseBright` is always-on ember (not gated behind strobe). FFT magnitude (`mag`) always visible. `strobeBoost` (new parameter, default 1.0, range 0.0–5.0) controls additive flash intensity on top.

### Strobe speed as uniform

`strobeSpeed` is passed to the shader as a uniform. When `strobeSpeed == 0`, the shader outputs uniform `strobeEnv = 1.0` instead of a static gradient from `fract(0 + i/N)`.

### Noise shimmer removed

`noiseStrength` parameter and the analytic grain modulation of strobe decay were not implemented. The strobe uses clean exponential decay only.

### Segment spacing changed

Plan originally used `t_i = fi` (1 radian steps), which scattered consecutive segments across the figure via wrapping. Changed to even spacing over one lap:

```
t_i = TWO_PI * i / totalSegments
```

This produces a continuous path where the strobe creates a visually coherent traveling arc.

### Default changes

| Parameter | Planned | Actual |
|-----------|---------|--------|
| segmentsPerOctave | 12 | 24 |
| glowIntensity | 0.01 | 2.0 |
| lineThickness | 0.005 | 0.01 |
| lissajous.amplitude | 0.3 | 0.5 |
| lissajous.freqX1 | 0.05 | 2.0 |
| lissajous.freqY1 | 0.08 | 3.0 |
| strobeSpeed | 1.0 | 0.3 |
| strobeSpeed max | 10.0 | 25.0 |
| orbitOffset | 1.0 | 2.0 |

### Aspect ratio correction

Lissajous x output is scaled by `resolution.x / resolution.y` so the figure fills the viewport instead of being confined to a square.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `TRANSFORM_ARC_STROBE_BLEND` appears in transform order pipeline
- [ ] Effect shows "Generators" category badge (not "???")
- [ ] Enabling effect adds it to pipeline list
- [ ] Strobe sweep visibly rotates through segments
- [ ] FFT energy drives segment brightness per semitone
- [ ] Gradient LUT colors segments by pitch class
- [ ] Lissajous shape params change knot geometry (including Z depth)
- [ ] Existing effects using DualLissajousConfig unchanged
- [ ] DrawLissajousControls shows Z controls only when show3D=true
- [ ] Modulatable params (including offsets) respond to LFOs/audio routing
- [ ] Preset save/load preserves all settings (including nested lissajous)
- [ ] Blend mode and intensity composite correctly
