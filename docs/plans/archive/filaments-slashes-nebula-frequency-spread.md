# Filaments / Slashes / Nebula Frequency-Spread Migration

Migrate filaments, slashes, and nebula from the old `numOctaves * 12` semitone loop to the `maxFreq` frequency-spread standard with band-averaging. Each effect gets an appropriately named count parameter (`filaments`, `bars`, `starBins`) decoupling visual density from frequency coverage.

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Filaments

**Config changes** (`FilamentsConfig`):

| Remove | Add | Type | Default | Range | Section |
|--------|-----|------|---------|-------|---------|
| `int numOctaves = 5` | `int filaments = 60` | int | 60 | 4-256 | Geometry |
| | `float maxFreq = 14000.0f` | float | 14000.0 | 1000-16000 | Audio |

**Effect struct changes** (`FilamentsEffect`):

| Remove | Add |
|--------|-----|
| `int numOctavesLoc` | `int filamentsLoc` |
| | `int maxFreqLoc` |

**Shader migration** (`filaments.fs`):
- Replace `uniform int numOctaves` with `uniform int filaments` and `uniform float maxFreq`
- Replace `int totalFilaments = numOctaves * 12` with `int totalFilaments = filaments`
- Replace single-point semitone FFT lookup with band-averaging:

```glsl
// Old:
float freq = baseFreq * pow(2.0, float(i) / 12.0);
float bin = freq / (sampleRate * 0.5);
float mag = 0.0;
if (bin <= 1.0) {
    mag = texture(fftTexture, vec2(bin, 0.5)).r;
    mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
}

// New (band-averaging from research doc):
float t0 = float(i) / float(totalFilaments);
float t1 = float(i + 1) / float(totalFilaments);
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
float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
```

- Replace color mapping: `fract(float(i) / 12.0)` becomes `float(i) / float(totalFilaments)` (smooth gradient low-to-high)

**UI** (Audio section slider order): Base Freq, Max Freq, Gain, Contrast, Base Bright. `filaments` int slider in Geometry section.

