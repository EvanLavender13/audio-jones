# Cymatics Physics Revision

Revise the Cymatics generator's wave propagation model for physical accuracy: replace pure Gaussian attenuation with cylindrical spreading, fix trail blending to use screen-blend instead of max(), normalize wave sum by source count, and add iso-line contour mode.

**Research**: `docs/research/cymatics-physics-revision.md`

## Design

### Types

**CymaticsConfig** — add one field, keep existing:

```
int contourMode = 0;     // Visualization mode (0=off, 1=bands, 2=lines)
int contourCount = 5;    // Number of contour bands/lines (1-20)
```

`contourCount` default changes from `0` to `5` (it no longer gates enable/disable — `contourMode` does that). Range widens to 1-20 for finer iso-lines.

**CymaticsEffect** — add one uniform location:

```
int contourModeLoc;
```

**CYMATICS_CONFIG_FIELDS** — insert `contourMode` after `contourCount`.

### Algorithm

Four independent shader changes, all in `cymatics.fs`:

**1. Attenuation — cylindrical spreading + Gaussian envelope:**

Replace the pure Gaussian with 1/√(r+ε) spreading multiplied by the existing Gaussian envelope:

```glsl
const float EPSILON = 0.01; // softens 1/0 singularity; max multiplier = 10x

// In the source loop (replaces: float attenuation = exp(-dist * dist * falloff))
float spreading = 1.0 / sqrt(dist + EPSILON);
float envelope = exp(-dist * dist * falloff);
totalWave += fetchWaveform(delay) * spreading * envelope;

// Same for mirror sources (replaces: float mAtten = exp(-mDist * mDist * falloff) * reflectionGain)
float mSpreading = 1.0 / sqrt(mDist + EPSILON);
float mEnvelope = exp(-mDist * mDist * falloff);
float mAtten = mSpreading * mEnvelope * reflectionGain;
totalWave += fetchWaveform(mDelay) * mAtten;
```

**2. Source normalization:**

After the source loop closes, divide by source count:

```glsl
totalWave /= float(sourceCount);
```

**3. Contour modes:**

Replace the existing single-mode posterize block with a mode switch:

```glsl
uniform int contourMode; // 0=off, 1=bands, 2=iso-lines

float wave = totalWave;
if (contourMode == 1) {
    // Bands: quantize to steps (existing posterize logic)
    wave = floor(totalWave * float(contourCount) + 0.5) / float(contourCount);
} else if (contourMode == 2) {
    // Iso-amplitude lines: thin bright lines at each contour level
    float contoured = fract(abs(totalWave) * float(contourCount));
    float lineWidth = 0.1;
    wave = totalWave * smoothstep(0.0, lineWidth, contoured)
                     * smoothstep(lineWidth * 2.0, lineWidth, contoured);
}
```

**4. Trail blending — screen-blend:**

Replace `max(existing, newColor)` with screen-blend accumulation:

```glsl
existing *= decayFactor;
finalColor = existing + newColor * (1.0 - existing);
```

The `existing *= decayFactor` line already exists at line 107; the only change is the final line from `max()` to screen-blend.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| contourMode | int | 0-2 | 0 | No | "Contour Mode##cym" |
| contourCount | int | 1-20 | 5 | No | "Contours##cym" |

Rebalanced defaults (1/√r spreading carries more energy at distance):

| Parameter | Old Default | New Default | Notes |
|-----------|-------------|-------------|-------|
| falloff | 1.0 | 0.5 | Gaussian envelope only — 1/√r handles near-field attenuation |
| visualGain | 2.0 | 1.0 | More energy reaches distant pixels; lower gain prevents clipping |

`contourMode` is an int combo — not a modulation target.

### Constants

No new enums or effect types. This modifies the existing `TRANSFORM_CYMATICS` generator.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Update CymaticsConfig and CymaticsEffect

**Files**: `src/effects/cymatics.h`
**Creates**: `contourMode` field and `contourModeLoc` uniform location that Wave 2 tasks depend on

