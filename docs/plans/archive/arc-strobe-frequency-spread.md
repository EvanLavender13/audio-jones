# Arc Strobe Frequency Spread Migration

Migrate arc strobe from the old `numOctaves * segmentsPerOctave` semitone mapping to the new frequency-spread pattern where `layers` subdivide `baseFreq → maxFreq` in log space with band averaging.

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Config Changes

Remove:
- `int numOctaves = 5`
- `int segmentsPerOctave = 24`

Add:
- `int layers = 24` — visual density, range 4-96, UI in Shape section
- `float maxFreq = 14000.0f` — ceiling frequency Hz, range 1000-16000, UI in Audio section

### Shader Algorithm

Replace the old semitone lookup loop with frequency-spread band averaging:

```glsl
// Old
int totalSegments = segmentsPerOctave * numOctaves;

// New
// `layers` is the uniform — totalSegments == layers
int totalSegments = layers;
```

FFT lookup per segment — replace the old single-point semitone lookup:

```glsl
// Old
float semitoneF = fi * 12.0 / float(segmentsPerOctave);
int semitone = int(floor(semitoneF));
float freq = baseFreq * pow(2.0, float(semitone) / 12.0);
float bin = freq / (sampleRate * 0.5);
float mag = 0.0;
if (bin <= 1.0) {
    mag = texture(fftTexture, vec2(bin, 0.5)).r;
    mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
}

// New — band averaging (4 samples per band)
float t0 = float(i) / float(totalSegments);
float t1 = float(i + 1) / float(totalSegments);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float energy = 0.0;
const int BAND_SAMPLES = 4;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) {
        energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
}
energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
```

Color mapping — replace pitch-class with normalized position:

```glsl
// Old
vec3 color = texture(gradientLUT, vec2(fract(semitoneF / 12.0), 0.5)).rgb;

// New
vec3 color = texture(gradientLUT, vec2(t0, 0.5)).rgb;
```

Strobe sweep position — replace per-totalSegments fraction (unchanged formula, just uses new totalSegments):

```glsl
float sc = fi / float(totalSegments);  // unchanged
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| layers | int | 4-96 | 24 | No | "Layers" |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | "Max Freq (Hz)" |

### Uniform Changes

Remove: `numOctaves` (int), `segmentsPerOctave` (int)
Add: `layers` (int), `maxFreq` (float)

### UI Layout

Audio section slider order becomes: Base Freq, Max Freq, Gain, Contrast, Base Bright
- Remove `Octaves` and `Segments/Octave` sliders
- Add `Max Freq (Hz)` modulatable slider after Base Freq
- Move `Layers` int slider to Shape section (before Stride)

### Preset Compatibility

Old `numOctaves` and `segmentsPerOctave` fields are silently ignored on load (nlohmann `WITH_DEFAULT` handles this). New `layers` and `maxFreq` get their defaults.

---

## Tasks

### Wave 1: Config

#### Task 1.1: Update ArcStrobeConfig

**Files**: `src/effects/arc_strobe.h`

**Do**:
- Remove `numOctaves` and `segmentsPerOctave` fields
- Add `int layers = 24` with range comment `(4-96)`
- Add `float maxFreq = 14000.0f` with range comment `(1000-16000)`
- Update `ARC_STROBE_CONFIG_FIELDS` macro: replace `numOctaves, segmentsPerOctave` with `layers, maxFreq`
- In `ArcStrobeEffect` struct: replace `numOctavesLoc` and `segmentsPerOctaveLoc` with `layersLoc` and `maxFreqLoc`

**Verify**: `cmake.exe --build build` compiles (will have errors in .cpp — that's expected, wave 2 fixes them).

---

### Wave 2: Implementation (all files independent)

#### Task 2.1: Update arc_strobe.cpp

**Files**: `src/effects/arc_strobe.cpp`
**Depends on**: Wave 1

**Do**:
- In `ArcStrobeEffectInit`: replace `GetShaderLocation` calls for `numOctaves`/`segmentsPerOctave` with `layers`/`maxFreq`
- In `ArcStrobeEffectSetup`: replace `SetShaderValue` calls for `numOctaves`/`segmentsPerOctave` with `layers`/`maxFreq`
- In `ArcStrobeRegisterParams`: add `ModEngineRegisterParam("arcStrobe.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)` — `layers` is int, not modulatable

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Update shader

**Files**: `shaders/arc_strobe.fs`
**Depends on**: Wave 1

**Do**:
- Replace uniforms: remove `numOctaves`, `segmentsPerOctave`; add `int layers`, `float maxFreq`
- Replace `totalSegments` calculation: `int totalSegments = layers;`
- Replace FFT lookup with band-averaging algorithm from Design section
- Replace color mapping with `t0`-based gradient lookup from Design section
- Keep strobe logic unchanged (it already uses `fi / fTotal` which maps to `fi / float(totalSegments)`)

**Verify**: No build needed (runtime shader). Visual check: segments should respond across full spectrum.

#### Task 2.3: Update UI

**Files**: `src/ui/imgui_effects_gen_geometric.cpp`
**Depends on**: Wave 1

**Do**:
- In `DrawArcStrobeParams`, Audio section:
  - Remove `Octaves` and `Segments/Octave` sliders
  - Reorder to: Base Freq, Max Freq, Gain, Contrast, Base Bright
  - Add `ModulatableSlider("Max Freq (Hz)##arcstrobe", &cfg->maxFreq, "arcStrobe.maxFreq", "%.0f", modSources)` after Base Freq
- In Shape section: add `ImGui::SliderInt("Layers##arcstrobe", &cfg->layers, 4, 96)` before Stride

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Arc strobe responds to full frequency spectrum (bass through treble)
- [ ] Changing `layers` adjusts visual density without affecting frequency coverage
- [ ] Old presets load without crash (missing fields get defaults)
