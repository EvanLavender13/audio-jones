# Motherboard Frequency-Spread Migration

Migrate motherboard from `numOctaves`-based semitone mapping to the `baseFreq → maxFreq` log-space spread pattern. Each fold iteration gets a frequency evenly spaced in log space across the full spectrum; `iterations` serves as the visual density control (no separate `layers` param needed).

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Config Changes

Remove `int numOctaves` (was default 5, range 1-8).

Add `float maxFreq = 14000.0f` (range 1000-16000).

All other fields unchanged.

### Shader Frequency Calc

Replace the current pitch-class + octave-center mapping:

```glsl
// OLD
int note = i % 12;
float octaveCenter = float(numOctaves - 1) * float(i) / max(float(iterations - 1), 1.0);
float freq = baseFreq * pow(2.0, float(note) / 12.0 + octaveCenter);
```

With log-space spread:

```glsl
// NEW
float freq = baseFreq * pow(maxFreq / baseFreq, float(i) / max(float(iterations - 1), 1.0));
```

### Shader Color Mapping

Replace cyclic pitch-class coloring:

```glsl
// OLD
vec3 layerColor = texture(gradientLUT, vec2(fract(float(i) / 12.0), 0.5)).rgb;
```

With smooth normalized ramp:

```glsl
// NEW
vec3 layerColor = texture(gradientLUT, vec2(float(i) / float(iterations), 0.5)).rgb;
```

### Shader Uniform Changes

- Remove: `uniform int numOctaves`
- Add: `uniform float maxFreq`

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| maxFreq | float | 1000-16000 | 14000.0 | yes | "Max Freq (Hz)" |

(Removed: `numOctaves` — not modulatable, no longer needed)

### UI Audio Section (new order)

1. Base Freq (Hz) — ModulatableSlider, "%.1f"
2. Max Freq (Hz) — ModulatableSlider, "%.0f"
3. Gain — ModulatableSlider, "%.1f"
4. Contrast — ModulatableSlider, "%.2f"
5. Base Bright — ModulatableSlider, "%.2f"

(Removed: `Octaves` SliderInt)

---

## Tasks

### Wave 1: Config

#### Task 1.1: Update MotherboardConfig

**Files**: `src/effects/motherboard.h`

**Do**:
- Remove `int numOctaves = 5;` field and its range comment
- Add `float maxFreq = 14000.0f; // Highest visible frequency (Hz) (1000-16000)` after `baseFreq`
- Update `MOTHERBOARD_CONFIG_FIELDS` macro: replace `numOctaves` with `maxFreq`

**Verify**: `cmake.exe --build build` compiles (will have warnings about missing uniform — expected until shader is updated).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update Shader

**Files**: `shaders/motherboard.fs`
**Depends on**: Wave 1 complete

**Do**:
- Remove `uniform int numOctaves;`
- Add `uniform float maxFreq;` in its place
- Replace frequency calc and color mapping inside the loop as specified in the Design section above

**Verify**: Compiles.

#### Task 2.2: Update C++ Module

**Files**: `src/effects/motherboard.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `MotherboardEffect` struct usage within Init: replace `numOctavesLoc` with `maxFreqLoc` for `GetShaderLocation(e->shader, "maxFreq")`
- In Setup: replace `SetShaderValue` for `numOctaves` (int) with `SetShaderValue` for `maxFreq` (float)
- In RegisterParams: add `ModEngineRegisterParam("motherboard.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

Also update the header (`src/effects/motherboard.h`):
- In `MotherboardEffect` struct: replace `int numOctavesLoc;` with `int maxFreqLoc;`

**Verify**: Compiles.

#### Task 2.3: Update UI

**Files**: `src/ui/imgui_effects_gen_texture.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `DrawGeneratorsMotherboard`, Audio section: remove `ImGui::SliderInt("Octaves##motherboard", ...)` line
- Add `ModulatableSlider("Max Freq (Hz)##motherboard", &cfg->maxFreq, "motherboard.maxFreq", "%.0f", modSources);` after the Base Freq slider
- Final Audio slider order: Base Freq, Max Freq, Gain, Contrast, Base Bright

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Motherboard effect loads and renders
- [ ] Audio reactivity spans full spectrum with default settings
- [ ] Changing iterations adjusts visual density without affecting frequency range
- [ ] maxFreq slider controls the upper frequency bound