**Modulation**: Add `ModEngineRegisterParam("filaments.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

---

### Slashes

**Config changes** (`SlashesConfig`):

| Remove | Add | Type | Default | Range | Section |
|--------|-----|------|---------|-------|---------|
| `int numOctaves = 5` | `int bars = 60` | int | 60 | 4-256 | Geometry |
| | `float maxFreq = 14000.0f` | float | 14000.0 | 1000-16000 | Audio |

**Effect struct changes** (`SlashesEffect`):

| Remove | Add |
|--------|-----|
| `int numOctavesLoc` | `int barsLoc` |
| | `int maxFreqLoc` |

**Shader migration** (`slashes.fs`):
- Replace `uniform int numOctaves` with `uniform int bars` and `uniform float maxFreq`
- Replace `int totalBars = numOctaves * 12` with `int totalBars = bars`
- Replace single-point semitone FFT lookup with identical band-averaging pattern as filaments (see above)
- Replace color mapping: `fract(float(i) / 12.0)` becomes `float(i) / float(totalBars)`

**UI**: Same pattern as filaments. `bars` int slider in Geometry section.

**Modulation**: Add `ModEngineRegisterParam("slashes.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

---

### Nebula

**Config changes** (`NebulaConfig`):

| Remove | Add | Type | Default | Range | Section |
|--------|-----|------|---------|-------|---------|
| `int numOctaves = 5` | `int starBins = 60` | int | 60 | 12-120 | Stars |
| | `float maxFreq = 14000.0f` | float | 14000.0 | 1000-16000 | Audio |

**Effect struct changes** (`NebulaEffect`):

| Remove | Add |
|--------|-----|
| `int numOctavesLoc` | `int starBinsLoc` |
| | `int maxFreqLoc` |

**Shader migration** (`nebula.fs`):
- Replace `uniform int numOctaves` with `uniform int starBins` and `uniform float maxFreq`
- In `main()`: replace `int totalSemitones = numOctaves * 12` with `int totalStarBins = starBins`
- In `starLayer()`: rename parameter from `totalSemitones` to `totalStarBins`
- Replace star frequency lookup (log-space instead of semitone-based):

```glsl
// Old:
float sBin = baseFreq * pow(2.0, semi / 12.0) / (sampleRate * 0.5);

// New:
float sBin = baseFreq * pow(maxFreq / baseFreq, semi / float(totalStarBins)) / (sampleRate * 0.5);
```

- Color mapping already uses `semi / float(totalSemitones)` — just rename to `semi / float(totalStarBins)`
- No band-averaging for nebula (stars pick random discrete bins, not sequential bands)

**UI**: `starBins` int slider in Stars section. `maxFreq` slider in Audio section.

**Modulation**: Add `ModEngineRegisterParam("nebula.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

---

## Tasks

### Wave 1: Config Headers

#### Task 1.1: Update FilamentsConfig

**Files**: `src/effects/filaments.h`

**Do**: Remove `numOctaves` field, add `filaments` and `maxFreq` fields per Design section. Update `FilamentsEffect` struct (remove `numOctavesLoc`, add `filamentsLoc` + `maxFreqLoc`). Update `FILAMENTS_CONFIG_FIELDS` macro (replace `numOctaves` with `filaments, maxFreq`).

**Verify**: Header compiles (included by other files in Wave 2).

#### Task 1.2: Update SlashesConfig

**Files**: `src/effects/slashes.h`

**Do**: Remove `numOctaves` field, add `bars` and `maxFreq` fields per Design section. Update `SlashesEffect` struct (remove `numOctavesLoc`, add `barsLoc` + `maxFreqLoc`). Update `SLASHES_CONFIG_FIELDS` macro (replace `numOctaves` with `bars, maxFreq`).

**Verify**: Header compiles.

#### Task 1.3: Update NebulaConfig

**Files**: `src/effects/nebula.h`

**Do**: Remove `numOctaves` field, add `starBins` and `maxFreq` fields per Design section. Update `NebulaEffect` struct (remove `numOctavesLoc`, add `starBinsLoc` + `maxFreqLoc`). Update `NEBULA_CONFIG_FIELDS` macro (replace `numOctaves` with `starBins, maxFreq`).

**Verify**: Header compiles.

---

### Wave 2: Implementation (all parallel, no file overlap)

#### Task 2.1: Filaments C++ and shader

**Files**: `src/effects/filaments.cpp`, `shaders/filaments.fs`
**Depends on**: Task 1.1

**Do**:
- **filaments.cpp**: Replace `numOctavesLoc` cache with `filamentsLoc` + `maxFreqLoc`. Update `SetShaderValue` calls — send `filaments` as `SHADER_UNIFORM_INT` and `maxFreq` as `SHADER_UNIFORM_FLOAT`. Add `ModEngineRegisterParam("filaments.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`.
- **filaments.fs**: Replace uniforms, loop count, FFT lookup, and color mapping per Design section. Follow arc_strobe.fs band-averaging pattern exactly.

**Verify**: Shader syntax is valid GLSL 330. C++ compiles.

#### Task 2.2: Slashes C++ and shader

**Files**: `src/effects/slashes.cpp`, `shaders/slashes.fs`
**Depends on**: Task 1.2

**Do**:
- **slashes.cpp**: Replace `numOctavesLoc` cache with `barsLoc` + `maxFreqLoc`. Update `SetShaderValue` calls. Add `ModEngineRegisterParam("slashes.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`.
- **slashes.fs**: Replace uniforms, loop count, FFT lookup, and color mapping per Design section. Follow arc_strobe.fs band-averaging pattern exactly.

**Verify**: Shader syntax is valid GLSL 330. C++ compiles.

#### Task 2.3: Nebula C++ and shader

**Files**: `src/effects/nebula.cpp`, `shaders/nebula.fs`
**Depends on**: Task 1.3

**Do**:
- **nebula.cpp**: Replace `numOctavesLoc` cache with `starBinsLoc` + `maxFreqLoc`. Update `SetShaderValue` calls. Add `ModEngineRegisterParam("nebula.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`.
- **nebula.fs**: Replace uniforms per Design section. In `main()`, replace `totalSemitones` derivation. In `starLayer()`, rename parameter and update frequency lookup to log-space formula. Keep single-point FFT (no band-averaging).

**Verify**: Shader syntax is valid GLSL 330. C++ compiles.

#### Task 2.4: Filaments and Slashes UI

**Files**: `src/ui/imgui_effects_gen_filament.cpp`
**Depends on**: Tasks 1.1, 1.2

**Do**:
- **DrawFilamentsParams**: Replace `SliderInt("Octaves##filaments", ...)` with `ImGui::SliderInt("Filaments##filaments", &cfg->filaments, 4, 256)` in Geometry section. Add `ModulatableSlider("Max Freq (Hz)##filaments", &cfg->maxFreq, "filaments.maxFreq", "%.0f", modSources)` in Audio section. Reorder Audio sliders to: Base Freq, Max Freq, Gain, Contrast, Base Bright (move `filaments` slider to Geometry section).
- **DrawSlashesParams**: Replace `SliderInt("Octaves##slashes", ...)` with `ImGui::SliderInt("Bars##slashes", &cfg->bars, 4, 256)` in Geometry section. Add `ModulatableSlider("Max Freq (Hz)##slashes", &cfg->maxFreq, "slashes.maxFreq", "%.0f", modSources)` in Audio section. Reorder Audio sliders to: Base Freq, Max Freq, Gain, Contrast, Base Bright (move `bars` slider to Geometry section).

**Verify**: C++ compiles.

#### Task 2.5: Nebula UI

**Files**: `src/ui/imgui_effects_gen_atmosphere.cpp`
**Depends on**: Task 1.3

**Do**:
- **DrawGeneratorsNebula**: Replace `SliderInt("Octaves##nebula", ...)` with `ImGui::SliderInt("Star Bins##nebula", &n->starBins, 12, 120)` in Stars section. Add `ModulatableSlider("Max Freq (Hz)##nebula", &n->maxFreq, "nebula.maxFreq", "%.0f", modSources)` in Audio section. Reorder Audio sliders to: Base Freq, Max Freq, Gain, Contrast, Base Bright.

**Verify**: C++ compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Filaments displays correctly with default settings (60 filaments, 55-14000 Hz)
- [ ] Slashes displays correctly with default settings (60 bars, 55-14000 Hz)
- [ ] Nebula stars react to audio across full frequency range
- [ ] Adjusting `filaments`/`bars`/`starBins` changes visual density without affecting frequency coverage
- [ ] Adjusting `maxFreq` changes frequency ceiling without affecting visual density
- [ ] Old presets load without crash (numOctaves ignored, new fields get defaults)
