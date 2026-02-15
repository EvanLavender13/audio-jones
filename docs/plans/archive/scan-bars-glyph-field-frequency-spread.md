# Scan Bars & Glyph Field Frequency Spread Migration

Migrate scan bars and glyph field from `numOctaves`-based semitone mapping to the `baseFreq → maxFreq` log-space frequency spread with configurable frequency bins. Both effects keep their existing coloring approaches (chaos-based for scan bars, hash-to-gradient for glyph field). Single-point FFT lookup per bin, same pattern as nebula's `starBins`.

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Config Changes

Both effects: remove `int numOctaves`, add `float maxFreq` and `int freqBins`.

**ScanBarsConfig** (in `scan_bars.h`):
```
Remove: int numOctaves = 5;
Add:    float maxFreq = 14000.0f; // Ceiling frequency Hz (1000-16000)
Add:    int freqBins = 48;        // Discrete frequency bins for FFT lookup (12-120)
```

**GlyphFieldConfig** (in `glyph_field.h`):
```
Remove: int numOctaves = 5;
Add:    float maxFreq = 14000.0f; // Ceiling frequency Hz (1000-16000)
Add:    int freqBins = 48;        // Discrete frequency bins for FFT lookup (12-120)
```

Both CONFIG_FIELDS macros: replace `numOctaves` with `maxFreq, freqBins`.

### Effect Struct Changes

Both effect structs: replace `int numOctavesLoc` with `int maxFreqLoc` and add `int freqBinsLoc`.

### Shader Algorithm

**Scan Bars** — Replace STEP 4 (semitone-to-FFT lookup). Current code maps color chaos `t` through semitones. New code quantizes `t` into `freqBins` discrete bins, then maps through log-space frequency spread:

```glsl
// Old:
float totalSemitones = float(numOctaves) * 12.0;
float semi = t * totalSemitones;
float freq = baseFreq * pow(2.0, semi / 12.0);

// New:
float binIdx = floor(t * float(freqBins));
float quantT = (binIdx + 0.5) / float(freqBins);
float freq = baseFreq * pow(maxFreq / baseFreq, quantT);
```

Remove `uniform int numOctaves`, add `uniform float maxFreq` and `uniform int freqBins`. Rest of lookup unchanged.

**Glyph Field** — Replace the per-cell FFT block (lines ~198-211). Current code quantizes `h.z` to an integer semitone. New code quantizes into `freqBins` discrete bins:

```glsl
// Old:
int totalSemitones = numOctaves * 12;
int globalSemi = int(floor(h.z * float(totalSemitones)));
float lutPos = (float(globalSemi) + 0.5) / float(totalSemitones);
float freq = baseFreq * pow(2.0, float(globalSemi) / 12.0);

// New:
float binIdx = floor(h.z * float(freqBins));
float lutPos = (binIdx + 0.5) / float(freqBins);
float freq = baseFreq * pow(maxFreq / baseFreq, lutPos);
```

Remove `uniform int numOctaves`, add `uniform float maxFreq` and `uniform int freqBins`. The `bin`, `mag`, gradient LUT, and brightness lines stay the same.

### Parameters

**Scan Bars**:

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| ~~numOctaves~~ | ~~int~~ | ~~1-8~~ | ~~5~~ | ~~No~~ | ~~Octaves~~ |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| freqBins | int | 12-120 | 48 | No | Freq Bins |

**Glyph Field**:

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| ~~numOctaves~~ | ~~int~~ | ~~1-8~~ | ~~5~~ | ~~No~~ | ~~Octaves~~ |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| freqBins | int | 12-120 | 48 | No | Freq Bins |

### UI Layout

Both effects' Audio sections: Remove `SliderInt("Octaves##...")`. Add Max Freq and Freq Bins. Final order: Base Freq, Max Freq, Freq Bins, Gain, Contrast, Base Bright. Max Freq uses `ModulatableSlider` with `"%.0f"` format. Freq Bins uses `ImGui::SliderInt`.

---

## Tasks

### Wave 1: Config Headers

#### Task 1.1: Update ScanBarsConfig

**Files**: `src/effects/scan_bars.h`

**Do**:
- In `ScanBarsConfig`: remove `int numOctaves = 5;`, add `float maxFreq = 14000.0f;` with range comment `// Ceiling frequency Hz (1000-16000)` and `int freqBins = 48;` with comment `// Discrete frequency bins for FFT lookup (12-120)`
- In `SCAN_BARS_CONFIG_FIELDS`: replace `numOctaves` with `maxFreq, freqBins`
- In `ScanBarsEffect`: replace `int numOctavesLoc;` with `int maxFreqLoc;` and add `int freqBinsLoc;`

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Update GlyphFieldConfig

**Files**: `src/effects/glyph_field.h`

**Do**:
- In `GlyphFieldConfig`: remove `int numOctaves = 5;`, add `float maxFreq = 14000.0f;` with range comment `// Ceiling frequency Hz (1000-16000)` and `int freqBins = 48;` with comment `// Discrete frequency bins for FFT lookup (12-120)`
- In `GLYPH_FIELD_CONFIG_FIELDS`: replace `numOctaves` with `maxFreq, freqBins`
- In `GlyphFieldEffect`: replace `int numOctavesLoc;` with `int maxFreqLoc;` and add `int freqBinsLoc;`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation

#### Task 2.1: Update scan_bars.cpp

