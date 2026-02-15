# Pitch Spiral Frequency Spread Migration

Migrate pitch spiral from the old `numOctaves`-based pitch mapping to the frequency-spread pattern. Replace `numOctaves` with `maxFreq`, rework `numTurns` from a visibility clamp into actual spiral scaling (N turns = full spectrum), and switch color mapping from repeating pitch class to smooth gradient.

**Research**: `docs/research/fft-frequency-spread.md`

## Design

### Current Problems

1. `numOctaves` couples frequency ceiling to octave count — can't reach high frequencies without many octaves
2. `numTurns` is just a visibility clamp — it doesn't change the spiral structure. Changing it clips audio data but the spiral geometry stays identical.
3. One turn is hardcoded to one octave (1200 cents), so frequency density is fixed

### New Behavior

- `maxFreq` directly sets the frequency ceiling (1000-16000 Hz)
- `numTurns` controls how many spiral turns cover the full `baseFreq → maxFreq` range
- More turns = finer frequency resolution per turn, spiral extends further at same spacing
- Fewer turns = coarser, each turn covers more of the spectrum
- `spiralSpacing` stays independent — user controls visual zoom

### Types

**PitchSpiralConfig** changes:
- Remove: `int numOctaves = 5`
- Add: `float maxFreq = 14000.0f` (range 1000-16000, modulatable)

`numTurns` stays as-is (already exists, just changes semantic from "clamp" to "density").

**PitchSpiralEffect** changes:
- Remove: `int numOctavesLoc`
- Add: `int maxFreqLoc`

### Algorithm

**Shader frequency mapping** (replaces the cents-based approach):

```glsl
// Continuous spiral position normalized to [0, 1] over numTurns
float spiralPos = offset / (spiralSpacing * float(numTurns));

// Log-space interpolation from baseFreq to maxFreq
float freq = baseFreq * pow(maxFreq / baseFreq, spiralPos);
float bin = freq / (sampleRate * 0.5);
```

**Visibility clamp** (replaces the old `radialTurn < numTurns` and `freq < freqCeiling` checks):

```glsl
if (bin >= 0.0 && bin <= 1.0 && spiralPos >= 0.0 && spiralPos <= 1.0) {
    magnitude = texture(fftTexture, vec2(bin, 0.5)).r;
}
```

`spiralPos <= 1.0` naturally replaces both old clamps — anything beyond numTurns is outside the spectrum.

**Color mapping** (smooth gradient replaces pitch class):

```glsl
// Old: float pitchClass = fract(cents / 1200.0);
float t = clamp(spiralPos, 0.0, 1.0);
vec4 noteColor = texture(gradientLUT, vec2(t, 0.5));
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| maxFreq | float | 1000-16000 | 14000.0 | Yes | "Max Freq (Hz)" |
| ~~numOctaves~~ | ~~int~~ | — | — | — | Removed |

All other params unchanged.

### UI Changes

Audio section slider order becomes: Base Freq, Max Freq, Gain, Contrast, Base Bright (matches convention). The old `Octaves` slider is removed. `Max Freq` uses `ModulatableSlider` with `"%.0f"` format.

---

## Tasks

### Wave 1: All changes

All files are disjoint — but this is small enough to be a single task.

#### Task 1.1: Config, shader, C++, UI, modulation

**Files** (modify):
- `src/effects/pitch_spiral.h`
- `src/effects/pitch_spiral.cpp`
- `shaders/pitch_spiral.fs`
- `src/ui/imgui_effects_gen_geometric.cpp`

**Do**:

1. **Header** (`pitch_spiral.h`):
   - Replace `int numOctaves = 5` with `float maxFreq = 14000.0f` in `PitchSpiralConfig`
   - Update `PITCH_SPIRAL_CONFIG_FIELDS`: replace `numOctaves` with `maxFreq`
   - In `PitchSpiralEffect`: replace `int numOctavesLoc` with `int maxFreqLoc`

2. **C++** (`pitch_spiral.cpp`):
   - In `PitchSpiralEffectInit`: replace `GetShaderLocation(e->shader, "numOctaves")` with `GetShaderLocation(e->shader, "maxFreq")`
   - In `PitchSpiralEffectSetup`: replace the `numOctaves` `SetShaderValue` (INT) with `maxFreq` (FLOAT)
   - In `PitchSpiralRegisterParams`: add `ModEngineRegisterParam("pitchSpiral.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

3. **Shader** (`pitch_spiral.fs`):
   - Replace `uniform int numOctaves` with `uniform float maxFreq`
   - Replace the pitch mapping block (Step 2) with the Algorithm section above
   - Replace the color mapping line with smooth gradient `t = clamp(spiralPos, 0.0, 1.0)`
   - Remove the `TET_ROOT` define (no longer used)

4. **UI** (`imgui_effects_gen_geometric.cpp`):
   - Remove `ImGui::SliderInt("Octaves##pitchspiral", &ps->numOctaves, 1, 8)`
   - Add `ModulatableSlider("Max Freq (Hz)##pitchspiral", &ps->maxFreq, "pitchSpiral.maxFreq", "%.0f", modSources)` after Base Freq
   - Audio slider order: Base Freq, Max Freq, Gain, Contrast, Base Bright

**Verify**: `cmake.exe --build build` compiles with no errors.

---

## Final Verification

- [ ] Build succeeds with no warnings on the changed files
- [ ] Old `numOctaves` field absent from config, shader, UI, and modulation
- [ ] `maxFreq` registered as modulatable param with range 1000-16000
- [ ] Shader uses `spiralPos` for both frequency mapping and color (no cents, no TET_ROOT)
- [ ] Audio section follows slider order convention: Base Freq, Max Freq, Gain, Contrast, Base Bright