**Do**:
1. Add `int contourMode = 0;` field to `CymaticsConfig` after `contourCount`. Change `contourCount` default from `0` to `5`.
2. Change `falloff` default from `1.0f` to `0.5f`. Change `visualGain` default from `2.0f` to `1.0f`.
3. Add `int contourModeLoc;` to `CymaticsEffect` after `contourCountLoc`.
4. Update `CYMATICS_CONFIG_FIELDS` to include `contourMode` after `contourCount`.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update cymatics.cpp

**Files**: `src/effects/cymatics.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. In `CymaticsEffectInit`: cache `contourModeLoc` via `GetShaderLocation(e->shader, "contourMode")` after the `contourCountLoc` line.
2. In `CymaticsEffectSetup`: bind `contourMode` uniform via `SetShaderValue(e->shader, e->contourModeLoc, &cfg->contourMode, SHADER_UNIFORM_INT)` after the `contourCount` binding.
3. In `DrawCymaticsParams` UI section: replace the `ImGui::SliderInt("Contours##cym", ...)` line with a `ImGui::Combo` for contour mode (items: "Off\0Bands\0Lines\0") followed by a conditional `ImGui::SliderInt("Contours##cym", &e->cymatics.contourCount, 1, 20)` that only shows when `contourMode > 0`.

Follow existing patterns in the file for uniform caching and binding.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Revise cymatics.fs shader

**Files**: `shaders/cymatics.fs`
**Depends on**: Wave 1 complete

**Do** — implement all four Algorithm section changes:

1. Add `uniform int contourMode;` to the uniform declarations (after `contourCount`).
2. Add `const float EPSILON = 0.01;` at the top of `main()`.
3. **Attenuation**: In the source loop, replace `float attenuation = exp(-dist * dist * falloff)` with the spreading + envelope formula from the Algorithm section. Apply the same change to the mirror source attenuation block.
4. **Source normalization**: After the source loop's closing brace (after the `}` for `for (int i = ...)`), add `totalWave /= float(sourceCount);`.
5. **Contour modes**: Replace the existing `if (contourCount > 0)` block with the contourMode switch from the Algorithm section.
6. **Trail blending**: Replace `finalColor = max(existing, newColor);` with `finalColor = existing + newColor * (1.0 - existing);`.

Reference for cylindrical wave spreading: amplitude ∝ 1/√r for 2D point sources (energy conservation over expanding wavefront circumference). See also [Paul Bourke - Chladni Plate Interference Surfaces](https://paulbourke.net/geometry/chladni/).

**Verify**: `cmake.exe --build build` compiles. Visual check: waves now carry across screen with gradual falloff instead of dying at short range; trails show dynamic interference instead of solid blobs.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Waves propagate visibly across the full screen (1/√r spreading working)
- [ ] Changing sourceCount doesn't drastically change brightness (normalization working)
- [ ] Trail shows dynamic interference, not static bright blobs (screen-blend working)
- [ ] Contour mode 0 = smooth, 1 = flat bands, 2 = thin iso-lines
- [ ] Contours slider only visible when mode > 0
- [ ] Existing presets still load (contourMode defaults to 0 via `from_json` default init)

---

## Implementation Notes

**Screen-blend reverted to max()**: The plan called for replacing `max(existing, newColor)` with screen-blend (`existing + newColor * (1.0 - existing)`). Screen-blend only increases brightness frame-over-frame, causing washed-out pastel accumulation. Reverted to `max()` with decay — the 1/√r spreading already provides dynamic propagation without needing additive blending.

**Color/brightness decoupled from gain**: The plan used a single `tanh(wave * visualGain)` for both color LUT mapping and brightness. This made gain control which colors were visible and caused white-out at high values. Split into two independent paths:
- Color: `tanh(wave)` (no gain) — always maps the full gradient across the wave pattern
- Brightness: `abs(tanh(wave * visualGain))` — gain controls how far waves reach before fading to black, capped at `value` so no white-out

**waveScale default raised to 50**: Only looked good at max; made it the default.

**reflectionGain default lowered to 0.5**: Boundary sources were too hot at 1.0 with the new 1/√r spreading.
