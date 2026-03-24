# Protean Clouds v2 - Camera & Mode Enhancements

Enhancement to the existing Protean Clouds generator. Three additions: configurable camera motion (replacing hardcoded `disp()`), swappable turbulence displacement waveforms, and swappable density evaluation functions.

## Classification

- **Category**: GENERATORS > Field (existing, section 13)
- **Pipeline Position**: Unchanged - single-pass fragment shader with blend compositor
- **Compute Model**: Unchanged - no ping-pong, no compute

## Attribution

- **Based on**: "Protean clouds" by nimitz (@stormoid)
- **Source**: https://www.shadertoy.com/view/3l23Rh
- **License**: CC BY-NC-SA 3.0

Density mode functions derived from Triply Periodic Minimal Surface (TPMS) implicit equations (Schoen 1970, Schwarz 1865).

Dot Noise mode from "Dot Noise" by Xor (GM Shaders): https://mini.gmshaders.com/p/dot-noise

Turbulence waveforms from existing Muons implementation (Muons by Xor, CC BY-NC-SA 3.0).

## References

- [IQ - FBM](https://iquilezles.org/articles/fbm/) - FBM theory, octave accumulation variations
- [IQ - Value Noise Derivatives](https://iquilezles.org/articles/morenoise/) - Derivative-modulated FBM for erosion effects
- [Xor - Dot Noise](https://mini.gmshaders.com/p/dot-noise) - Aperiodic gyroid via gold-ratio rotation matrix
- [Xor - Volumetric Raymarching](https://mini.gmshaders.com/p/volumetric) - Density field fundamentals, gyroid as cheap noise
- [Godot Shaders - Protean Clouds](https://godotshaders.com/shader/protean-clouds-2/) - Reference port of original nimitz shader

## Reference Code

### Current `disp()` function (to be replaced)

```glsl
vec2 disp(float t){ return vec2(sin(t*0.22), cos(t*0.175))*2.; }
```

Used in three places:
1. `ro.xy += disp(ro.z) * dspAmp;` - camera position
2. `p2.xy -= disp(p.z).xy;` - density field centering in `map()`
3. `target = normalize(ro - vec3(disp(flyPhase + tgtDst) * dspAmp, flyPhase + tgtDst));` - look-at target
4. `rd.xy *= rot(-disp(flyPhase + 3.5).x * 0.2);` - view roll (replaced by rollSpeed)

### Current FBM loop in `map()` (swap points marked)

```glsl
for(int i = 0; i < 5; i++)
{
    p += sin(p.zxy*0.75*trk + time*trk*.8)*dspAmp;        // SWAP 1: turbulence waveform
    d -= abs(dot(cos(p), sin(p.yzx))*z);                   // SWAP 2: density function
    z *= 0.57;
    trk *= 1.4;
    p = p*m3;
}
```

### Muons turbulence mode switch (reference for Swap 1)

```glsl
switch (turbulenceMode) {
case 1: // Fract Fold
    a += (fract(a * d + time * PHI) * 2.0 - 1.0).yzx / d * turbulenceStrength;
    break;
case 2: // Abs-Sin Fold
    a += abs(sin(a * d + time * PHI)).yzx / d * turbulenceStrength;
    break;
case 3: // Triangle Wave
    a += (abs(fract(a * d + time * PHI) * 2.0 - 1.0) * 2.0 - 1.0).yzx / d * turbulenceStrength;
    break;
case 4: // Squared Sin
    a += (sin(a * d + time * PHI) * abs(sin(a * d + time * PHI))).yzx / d * turbulenceStrength;
    break;
case 5: // Square Wave
    a += (step(0.5, fract(a * d + time * PHI)) * 2.0 - 1.0).yzx / d * turbulenceStrength;
    break;
case 6: // Quantized
    a += (floor(a * d + time * PHI + 0.5)).yzx / d * turbulenceStrength;
    break;
case 7: // Cosine
    a += cos(a * d + time * PHI).yzx / d * turbulenceStrength;
    break;
default: // 0: Sine (original)
    a += sin(a * d + time * PHI).yzx / d * turbulenceStrength;
    break;
}
```

### TPMS density functions (reference for Swap 2)

Gyroid (current):
```glsl
// Smooth interconnected channels - the classic nimitz cloud shape
dot(cos(p), sin(p.yzx))
// Range: [-3, +3]
```

Schwarz P:
```glsl
// Cubic lattice with spherical voids - bubbly/cellular
cos(p.x) + cos(p.y) + cos(p.z)
// Range: [-3, +3]
```

Diamond (Schwarz D):
```glsl
// Tetrahedral lattice - angular crystalline structure
cos(p.x)*cos(p.y)*cos(p.z) + sin(p.x)*sin(p.y)*cos(p.z)
    + sin(p.x)*cos(p.y)*sin(p.z) + cos(p.x)*sin(p.y)*sin(p.z)
// Range: [-2, +2], scale by 1.5 to match gyroid amplitude
```

Neovius:
```glsl
// Thick-walled complex cubic lattice - dense interlocking structure
(3.0*(cos(p.x) + cos(p.y) + cos(p.z)) + 4.0*cos(p.x)*cos(p.y)*cos(p.z)) * 0.23
// Raw range: [-13, +13], scaled to [-3, +3]
```

Dot Noise (aperiodic gyroid):
```glsl
// Aperiodic - no visible tiling, cloud-like blobs
const float PHI = 1.618033988;
const mat3 GOLD = mat3(
    -0.571464913, +0.814921382, +0.096597072,
    -0.278044873, -0.303026659, +0.911518454,
    +0.772087367, +0.494042493, +0.399753815);
dot(cos(GOLD * p), sin(PHI * p * GOLD))
// Range: [-3, +3]
```

## Algorithm

### Camera Motion

Replace the hardcoded `disp()` with parameterized dual-frequency displacement. The displacement must be evaluable per-sample in the shader (not just at camera position) because `map()` uses it to center the density field around the flight path.

**Shader-side `disp()` replacement:**
```glsl
uniform float driftAmplitude;
uniform float driftFreqX1, driftFreqY1;
uniform float driftFreqX2, driftFreqY2;
uniform float driftOffsetX2, driftOffsetY2;
uniform float rollAngle;

vec2 disp(float t) {
    vec2 d = vec2(sin(t * driftFreqX1), cos(t * driftFreqY1));
    if (driftFreqX2 > 0.0) d.x += sin(t * driftFreqX2 + driftOffsetX2);
    if (driftFreqY2 > 0.0) d.y += cos(t * driftFreqY2 + driftOffsetY2);
    return d * driftAmplitude;
}
```

**Roll**: Replace `rd.xy *= rot(-disp(flyPhase + 3.5).x * 0.2)` with `rd.xy *= rot(rollAngle)`. Accumulate `rollAngle += rollSpeed * deltaTime` on CPU.

**Config struct**: Embed `DualLissajousConfig drift` for parameter storage, UI, and serialization. In Setup, read its fields and pass as individual uniforms to the shader. Do NOT call `DualLissajousUpdate()` - the shader evaluates the displacement per-sample using flyPhase as the phase input, not CPU-accumulated time.

**Defaults** (approximate current behavior at speed 3.0):
- amplitude = 2.0, freqX1 = 0.22, freqY1 = 0.175
- freqX2 = 0.0 (disabled), freqY2 = 0.0 (disabled)
- rollSpeed = 0.0 (no roll by default)

Setting amplitude to 0 gives straight flight. Enabling secondary frequencies creates non-repeating weave paths.

### Turbulence Modes (Swap 1)

Same waveform set as Muons, adapted to protean clouds' FBM structure. The current sine displacement:
```glsl
p += sin(p.zxy * 0.75 * trk + time * trk * .8) * dspAmp;
```

becomes a switch on `turbulenceMode`, replacing `sin(...)` with each waveform while keeping the same argument `(p.zxy * 0.75 * trk + time * trk * .8)`:

| Mode | Name | Waveform | Character |
|------|------|----------|-----------|
| 0 | Sine (default) | `sin(arg)` | Smooth swirling folds |
| 1 | Fract Fold | `fract(arg) * 2.0 - 1.0` | Crystalline/faceted |
| 2 | Abs-Sin | `abs(sin(arg))` | Sharp valleys, always positive |
| 3 | Triangle | `abs(fract(arg/TWO_PI + 0.5) * 2.0 - 1.0) * 2.0 - 1.0` | Zigzag between sin and fract |
| 4 | Squared Sin | `sin(arg) * abs(sin(arg))` | Soft peaks, flat valleys |
| 5 | Square Wave | `step(0.5, fract(arg/TWO_PI)) * 2.0 - 1.0` | Blocky digital geometry |
| 6 | Quantized | `floor(sin(arg) * 4.0 + 0.5) / 4.0` | Staircase structures |
| 7 | Cosine | `cos(arg)` | Phase-shifted sine, different fold geometry |

### Density Modes (Swap 2)

Replace the density evaluation in the FBM loop. The current line:
```glsl
d -= abs(dot(cos(p), sin(p.yzx)) * z);
```

becomes:
```glsl
float density;
switch (densityMode) {
case 1:  // Schwarz P
    density = cos(p.x) + cos(p.y) + cos(p.z);
    break;
case 2:  // Diamond
    density = (cos(p.x)*cos(p.y)*cos(p.z) + sin(p.x)*sin(p.y)*cos(p.z)
             + sin(p.x)*cos(p.y)*sin(p.z) + cos(p.x)*sin(p.y)*sin(p.z)) * 1.5;
    break;
case 3:  // Neovius
    density = (3.0*(cos(p.x)+cos(p.y)+cos(p.z))
              + 4.0*cos(p.x)*cos(p.y)*cos(p.z)) * 0.23;
    break;
case 4:  // Dot Noise
    density = dot(cos(GOLD * p), sin(PHI * p * GOLD));
    break;
default: // 0: Gyroid (original)
    density = dot(cos(p), sin(p.yzx));
    break;
}
d -= abs(density * z);
```

All modes normalized to approximately [-3, +3] range so `morph` parameter behavior stays consistent across modes.

The `GOLD` matrix and `PHI` constant for Dot Noise mode are declared as shader constants (same values as the Reference Code section above).

## Parameters

### New Parameters

| Parameter | Type | Range | Default | Modulatable | Effect |
|-----------|------|-------|---------|-------------|--------|
| densityMode | int | 0-4 | 0 | No | Density evaluation function (Gyroid/Schwarz P/Diamond/Neovius/Dot Noise) |
| turbulenceMode | int | 0-7 | 0 | No | FBM displacement waveform (Sine/Fract/Abs-Sin/Triangle/Squared/Square/Quantized/Cosine) |
| rollSpeed | float | -PI_F to PI_F | 0.0 | Yes | Barrel roll rate around axis of travel (rad/s) |
| drift.amplitude | float | 0.0-4.0 | 2.0 | Yes | Lateral weave amplitude (0 = straight flight) |
| drift.motionSpeed | float | 0.0-5.0 | 1.0 | Yes | Drift rate multiplier |
| drift.freqX1 | float | 0.0-1.0 | 0.22 | No | Primary X displacement frequency |
| drift.freqY1 | float | 0.0-1.0 | 0.175 | No | Primary Y displacement frequency |
| drift.freqX2 | float | 0.0-1.0 | 0.0 | No | Secondary X frequency (0 = disabled) |
| drift.freqY2 | float | 0.0-1.0 | 0.0 | No | Secondary Y frequency (0 = disabled) |
| drift.offsetX2 | float | 0.0-TWO_PI | 0.3 | No | Phase offset for secondary X |
| drift.offsetY2 | float | 0.0-TWO_PI | 2.09 | No | Phase offset for secondary Y |

### Existing Parameters (unchanged)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| morph | float | 0.0-1.0 | 0.5 | Cloud morphology / fill amount |
| turbulence | float | 0.05-0.5 | 0.15 | FBM displacement amplitude |
| densityThreshold | float | 0.0-1.0 | 0.3 | Minimum density for visible volume |
| marchSteps | int | 40-130 | 80 | Raymarch iterations |
| colorBlend | float | 0.0-1.0 | 0.3 | LUT index: density vs depth |
| fogIntensity | float | 0.0-1.0 | 0.5 | Atmospheric fog amount |
| speed | float | 0.5-6.0 | 3.0 | Flight speed along z-axis |

## Modulation Candidates

- **rollSpeed**: Modulating creates swaying/rocking camera motion synced to audio
- **drift.amplitude**: Modulating widens/narrows the weave path with audio energy
- **morph**: Already modulatable - controls how filled/hollow the volume is
- **turbulence**: Already modulatable - controls displacement intensity in FBM
- **densityThreshold**: Already modulatable - gates volume visibility

### Interaction Patterns

**densityMode x turbulenceMode** (combinatorial emergence): 5 density modes x 8 turbulence waveforms = 40 unique visual characters from 2 dropdowns. The turbulence deforms the density field's periodic structure differently depending on which surface is being evaluated. Fract Fold turbulence + Diamond density creates sharp crystalline facets; Sine turbulence + Schwarz P creates bubbly organic cells.

**drift.amplitude x densityMode** (structural framing): The drift path weaves through different periodic structures. With Schwarz P the camera threads through spherical voids. With Diamond it cuts through angular tunnels. Same drift parameters, visually distinct flight paths.

**morph x densityMode** (threshold gating): Morph controls fill amount, but "filled" means different things per density mode. Gyroid fills channels, Schwarz P fills spherical cells, Neovius fills thick walls. Low morph + high densityThreshold can create completely different reveals per mode.

**rollSpeed x drift.amplitude** (corkscrew): Barrel roll + lateral weave creates helical flight. When both are modulated by different audio bands, verse/chorus transitions produce straight vs. spiraling flight.

## Notes

- The `disp()` function must remain shader-side (not CPU-computed via DualLissajousUpdate) because `map()` evaluates it at each sample point's z-position to center the density field around the flight path. The CPU passes the DualLissajousConfig field values as raw uniforms; the shader reconstructs the displacement.
- `DualLissajousConfig` is embedded in the config struct for serialization and UI reuse (`DrawLissajousControls`), but its `phase` field and `DualLissajousUpdate()` function are not used. The shader uses `flyPhase` (z-position) as the phase input.
- Dot Noise mode is more expensive than other density modes due to the matrix multiplications. At 80 march steps x 5 FBM octaves = 400 evaluations per pixel, the cost of 2 mat3*vec3 multiplies per evaluation adds up. Monitor performance on lower-end GPUs.
- The `GOLD` matrix for Dot Noise mode should be declared as a `const mat3` in the shader, not computed per-frame.
- Turbulence modes use the same argument structure as the current sine displacement: `p.zxy * 0.75 * trk + time * trk * .8`. The .zxy swizzle and scaling constants are kept verbatim from the original nimitz code.
- Some turbulence modes (Fract Fold, Square Wave, Quantized) produce discontinuous outputs. Combined with certain density modes, this can create hard edges in the volume. This is a feature, not a bug - it's what makes those modes visually distinct.
- Density mode names for the UI combo: "Gyroid", "Schwarz P", "Diamond", "Neovius", "Dot Noise"
- Turbulence mode names for the UI combo: "Sine", "Fract Fold", "Abs-Sin", "Triangle", "Squared Sin", "Square Wave", "Quantized", "Cosine"