**Files**: `src/effects/scan_bars.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `ScanBarsEffectInit`: replace `GetShaderLocation(e->shader, "numOctaves")` → `GetShaderLocation(e->shader, "maxFreq")` storing in `e->maxFreqLoc`, add `e->freqBinsLoc = GetShaderLocation(e->shader, "freqBins")`
- In `ScanBarsEffectSetup`: replace `SetShaderValue(...numOctavesLoc, &cfg->numOctaves, SHADER_UNIFORM_INT)` with `SetShaderValue(...maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT)` and add `SetShaderValue(...freqBinsLoc, &cfg->freqBins, SHADER_UNIFORM_INT)`
- In `ScanBarsRegisterParams`: add `ModEngineRegisterParam("scanBars.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)` (freqBins is int, not modulatable)

**Verify**: Compiles.

#### Task 2.2: Update glyph_field.cpp

**Files**: `src/effects/glyph_field.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `CacheLocations`: replace `GetShaderLocation(e->shader, "numOctaves")` → `GetShaderLocation(e->shader, "maxFreq")` storing in `e->maxFreqLoc`, add `e->freqBinsLoc = GetShaderLocation(e->shader, "freqBins")`
- In `BindUniforms`: replace `SetShaderValue(...numOctavesLoc, &cfg->numOctaves, SHADER_UNIFORM_INT)` with `SetShaderValue(...maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT)` and add `SetShaderValue(...freqBinsLoc, &cfg->freqBins, SHADER_UNIFORM_INT)`
- In `GlyphFieldRegisterParams`: add `ModEngineRegisterParam("glyphField.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)` (freqBins is int, not modulatable)

**Verify**: Compiles.

#### Task 2.3: Update scan_bars.fs shader

**Files**: `shaders/scan_bars.fs`
**Depends on**: Wave 1 complete

**Do**:
- Replace `uniform int numOctaves;` with `uniform float maxFreq;` and add `uniform int freqBins;`
- Replace STEP 4 FFT lookup block:
  ```glsl
  // Old (3 lines):
  float totalSemitones = float(numOctaves) * 12.0;
  float semi = t * totalSemitones;
  float freq = baseFreq * pow(2.0, semi / 12.0);

  // New (3 lines):
  float binIdx = floor(t * float(freqBins));
  float quantT = (binIdx + 0.5) / float(freqBins);
  float freq = baseFreq * pow(maxFreq / baseFreq, quantT);
  ```
- Rest of STEP 4 (bin calculation, mag lookup, pow/clamp) stays the same

**Verify**: Shader is runtime-compiled, no build needed.

#### Task 2.4: Update glyph_field.fs shader

**Files**: `shaders/glyph_field.fs`
**Depends on**: Wave 1 complete

**Do**:
- Replace `uniform int numOctaves;` with `uniform float maxFreq;` and add `uniform int freqBins;`
- Replace the per-cell FFT block (lines ~198-211):
  ```glsl
  // Old:
  int totalSemitones = numOctaves * 12;
  int globalSemi = int(floor(h.z * float(totalSemitones)));
  float lutPos = (float(globalSemi) + 0.5) / float(totalSemitones);
  float freq = baseFreq * pow(2.0, float(globalSemi) / 12.0);

  // New:
  float binIdx = floor(h.z * float(freqBins));
  float lutPos = (binIdx + 0.5) / float(freqBins);
  float freq = baseFreq * pow(maxFreq / baseFreq, lutPos);
  ```
- The `bin`, `mag`, gradient LUT, and brightness lines stay the same

**Verify**: Shader is runtime-compiled, no build needed.

#### Task 2.5: Update UI for both effects

**Files**: `src/ui/imgui_effects_gen_texture.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `DrawGeneratorsScanBars` Audio section:
  - Remove: `ImGui::SliderInt("Octaves##scanbars", &sb->numOctaves, 1, 8);`
  - After the Base Freq slider, add:
    ```cpp
    ModulatableSlider("Max Freq (Hz)##scanbars", &sb->maxFreq, "scanBars.maxFreq", "%.0f", modSources);
    ImGui::SliderInt("Freq Bins##scanbars", &sb->freqBins, 12, 120);
    ```
  - Result order: Base Freq, Max Freq, Freq Bins, Gain, Contrast, Base Bright

- In `DrawGeneratorsGlyphField` Audio section:
  - Remove: `ImGui::SliderInt("Octaves##glyphfield", &c->numOctaves, 1, 8);`
  - After the Base Freq slider, add:
    ```cpp
    ModulatableSlider("Max Freq (Hz)##glyphfield", &c->maxFreq, "glyphField.maxFreq", "%.0f", modSources);
    ImGui::SliderInt("Freq Bins##glyphfield", &c->freqBins, 12, 120);
    ```
  - Result order: Base Freq, Max Freq, Freq Bins, Gain, Contrast, Base Bright

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Scan bars Audio section shows: Base Freq, Max Freq, Freq Bins, Gain, Contrast, Base Bright (no Octaves)
- [ ] Glyph field Audio section shows: Base Freq, Max Freq, Freq Bins, Gain, Contrast, Base Bright (no Octaves)
- [ ] Both effects respond to audio across the full baseFreq → maxFreq range
- [ ] Scan bars coloring unchanged (chaos-based palette, not frequency-based)
- [ ] Glyph field coloring unchanged (hash-to-gradient per cell)
- [ ] Freq Bins slider adjusts granularity of frequency quantization
