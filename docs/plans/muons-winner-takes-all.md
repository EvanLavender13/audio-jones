# Muons Winner-Takes-All Color & Audio

Replace the additive per-step color accumulation in the Muons generator with a winner-takes-all approach: the closest shell crossing determines both the pixel's LUT color and its FFT frequency band, producing spatially coherent audio-reactive regions instead of washed-out additive glow.

**Research**: `docs/research/muons-winner-takes-all.md`

## Design

### Changes Summary

1. **Shader rewrite** — replace additive loop body with winner tracking + post-loop color/audio
2. **Remove `colorFreq`** — winner step index IS the LUT coordinate; `colorFreq` is redundant
3. **No new params** — reuses all existing audio/color/tonemap params

### Algorithm

The march loop body changes from accumulating color per step to tracking which step has the minimum proximity (`d * s`). After the loop, the winner's step index drives both LUT color and FFT frequency band.

**Inside the march loop** (replaces `color += ...` accumulation):

```glsl
// After shell distance computation (d = ringThickness * ...):
z += d;

// Track closest shell crossing
float proximity = d * s;
if (proximity < closestHit) {
    closestHit = proximity;
    winnerStep = i;
    winnerGlow = 1.0 / max(proximity, 1e-6);
}
```

**After the loop** (replaces nothing — this is new post-loop code):

```glsl
// Color from winner's step position in LUT
float lutCoord = fract(float(winnerStep) / float(max(marchSteps - 1, 1)) + time * colorSpeed);
vec3 sampleColor = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

// FFT from winner's frequency band (multi-sample, same as motherboard)
float t0 = float(winnerStep) / float(max(marchSteps - 1, 1));
float t1 = float(winnerStep + 1) / float(max(marchSteps - 1, 1));
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

const int BAND_SAMPLES = 4;
float energy = 0.0;
for (int bs = 0; bs < BAND_SAMPLES; bs++) {
    float bin = mix(binLo, binHi, (float(bs) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
}
energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float audio = baseBright + energy;

// Final pixel — glow * color * audio * brightness
color = vec3(winnerGlow * audio) * sampleColor;
```

Key details:
- `closestHit` initialized to `1e6` before the loop
- `winnerStep` initialized to `0`
- `winnerGlow` initialized to `0.0`
- `s` variable in the existing shader means the `length(a)` value — rename the band-sample loop variable to `bs` to avoid shadowing
- The existing `z += d` step remains in the loop body (before the proximity check)
- The existing FBM turbulence and distance mode `switch` blocks are untouched
- Tonemap (`tanh`) and trail buffer code after the loop are untouched
- Research multiplies `brightness` in both the pixel line and the tonemap — the plan intentionally applies it only in the tonemap to avoid doubling
- Risk: winner-takes-all may look harsher than the old soft additive glow. If so, a `winnerSharpness` blend param can be added later — start with pure winner and evaluate

### Field Removal: `colorFreq`

Remove `colorFreq` from all touchpoints:
- `MuonsConfig` struct field and range comment
- `MUONS_CONFIG_FIELDS` macro
- `MuonsEffect` struct: `colorFreqLoc` member
- Init: `GetShaderLocation(e->shader, "colorFreq")` line
- Setup: `SetShaderValue(e->shader, e->colorFreqLoc, ...)` line
- Shader: `uniform float colorFreq;` declaration
- RegisterParams: `ModEngineRegisterParam("muons.colorFreq", ...)` line
- UI: `ModulatableSlider("Color Freq##muons", ...)` line

### Parameters

No new parameters. Existing params that gain new perceptual impact:

| Parameter | Effect in Winner Mode |
|-----------|----------------------|
| baseFreq / maxFreq | Visibly control which screen regions respond to which frequencies |
| gain / curve | Brightness contrast between active and silent frequency bands per-region |
| colorSpeed | LUT scroll rate still applies as offset to winner-derived coordinate |
| brightness / exposure | Same tonemap role |

---

## Tasks

### Wave 1: Config & Shader

All three files have no overlap — can run in parallel.

#### Task 1.1: Remove `colorFreq` from config header

**Files**: `src/effects/muons.h` (modify)

**Do**:
- Remove the `float colorFreq = 33.0f;` field and its range comment from `MuonsConfig`
- Remove `colorFreq,` from `MUONS_CONFIG_FIELDS` macro
- Remove `int colorFreqLoc;` from `MuonsEffect` struct

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Rewrite shader to winner-takes-all

**Files**: `shaders/muons.fs` (modify)

**Do**:
- Remove `uniform float colorFreq;` declaration
- Rewrite the march loop and post-loop code. Implement the Algorithm section above exactly. Keep all code before the loop (coordinate setup, ray direction) and after the new color computation (tonemap, trail buffer) unchanged. The existing FBM turbulence block (lines 69-95) and distance mode switch (lines 98-124) inside the loop are untouched — only the accumulation at lines 130-137 changes, plus new post-loop code replaces the current inline accumulation.

**Verify**: Shader file has no syntax errors (build + run to confirm).

#### Task 1.3: Remove `colorFreq` from C++ module

**Files**: `src/effects/muons.cpp` (modify)

**Do**:
- Remove `GetShaderLocation(e->shader, "colorFreq")` from `MuonsEffectInit`
- Remove `SetShaderValue(e->shader, e->colorFreqLoc, ...)` from `MuonsEffectSetup`
- Remove `ModEngineRegisterParam("muons.colorFreq", ...)` from `MuonsRegisterParams`
- Remove the `ModulatableSlider("Color Freq##muons", ...)` line from `DrawMuonsParams`

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect renders: enable Muons, confirm filaments appear with distinct color regions rather than washed-out additive glow
- [ ] Audio reactivity: different screen regions should visibly respond to different frequency bands
- [ ] Trail persistence: ping-pong decay/blur still works (filaments leave trails)
- [ ] Tonemap: bright areas clamp smoothly (tanh rolloff unchanged)
- [ ] Preset load: existing presets with `colorFreq` field load without error (nlohmann `WITH_DEFAULT` ignores unknown fields)

---

## Implementation Notes

### Additional change: removed `exposure` parameter

The original plan kept both `brightness` and `exposure` as tonemap controls. Post-implementation, `exposure` was removed because the two params were redundant — `brightness / exposure` is a single ratio, so two sliders for one knob is confusing. The `exposure` default (3000) is now baked into the shader as a constant divisor, leaving `brightness` (0.1-5.0) as the sole tonemap control.

Removed `exposure` from: `MuonsConfig` field, `MUONS_CONFIG_FIELDS`, `MuonsEffect::exposureLoc`, init/setup/registerParams/UI, and `uniform float exposure` in the shader.

### Removed fields total

- `colorFreq` — winner step index IS the LUT coordinate
- `exposure` — redundant with `brightness`; divisor baked into shader
