# Kaleidoscope Enhancements

Add three configurable radial effects to the kaleidoscope shader: radial twist (rotation varies by distance from center), moving focal point (Lissajous-animated center), and domain warping (fBM noise displacement). Create a new collapsible Kaleidoscope section with UI controls.

## Current State

- `shaders/kaleidoscope.fs` - Polar mirroring with 4x supersampling, receives `segments` and `rotation` uniforms
- `src/config/effect_config.h:22,25` - `kaleidoRotationSpeed` and `kaleidoSegments` fields (standalone, not nested)
- `src/render/post_effect.h:35-36` - Two uniform locations (`kaleidoSegmentsLoc`, `kaleidoRotationLoc`)
- `src/render/render_pipeline.cpp:103-109` - `SetupKaleido()` uploads uniforms
- `src/ui/imgui_effects.cpp:29-30` - Inline SliderInt and SliderAngleDeg controls

---

## Phase 1: Config Migration

**Goal**: Create `KaleidoscopeConfig` struct and migrate existing fields.

**Files**:
- Create `src/config/kaleidoscope_config.h` with struct containing:
  - `int segments = 1` (migrated)
  - `float rotationSpeed = 0.002f` (migrated)
  - `float twistAmount = 0.0f` (new, radians, 0 = disabled)
  - `float focalAmplitude = 0.0f` (new, UV units, 0 = disabled)
  - `float focalFreqX = 1.0f` (new, Lissajous X frequency)
  - `float focalFreqY = 1.5f` (new, Lissajous Y frequency)
  - `float warpStrength = 0.0f` (new, 0 = disabled)
  - `float noiseScale = 2.0f` (new, fBM spatial scale)
- Modify `src/config/effect_config.h`:
  - Add `#include "kaleidoscope_config.h"`
  - Remove `kaleidoRotationSpeed` and `kaleidoSegments`
  - Add `KaleidoscopeConfig kaleidoscope;`
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KaleidoscopeConfig, ...)`
  - Update EffectConfig macro: replace `kaleidoRotationSpeed, kaleidoSegments` with `kaleidoscope`
- Search/replace in all C++ files:
  - `effects.kaleidoSegments` → `effects.kaleidoscope.segments`
  - `effects.kaleidoRotationSpeed` → `effects.kaleidoscope.rotationSpeed`

**Done when**: Project compiles and loads existing presets with defaults for new params.

---

## Phase 2: Shader Extension

**Goal**: Add fBM noise and new effect logic to kaleidoscope shader.

**Files**:
- Modify `shaders/kaleidoscope.fs`:
  - Add uniforms: `time`, `twist`, `focalOffset` (vec2), `warpStrength`, `noiseScale`
  - Add hash and 2D gradient noise functions (8-10 lines)
  - Add fBM function: 4 octaves, lacunarity=2.0, gain=0.5
  - In `main()`:
    1. Offset UV by `focalOffset` after centering
    2. Convert to polar
    3. Apply radial twist: `angle += twist * (1.0 - radius)` (inner rotates more)
    4. If `warpStrength > 0`: convert back to Cartesian, apply fBM warp, reconvert to polar
    5. Existing segment mirroring and supersampling

**Done when**: Shader compiles. Effects disabled by default (uniforms = 0).

---

## Phase 3: Uniform Plumbing

**Goal**: Wire new shader uniforms through render pipeline.

**Files**:
- Modify `src/render/post_effect.h`:
  - Add fields: `kaleidoTimeLoc`, `kaleidoTwistLoc`, `kaleidoFocalLoc`, `kaleidoWarpStrengthLoc`, `kaleidoNoiseScaleLoc`
- Modify `src/render/post_effect.cpp`:
  - Get locations for all 5 new uniforms in `GetShaderUniformLocations()`
- Modify `src/render/render_pipeline.cpp`:
  - In `RenderPipelineApplyOutput()`: compute Lissajous focal position from `globalTick`:
    ```
    float t = (float)globalTick * 0.016f;
    vec2 focal = { amplitude * sinf(t * freqX), amplitude * cosf(t * freqY) };
    ```
  - Store computed values in PostEffect temporaries
  - In `SetupKaleido()`: upload all new uniforms

**Done when**: All uniforms reach shader (verify with RenderDoc or log statements).

---

## Phase 4: UI Section

**Goal**: Create collapsible Kaleidoscope section with all controls.

**Files**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add `static bool sectionKaleidoscope = false;`
  - Remove inline Kaleido sliders (lines 29-30)
  - Add collapsible section after Core Effects:
    ```
    Kaleidoscope (Theme::GLOW_CYAN)
    ├── Segments       (SliderInt, 1-12)
    ├── Spin           (SliderAngleDeg, -0.6 to 0.6)
    ├── Twist          (SliderFloat, -2.0 to 2.0)
    ├── Focal Amp      (SliderFloat, 0.0 to 0.2)
    ├── Focal Freq X   (SliderFloat, 0.1 to 5.0)
    ├── Focal Freq Y   (SliderFloat, 0.1 to 5.0)
    ├── Warp           (SliderFloat, 0.0 to 0.5)
    └── Noise Scale    (SliderFloat, 0.5 to 10.0)
    ```

**Done when**: All sliders visible and control shader uniforms in real-time.

---

## Phase 5: Preset Migration

**Goal**: Update existing preset files to new JSON structure.

**Files**:
- All files in `presets/*.json`
- For each preset:
  - Replace top-level `kaleidoRotationSpeed` and `kaleidoSegments` with nested:
    ```json
    "kaleidoscope": {
      "segments": <old kaleidoSegments value>,
      "rotationSpeed": <old kaleidoRotationSpeed value>
    }
    ```
  - New fields use defaults (omit from JSON or include with default values)

**Done when**: All presets load correctly with migrated values.

---

## Testing Checklist

- [ ] Old presets load without error (missing `kaleidoscope` object uses defaults)
- [ ] Each effect works in isolation (twist only, warp only, focal only)
- [ ] Combined effects produce expected spiral/organic kaleidoscope
- [ ] focalFreqX ≠ focalFreqY creates Lissajous patterns
- [ ] Segment count and rotation speed still work as before
- [ ] Performance acceptable at 1080p (fBM overhead < 0.5ms)
