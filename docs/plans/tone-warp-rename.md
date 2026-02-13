# Tone Warp (Rename + Convert FFT Radial Warp)

Rename FFT Radial Warp to Tone Warp and convert from raw FFT bin mapping to the standard semitone/octave audio technique with logarithmic frequency-to-radius mapping. Drop bassBoost. Add the standard 5-param Audio block.

## Design

### Rename Map

| Old | New |
|-----|-----|
| `fft_radial_warp.h` / `.cpp` / `.fs` | `tone_warp.h` / `.cpp` / `.fs` |
| `FftRadialWarpConfig` | `ToneWarpConfig` |
| `FftRadialWarpEffect` | `ToneWarpEffect` |
| `FftRadialWarp*()` functions | `ToneWarp*()` functions |
| `TRANSFORM_FFT_RADIAL_WARP` | `TRANSFORM_TONE_WARP` |
| `"FFT Radial Warp"` display name | `"Tone Warp"` |
| `e->fftRadialWarp` / `pe->fftRadialWarp` | `e->toneWarp` / `pe->toneWarp` |
| `"fftRadialWarp.*"` param IDs | `"toneWarp.*"` param IDs |
| JSON key `"fftRadialWarp"` | `"toneWarp"` |

### Config Struct: `ToneWarpConfig`

**Removed fields**: `freqStart`, `freqEnd`, `freqCurve`, `bassBoost`

**New standard audio fields**:

| Field | Type | Range | Default | UI Label | Format |
|-------|------|-------|---------|----------|--------|
| `numOctaves` | `float` | 1.0–8.0 | 5.0f | `"Octaves"` | ModulatableSliderInt |
| `baseFreq` | `float` | 27.5–440.0 | 55.0f | `"Base Freq (Hz)"` | `"%.1f"` |
| `gain` | `float` | 0.1–10.0 | 2.0f | `"Gain"` | `"%.1f"` |
| `curve` | `float` | 0.1–3.0 | 0.7f | `"Contrast"` | `"%.2f"` |
| `baseBright` | `float` | 0.0–1.0 | 0.0f | `"Base Bright"` | `"%.2f"` |

**Kept fields** (unchanged):

| Field | Type | Range | Default | Purpose |
|-------|------|-------|---------|---------|
| `enabled` | `bool` | — | false | — |
| `intensity` | `float` | 0.0–1.0 | 0.1f | Displacement strength |
| `maxRadius` | `float` | 0.1–1.0 | 0.7f | Screen radius at octave ceiling |
| `segments` | `int` | 1–16 | 4 | Angular push/pull divisions |
| `pushPullBalance` | `float` | 0.0–1.0 | 0.5f | Pull ↔ push bias |
| `pushPullSmoothness` | `float` | 0.0–1.0 | 0.0f | Hard ↔ smooth edges |
| `phaseSpeed` | `float` | — | 0.0f | Auto-rotate (rad/s) |

### Shader Algorithm: Logarithmic Octave Mapping

The current shader maps radius linearly to FFT bin range `[freqStart, freqEnd]`. The new shader maps radius logarithmically so each octave occupies equal radial space:

```
// Radius → frequency (logarithmic: each octave = equal screen space)
float t = clamp(radius / maxRadius, 0.0, 1.0);
float freq = baseFreq * pow(2.0, t * float(numOctaves));
float bin = freq / (sampleRate * 0.5);

// Sample FFT, apply gain + contrast curve
float magnitude = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
magnitude = clamp(magnitude * gain, 0.0, 1.0);
magnitude = pow(magnitude, curve);
magnitude = max(magnitude, baseBright);
```

Everything after magnitude sampling stays the same: angular wave, push/pull, displacement.

New uniforms: `sampleRate`, `baseFreq`, `numOctaves` (int), `gain`, `curve`, `baseBright`
Removed uniforms: `freqStart`, `freqEnd`, `freqCurve`, `bassBoost`

### Effect Struct: `ToneWarpEffect`

Same structure as `FftRadialWarpEffect` but with renamed/replaced uniform locations:
- Remove: `freqStartLoc`, `freqEndLoc`, `freqCurveLoc`, `bassBoostLoc`
- Add: `sampleRateLoc`, `baseFreqLoc`, `numOctavesLoc`, `gainLoc`, `curveLoc`, `baseBrightLoc`

### UI Layout

```
ImGui::SeparatorText("Audio");
  ModulatableSliderInt("Octaves##tonewarp", ...)
  ModulatableSlider("Base Freq (Hz)##tonewarp", ...)
  ModulatableSlider("Gain##tonewarp", ...)
  ModulatableSlider("Contrast##tonewarp", ...)
  ModulatableSlider("Base Bright##tonewarp", ...)
[existing warp-specific sliders: Intensity, Max Radius, Segments, Balance, Smoothness, Phase Speed]
```

