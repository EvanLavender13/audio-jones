# Tone Warp Cleanup

Migrate Tone Warp from `numOctaves`-based frequency mapping to the standard `baseFreq`/`maxFreq` frequency-spread pattern. Replace `numOctaves` with `maxFreq`, replace `baseBright` with `bassBoost` (center-weighted displacement from bass energy). Keep remaining audio params (`baseFreq`, `gain`, `curve`). Split UI into Audio and Warp sections.

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Config Changes

Remove: `numOctaves`, `baseBright`

Add:

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| maxFreq | float | 1000.0 - 16000.0 | 14000.0 | yes | Max Freq (Hz) |
| bassBoost | float | 0.0 - 2.0 | 0.0 | yes | Bass Boost |

Keep unchanged:

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| enabled | bool | — | false | no | Enabled |
| intensity | float | 0.0 - 1.0 | 0.1 | yes | Intensity |
| baseFreq | float | 27.5 - 440.0 | 55.0 | yes | Base Freq (Hz) |
| gain | float | 0.1 - 10.0 | 2.0 | yes | Gain |
| curve | float | 0.1 - 3.0 | 0.7 | yes | Contrast |
| maxRadius | float | 0.1 - 1.0 | 0.7 | yes | Max Radius |
| segments | int | 1 - 16 | 4 | no | Segments |
| pushPullBalance | float | 0.0 - 1.0 | 0.5 | yes | Balance |
| pushPullSmoothness | float | 0.0 - 1.0 | 0.0 | yes | Smoothness |
| phaseSpeed | float | ±ROTATION_SPEED_MAX | 0.0 | yes | Phase Speed |

### Effect Struct Changes

Remove uniform locs: `numOctavesLoc`, `baseBrightLoc`

Add uniform locs: `maxFreqLoc`, `bassBoostLoc`

All other locs unchanged.

### Shader Algorithm

Replace `numOctaves` exponent with frequency-spread log interpolation. Replace `baseBright` floor with `bassBoost` center-weighted displacement.

```glsl
float t = clamp(radius / maxRadius, 0.0, 1.0);
float freq = baseFreq * pow(maxFreq / baseFreq, t);  // log space: baseFreq -> maxFreq
float bin = freq / (sampleRate * 0.5);

float magnitude = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
magnitude = clamp(magnitude * gain, 0.0, 1.0);
magnitude = pow(magnitude, curve);

// Bass boost: extra center-weighted displacement from bass energy
float bassBin = baseFreq / (sampleRate * 0.5);
float bassEnergy = texture(fftTexture, vec2(bassBin, 0.5)).r;
float centerWeight = pow(1.0 - t, 2.0);
magnitude += bassBoost * bassEnergy * centerWeight;
```

Angular push/pull logic stays unchanged from current.

### UI Sections

**Audio** section: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Bass Boost

**Warp** section: Intensity, Max Radius, Segments, Balance, Smoothness, Phase Speed

### Serialization

Update `TONE_WARP_CONFIG_FIELDS` macro — replace `numOctaves` with `maxFreq`, `baseBright` with `bassBoost`. Old preset fields silently ignored on load.

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Update ToneWarpConfig and ToneWarpEffect

**Files**: `src/effects/tone_warp.h`
**Creates**: Updated config struct and effect struct that Wave 2 depends on

**Do**:
- Replace `int numOctaves = 5` with `float maxFreq = 14000.0f`
- Replace `float baseBright = 0.0f` with `float bassBoost = 0.0f` (range 0.0-2.0)
- Update `TONE_WARP_CONFIG_FIELDS` macro
- In `ToneWarpEffect`: replace `numOctavesLoc` with `maxFreqLoc`, `baseBrightLoc` with `bassBoostLoc`

**Verify**: `cmake.exe --build build` compiles (will have linker errors until Wave 2).

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Update tone_warp.cpp

**Files**: `src/effects/tone_warp.cpp`
**Depends on**: Wave 1

**Do**:
- `Init`: replace uniform location caching for `numOctaves`→`maxFreq`, `baseBright`→`bassBoost`
- `Setup`: replace uniform binding — `numOctaves` (INT) → `maxFreq` (FLOAT), `baseBright` → `bassBoost`
- `RegisterParams`: replace `baseBright` (0-1) with `bassBoost` (0-2), add `maxFreq` (1000-16000)

**Verify**: Compiles.

#### Task 2.2: Update shader frequency mapping

**Files**: `shaders/tone_warp.fs`
**Depends on**: Wave 1

**Do**:
- Replace `uniform int numOctaves` with `uniform float maxFreq`
- Replace `uniform float baseBright` with `uniform float bassBoost`
- Replace frequency calc with log-space interpolation
- Replace `max(magnitude, baseBright)` with bass boost center-weighted displacement (see Algorithm)

**Verify**: Shader loads at runtime.

#### Task 2.3: Update UI with Audio/Warp sections

**Files**: `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 1

**Do**:
- Split into Audio and Warp sections
- Audio: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Bass Boost
- Warp: Intensity, Max Radius, Segments, Balance, Smoothness, Phase Speed
- Replace Octaves SliderInt with Max Freq ModulatableSlider
- Replace Base Bright with Bass Boost

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect enables and displaces image reactively to audio
- [ ] Base Freq / Max Freq control which part of the spectrum drives displacement
- [ ] Bass Boost adds extra center displacement from bass energy
- [ ] Gain amplifies FFT response, Contrast >1 makes response punchier
- [ ] Warp section params (segments, balance, smoothness, phase speed) work as before
