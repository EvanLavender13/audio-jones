# Spectral Arcs Frequency-Spread Migration

Migrate spectral arcs from the old `numOctaves * 12` semitone loop to the frequency-spread pattern where `rings` subdivide `baseFreq → maxFreq` in log space. Ring count becomes purely visual density; frequency coverage is always the full spectrum.

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Types

**SpectralArcsConfig** — remove `int numOctaves`, add:

| Field | Type | Default | Range | Notes |
|-------|------|---------|-------|-------|
| `rings` | `int` | `24` | 4–96 | Visual density (Ring Layout section, not Audio) |
| `maxFreq` | `float` | `14000.0f` | 1000–16000 | Ceiling frequency (Audio section) |

**SpectralArcsEffect** — remove `numOctavesLoc`, add:

| Field | Type |
|-------|------|
| `ringsLoc` | `int` |
| `maxFreqLoc` | `int` |

**CONFIG_FIELDS macro** — replace `numOctaves` with `rings, maxFreq`.

### Shader Changes

**Loop**: Replace `int totalRings = numOctaves * 12` with `int totalRings = rings`.

**Frequency calc** (old):
```glsl
float freq = baseFreq * pow(2.0, float(i) / 12.0);
```
New:
```glsl
float freq = baseFreq * pow(maxFreq / baseFreq, float(i) / float(rings - 1));
```

**Color mapping** (old):
```glsl
float pitchClass = fract(float(i) / 12.0);
vec3 color = texture(gradientLUT, vec2(pitchClass, 0.5)).rgb;
```
New:
```glsl
float t = float(i) / float(rings);
vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
```

**Uniforms**: Replace `uniform int numOctaves` with `uniform int rings` and add `uniform float maxFreq`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label | Section |
|-----------|------|-------|---------|-------------|----------|---------|
| `rings` | `int` | 4–96 | 24 | No | `"Rings"` | Ring Layout |
| `maxFreq` | `float` | 1000–16000 | 14000.0 | Yes | `"Max Freq (Hz)"` | Audio |

### Modulation

- Remove: nothing (`numOctaves` was not modulatable)
- Add: `ModEngineRegisterParam("spectralArcs.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

### Preset Compatibility

Old `numOctaves` field silently ignored on load (nlohmann `WITH_DEFAULT` ignores unknown fields). New `rings` and `maxFreq` get defaults when absent.

---

## Tasks

### Wave 1: Config & Shader (no file overlap)

#### Task 1.1: Update config header

**Files**: `src/effects/spectral_arcs.h`

**Do**:
- Remove `int numOctaves = 5` field
- Add `int rings = 24` with range comment `(4-96)`
- Add `float maxFreq = 14000.0f` with range comment `(1000-16000)` — place after `baseFreq`
- In `SpectralArcsEffect`, remove `numOctavesLoc`, add `ringsLoc` and `maxFreqLoc`
- Update `SPECTRAL_ARCS_CONFIG_FIELDS` macro: replace `numOctaves` with `rings, maxFreq`

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Update shader

**Files**: `shaders/spectral_arcs.fs`

**Do**:
- Replace `uniform int numOctaves` with `uniform int rings` and add `uniform float maxFreq`
- Replace `int totalRings = numOctaves * 12` with `int totalRings = rings`
- Replace `float ft = float(totalRings)` — keep as-is (it's just `float(rings)` now)
- Replace frequency calc: `float freq = baseFreq * pow(maxFreq / baseFreq, float(i) / float(rings - 1));`
- Replace color mapping: `float t = float(i) / float(rings);` then `texture(gradientLUT, vec2(t, 0.5)).rgb`

**Verify**: Shader syntax correct (no standalone verify — builds with Task 1.3).

#### Task 1.3: Update C++ module

**Files**: `src/effects/spectral_arcs.cpp`

**Do**:
- In `Init`: replace `GetShaderLocation(e->shader, "numOctaves")` with `GetShaderLocation(e->shader, "rings")` and add `GetShaderLocation(e->shader, "maxFreq")`
- In `Setup`: replace `SetShaderValue` for `numOctavesLoc`/`numOctaves` with `ringsLoc`/`rings` (type `SHADER_UNIFORM_INT`) and add `maxFreqLoc`/`maxFreq` (type `SHADER_UNIFORM_FLOAT`)
- In `RegisterParams`: add `ModEngineRegisterParam("spectralArcs.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.4: Update UI

**Files**: `src/ui/imgui_effects_gen_geometric.cpp`

**Do**:
- In `DrawSpectralArcsParams`, Audio section:
  - Remove `ImGui::SliderInt("Octaves##spectralarcs", &sa->numOctaves, 1, 8)`
  - Add `ModulatableSlider("Max Freq (Hz)##spectralarcs", &sa->maxFreq, "spectralArcs.maxFreq", "%.0f", modSources)` — place after Base Freq, before Gain (standard Audio slider order)
- In Ring Layout section:
  - Add `ImGui::SliderInt("Rings##spectralarcs", &sa->rings, 4, 96)` as the first slider

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Spectral arcs renders with `rings=24` covering full `baseFreq → maxFreq` spectrum
- [ ] Changing `rings` adjusts visual density without affecting frequency coverage
- [ ] Changing `maxFreq` adjusts frequency ceiling without affecting ring count
- [ ] Gradient color smoothly maps low-to-high frequency across all rings
- [ ] Old presets load without errors (missing `rings`/`maxFreq` get defaults)
