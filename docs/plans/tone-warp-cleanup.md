# Tone Warp Cleanup

Migrate Tone Warp from `numOctaves`-based frequency mapping to the standard `baseFreq`/`maxFreq` frequency-spread pattern. Replace `numOctaves` with `maxFreq`, keep all other audio params (`baseFreq`, `gain`, `curve`, `baseBright`) which already match generator conventions. Split UI into Audio and Warp sections following standard Audio slider order.

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Config Changes

Remove: `numOctaves`

Add:

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| maxFreq | float | 1000.0 - 16000.0 | 14000.0 | yes | Max Freq (Hz) |

Keep unchanged:

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| enabled | bool | — | false | no | Enabled |
| intensity | float | 0.0 - 1.0 | 0.1 | yes | Intensity |
| baseFreq | float | 27.5 - 440.0 | 55.0 | yes | Base Freq (Hz) |
| gain | float | 0.1 - 10.0 | 2.0 | yes | Gain |
| curve | float | 0.1 - 3.0 | 0.7 | yes | Contrast |
| baseBright | float | 0.0 - 1.0 | 0.0 | yes | Base Bright |
| maxRadius | float | 0.1 - 1.0 | 0.7 | yes | Max Radius |
| segments | int | 1 - 16 | 4 | no | Segments |
| pushPullBalance | float | 0.0 - 1.0 | 0.5 | yes | Balance |
| pushPullSmoothness | float | 0.0 - 1.0 | 0.0 | yes | Smoothness |
| phaseSpeed | float | ±ROTATION_SPEED_MAX | 0.0 | yes | Phase Speed |

### Effect Struct Changes

Remove uniform loc: `numOctavesLoc`

Add uniform loc: `maxFreqLoc`

All other locs unchanged (`sampleRateLoc`, `baseFreqLoc`, `gainLoc`, `curveLoc`, `baseBrightLoc`, etc.).

### Shader Algorithm

Replace the `numOctaves` exponent with the frequency-spread log interpolation. One line changes in the frequency mapping:

```glsl
// Current:  freq = baseFreq * pow(2.0, t * float(numOctaves));
// New:      freq = baseFreq * pow(maxFreq / baseFreq, t);
```

Full context (only the frequency mapping changes, everything else stays):

```glsl
float t = clamp(radius / maxRadius, 0.0, 1.0);
float freq = baseFreq * pow(maxFreq / baseFreq, t);  // log space: baseFreq -> maxFreq
float bin = freq / (sampleRate * 0.5);

float magnitude = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
magnitude = clamp(magnitude * gain, 0.0, 1.0);
magnitude = pow(magnitude, curve);
magnitude = max(magnitude, baseBright);
```

Angular push/pull logic stays unchanged from current.

### UI Sections

**Audio** section (standard generator order): Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Base Bright

**Warp** section: Intensity, Max Radius, Segments, Balance, Smoothness, Phase Speed

### Serialization

Update `TONE_WARP_CONFIG_FIELDS` macro — replace `numOctaves` with `maxFreq`. Old preset `numOctaves` field silently ignored on load.

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Update ToneWarpConfig and ToneWarpEffect

**Files**: `src/effects/tone_warp.h`
**Creates**: Updated config struct and effect struct that Wave 2 depends on

**Do**:
- Replace `int numOctaves = 5` with `float maxFreq = 14000.0f; // Frequency ceiling Hz (1000.0 - 16000.0)`
- Update `TONE_WARP_CONFIG_FIELDS` macro — replace `numOctaves` with `maxFreq`
- In `ToneWarpEffect`: replace `int numOctavesLoc` with `int maxFreqLoc`

**Verify**: `cmake.exe --build build` compiles (will have linker errors until Wave 2).

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Update tone_warp.cpp

**Files**: `src/effects/tone_warp.cpp`
**Depends on**: Wave 1

**Do**:
- `Init`: replace `numOctavesLoc = GetShaderLocation(e->shader, "numOctaves")` with `maxFreqLoc = GetShaderLocation(e->shader, "maxFreq")`
- `Setup`: replace `SetShaderValue(e->shader, e->numOctavesLoc, &cfg->numOctaves, SHADER_UNIFORM_INT)` with `SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT)`
- `RegisterParams`: add `ModEngineRegisterParam("toneWarp.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

**Verify**: Compiles.

#### Task 2.2: Update shader frequency mapping

**Files**: `shaders/tone_warp.fs`
**Depends on**: Wave 1

**Do**:
- Replace `uniform int numOctaves;` with `uniform float maxFreq;`
- Replace frequency calc: `baseFreq * pow(2.0, t * float(numOctaves))` → `baseFreq * pow(maxFreq / baseFreq, t)`
- Everything else unchanged

**Verify**: Shader loads at runtime (no build step needed).

#### Task 2.3: Update UI with Audio/Warp sections

**Files**: `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 1

**Do**:
- In `DrawWarpToneWarp`, split into two sections:
  - `ImGui::SeparatorText("Audio")`: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Base Bright — standard generator Audio slider order
  - `ImGui::SeparatorText("Warp")`: Intensity, Max Radius, Segments, Balance, Smoothness, Phase Speed
- Replace `ImGui::SliderInt("Octaves##tonewarp", &e->toneWarp.numOctaves, 1, 8)` with `ModulatableSlider("Max Freq (Hz)##tonewarp", &e->toneWarp.maxFreq, "toneWarp.maxFreq", "%.0f", modSources)`
- `baseFreq` slider uses `"%.1f"` format, `maxFreq` uses `"%.0f"`, `gain` uses `"%.1f"`, `curve`/`baseBright` use `"%.2f"`

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect enables and displaces image reactively to audio
- [ ] Base Freq / Max Freq control which part of the spectrum drives displacement
- [ ] Gain amplifies FFT response
- [ ] Contrast (curve) >1 makes response punchier
- [ ] Warp section params (segments, balance, smoothness, phase speed) work as before
