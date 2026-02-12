# Scan Bars Audio Reactivity

Add per-bar FFT brightness modulation to the scan bars generator. Each bar stripe gets a semitone from its bar index via `floor()`. Active notes brighten their bar; silent bars dim to `baseBright`. The chaos color system stays untouched — FFT drives brightness only.

**Research**: `docs/research/scan_bars_audio.md`

## Design

### Types

Add to `ScanBarsConfig`:

```
// Audio
float baseFreq = 55.0f;    // Lowest mapped frequency in Hz (A1)
int numOctaves = 5;         // Octave range mapped across bars
float gain = 2.0f;          // FFT magnitude amplifier (0.1-10.0)
float curve = 0.7f;         // Contrast exponent (0.1-3.0)
float baseBright = 0.15f;   // Minimum brightness when silent (0.0-1.0)
```

Add to `ScanBarsEffect`:

```
int fftTextureLoc;
int sampleRateLoc;
int baseFreqLoc;
int numOctavesLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
```

Change `ScanBarsEffectSetup` signature: add `Texture2D fftTexture` parameter (same as filaments, slashes, spectral arcs).

### Algorithm

#### Bar index extraction (shader)

Before the existing `fract()` call on line 93, extract the integer bar index:

```glsl
float barCoord = barDensity * coord + scroll;
float barIndex = floor(barCoord);
float d = fract(barCoord);  // replaces fract(barDensity * coord + scroll)
```

#### Semitone-to-FFT lookup (shader)

Standard codebase pattern from spectral arcs / filaments:

```glsl
float totalSemitones = float(numOctaves) * 12.0;
float semi = mod(barIndex, totalSemitones);
float freq = baseFreq * pow(2.0, semi / 12.0);
float bin = freq / (sampleRate * 0.5);
float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
```

#### Brightness modulation (shader)

Replace the final output line:

```glsl
float react = baseBright + mag;
finalColor = vec4(color.rgb * mask * react, 1.0);
```

`baseBright + mag` follows the established generator pattern (gain already applied in the pow/clamp step). No double-application of gain.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 20.0–200.0 | 55.0 | yes | Base Freq (Hz) |
| numOctaves | int | 1–8 | 5 | yes | Octaves |
| gain | float | 0.1–10.0 | 2.0 | yes | Gain |
| curve | float | 0.1–3.0 | 0.7 | yes | Contrast |
| baseBright | float | 0.0–1.0 | 0.15 | yes | Base Bright |

### UI Layout

Audio section placed after Snap Amount slider, before the Color separator. Uses `ImGui::SeparatorText("Audio")` header. Slider order: Octaves, Base Freq, Gain, Contrast, Base Bright. `numOctaves` uses `ModulatableSliderInt`. `baseFreq` uses `ModulatableSlider` with `"%.1f"`. `gain` uses `"%.1f"`. `curve` and `baseBright` use `"%.2f"`.

---

## Tasks

### Wave 1: All files (no overlap)

All six files are distinct — every task touches different files.

#### Task 1.1: Config & effect structs

**Files**: `src/effects/scan_bars.h`
**Creates**: FFT config fields and uniform location slots

**Do**: Add 5 audio fields to `ScanBarsConfig` (after `snapAmount`), 7 uniform location ints to `ScanBarsEffect` (after `gradientLUTLoc`). Change `ScanBarsEffectSetup` signature to add `Texture2D fftTexture` parameter. Follow filaments.h as pattern.

**Verify**: Header parses (included by other files in build).

#### Task 1.2: Effect implementation

**Files**: `src/effects/scan_bars.cpp`
**Depends on**: Task 1.1 (needs updated header)

**Do**:
- In `ScanBarsEffectInit`: cache 7 new uniform locations (`fftTexture`, `sampleRate`, `baseFreq`, `numOctaves`, `gain`, `curve`, `baseBright`).
- In `ScanBarsEffectSetup`: add `Texture2D fftTexture` parameter. Bind all new uniforms. Bind `fftTexture` via `SetShaderValueTexture`. Bind `sampleRate` as `44100.0f` (matches other generators).
- In `ScanBarsRegisterParams`: register 5 new params with ranges from the Parameters table. `numOctaves` registered as float for modulation (cast to int in shader). Use `ModEngineRegisterParam("scanBars.baseFreq", ...)` etc.
- Follow filaments.cpp as pattern for FFT uniform binding.

**Verify**: Compiles.

#### Task 1.3: Shader FFT integration

**Files**: `shaders/scan_bars.fs`

**Do**: Implement the Algorithm section above:
- Add uniforms: `fftTexture` (sampler2D), `sampleRate` (float), `baseFreq` (float), `numOctaves` (int), `gain` (float), `curve` (float), `baseBright` (float).
- Extract `barIndex = floor(barCoord)` before the existing `fract()`.
- Add semitone-to-FFT lookup after the bar mask computation.
- Multiply final output by `react = baseBright + mag`.
- Use `mod()` for negative barIndex wrapping.

**Verify**: Shader compiles at runtime.

#### Task 1.4: Shader setup binding

**Files**: `src/render/shader_setup_generators.cpp`

**Do**: In `SetupScanBars`, change the call to pass `pe->fftTexture`:
```
ScanBarsEffectSetup(&pe->scanBars, &pe->effects.scanBars,
                    pe->currentDeltaTime, pe->fftTexture);
```
Follow the `SetupFilaments` pattern on line 113-115.

**Verify**: Compiles.

#### Task 1.5: UI panel

**Files**: `src/ui/imgui_effects_gen_texture.cpp`

**Do**: In `DrawGeneratorsScanBars`, add Audio section after the Snap Amount slider (before the Color separator). Use `ImGui::SeparatorText("Audio")`. Add sliders in order: Octaves (`ModulatableSliderInt`), Base Freq (`ModulatableSlider`, `"%.1f"`), Gain (`"%.1f"`), Contrast (`"%.2f"`), Base Bright (`"%.2f"`). All use `"scanBars.*"` param keys. Follow memory conventions for Audio section layout.

**Verify**: Compiles.

#### Task 1.6: Preset serialization

**Files**: `src/config/preset.cpp`

**Do**: Add `baseFreq`, `numOctaves`, `gain`, `curve`, `baseBright` to the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ScanBarsConfig, ...)` macro call. Append after `snapAmount`.

**Verify**: Compiles.

---

### Wave Dependencies

Tasks 1.1 must complete before 1.2 (header dependency). Tasks 1.3, 1.4, 1.5, 1.6 also depend on 1.1 for the updated type definitions.

Revised wave structure:
- **Wave 1**: Task 1.1 (header)
- **Wave 2**: Tasks 1.2, 1.3, 1.4, 1.5, 1.6 (all parallel — no file overlap)

## Final Verification

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] Scan bars render with FFT brightness variation when audio plays
- [ ] Silent bars dim to baseBright level
- [ ] Ring mode maps bass to inner rings, treble to outer
- [ ] All 5 new sliders appear in Audio section
- [ ] Presets save/load new fields correctly
- [ ] Existing scan bars presets load without regression (new fields get defaults)