### Files Touched

**Create (from old files):**
- `src/effects/tone_warp.h`
- `src/effects/tone_warp.cpp`
- `shaders/tone_warp.fs`

**Modify (rename references):**
- `src/config/effect_config.h` — enum value + config member
- `src/config/effect_descriptor.h` — descriptor row
- `src/render/post_effect.h` — include + effect member
- `src/render/post_effect.cpp` — init/uninit/register calls
- `src/render/shader_setup.cpp` — dispatcher case
- `src/render/shader_setup_warp.h` — function declaration
- `src/render/shader_setup_warp.cpp` — include + setup function
- `src/ui/imgui_effects_warp.cpp` — UI section
- `src/config/preset.cpp` — JSON macro + serialize/deserialize
- `CMakeLists.txt` — source file path
- `docs/effects.md` — effect entry

**Delete:**
- `src/effects/fft_radial_warp.h`
- `src/effects/fft_radial_warp.cpp`
- `shaders/fft_radial_warp.fs`

---

## Tasks

### Wave 1: New Effect Files

#### Task 1.1: Tone Warp Header + Source

**Files**: `src/effects/tone_warp.h` (create), `src/effects/tone_warp.cpp` (create)
**Creates**: ToneWarpConfig, ToneWarpEffect types and all public functions

**Do**:
- Use `src/effects/fft_radial_warp.h` and `.cpp` as the starting base
- Rename all types and functions per the Rename Map
- Apply the Config Struct changes from Design: remove freqStart/freqEnd/freqCurve/bassBoost fields, add numOctaves/baseFreq/gain/curve/baseBright with standard ranges and defaults
- Apply the Effect Struct changes: swap uniform location fields per Design
- In Init: load `"shaders/tone_warp.fs"`, cache new uniform locations (sampleRateLoc, baseFreqLoc, numOctavesLoc, gainLoc, curveLoc, baseBrightLoc), remove old ones
- In Setup: bind new uniforms. `numOctaves` as int cast (glyph field pattern: `int numOctavesInt = (int)cfg->numOctaves;`). Add `float sampleRate = (float)AUDIO_SAMPLE_RATE;` (include `audio/audio_config.h`). Remove freqStart/freqEnd/freqCurve/bassBoost bindings.
- In RegisterParams: register all 5 standard audio params with standard ranges, plus the kept effect-specific params (intensity, maxRadius, pushPullBalance, pushPullSmoothness, phaseSpeed). Remove freqStart/freqEnd/freqCurve/bassBoost registrations.

**Verify**: Files compile when included (header has no missing types).

---

#### Task 1.2: Tone Warp Shader

**Files**: `shaders/tone_warp.fs` (create)
**Creates**: New shader with logarithmic octave-based FFT sampling

**Do**:
- Use `shaders/fft_radial_warp.fs` as the starting base
- Replace uniforms: remove `freqStart`, `freqEnd`, `freqCurve`, `bassBoost`. Add `sampleRate` (float), `baseFreq` (float), `numOctaves` (int), `gain` (float), `curve` (float), `baseBright` (float).
- Replace the FFT sampling block (lines 28–40 of old shader) with the logarithmic mapping from the Algorithm section in Design
- Keep everything else unchanged: angular wave, push/pull balance/smoothness, displacement calculation, final output

**Verify**: Shader has valid GLSL syntax (uniform types match, no undefined variables).

---

### Wave 2: Integration Renames

All tasks depend on Wave 1 complete. No file overlap between tasks.

#### Task 2.1: Config + Descriptor Renames

**Files**: `src/config/effect_config.h`, `src/config/effect_descriptor.h`
**Depends on**: Wave 1 complete

**Do**:
- `effect_config.h`: Rename `TRANSFORM_FFT_RADIAL_WARP` → `TRANSFORM_TONE_WARP` in enum. Rename `FftRadialWarpConfig fftRadialWarp;` → `ToneWarpConfig toneWarp;` in EffectConfig struct. Update the include from `"effects/fft_radial_warp.h"` to `"effects/tone_warp.h"` if present (check — it may be included transitively).
- `effect_descriptor.h`: Update the descriptor row: enum `TRANSFORM_TONE_WARP`, display name `"Tone Warp"`, offsetof `toneWarp.enabled`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Render Pipeline Renames

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/render/shader_setup.cpp`, `src/render/shader_setup_warp.h`, `src/render/shader_setup_warp.cpp`
**Depends on**: Wave 1 complete

**Do**:
- `post_effect.h`: Change include to `"effects/tone_warp.h"`. Rename `FftRadialWarpEffect fftRadialWarp;` → `ToneWarpEffect toneWarp;`
- `post_effect.cpp`: Change include. Rename all `FftRadialWarp*` function calls → `ToneWarp*`. Rename `pe->fftRadialWarp` → `pe->toneWarp`. Rename `pe->effects.fftRadialWarp` → `pe->effects.toneWarp`.
- `shader_setup.cpp`: Rename `TRANSFORM_FFT_RADIAL_WARP` → `TRANSFORM_TONE_WARP`. Rename `pe->fftRadialWarp` → `pe->toneWarp`. Rename `SetupFftRadialWarp` → `SetupToneWarp`. Rename `pe->effects.fftRadialWarp` → `pe->effects.toneWarp`.
- `shader_setup_warp.h`: Rename function declaration `SetupFftRadialWarp` → `SetupToneWarp`
- `shader_setup_warp.cpp`: Change include. Rename function `SetupFftRadialWarp` → `SetupToneWarp`. Rename all `FftRadialWarp*` → `ToneWarp*`. Rename `pe->fftRadialWarp` → `pe->toneWarp`. Rename `pe->effects.fftRadialWarp` → `pe->effects.toneWarp`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: UI + Preset Renames

**Files**: `src/ui/imgui_effects_warp.cpp`, `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**:
- `imgui_effects_warp.cpp`:
  - Rename static `sectionFftRadialWarp` → `sectionToneWarp`
  - Rename function `DrawWarpFftRadialWarp` → `DrawWarpToneWarp`
  - Change section title from `"FFT Radial Warp"` → `"Tone Warp"`
  - Change checkbox label: `"Enabled##fftradialwarp"` → `"Enabled##tonewarp"`
  - Change `TRANSFORM_FFT_RADIAL_WARP` → `TRANSFORM_TONE_WARP`
  - Rename `e->fftRadialWarp` → `e->toneWarp` throughout
  - Replace the old frequency sliders (Freq Start, Freq End, Freq Curve, Bass Boost) with the standard Audio section per the UI Layout in Design: add `ImGui::SeparatorText("Audio")`, then the 5 standard sliders with `##tonewarp` tags and `"toneWarp.*"` param IDs
  - Keep Intensity, Max Radius, Segments, Balance, Smoothness, Phase Speed sliders (rename `##fftradialwarp` → `##tonewarp`, rename `"fftRadialWarp.*"` → `"toneWarp.*"` param IDs)
  - Update the function call site (search for `DrawWarpFftRadialWarp`)
- `preset.cpp`:
  - Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro: `FftRadialWarpConfig` → `ToneWarpConfig`, update field list (remove freqStart/freqEnd/freqCurve/bassBoost, add numOctaves/baseFreq/gain/curve/baseBright)
  - Rename JSON key: `"fftRadialWarp"` → `"toneWarp"` in both serialize and deserialize
  - Add backward compat: also check for `"fftRadialWarp"` key during deserialization (old presets gracefully ignored — fields won't match but won't crash)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Build + Docs + Cleanup

**Files**: `CMakeLists.txt`, `docs/effects.md`
**Depends on**: Wave 1 complete

**Do**:
- `CMakeLists.txt`: Replace `src/effects/fft_radial_warp.cpp` with `src/effects/tone_warp.cpp`
- `docs/effects.md`: In the Warp section, rename `"FFT Radial Warp"` → `"Tone Warp"` and update the description to reflect the new octave-based mapping: `"Audio-reactive breathing flower where bass pumps the center and treble ripples the edges, with alternating petals pushing in and out to the beat"` → `"Audio-reactive radial displacement where octave bands map logarithmically from center to edges, with alternating angular segments pushing and pulling to the beat"`
- Delete old files: `git rm src/effects/fft_radial_warp.h src/effects/fft_radial_warp.cpp shaders/fft_radial_warp.fs`

**Verify**: `cmake.exe --build build` compiles. `git status` shows old files deleted, new files added.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] No remaining references to `fftRadialWarp`, `FftRadialWarp`, `FFT_RADIAL_WARP`, or `fft_radial_warp` in src/ or shaders/
- [ ] Tone Warp appears in the transform list with correct "WARP" category badge
- [ ] UI shows "Tone Warp" section with "Audio" separator and 5 standard sliders
- [ ] Effect displaces pixels radially based on FFT — bass at center, treble at edges
- [ ] Each octave occupies equal radial screen space (logarithmic mapping)
- [ ] Segments, balance, smoothness, phase speed still function as before
