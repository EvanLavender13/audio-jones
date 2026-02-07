# Pitch Spiral Enhancements

Adds perpetual rotation, organic radial breathing, and a parametric shape exponent to the existing pitch spiral generator. The spiral drifts, pulses, and morphs its curvature — transforming a static FFT readout into a living, hypnotic geometry.

## Classification

- **Category**: GENERATORS (existing effect enhancement)
- **Pipeline Position**: Generator stage, unchanged
- **Chosen Approach**: Minimal — 4 new parameters, ~20 shader lines, no structural changes

## References

- [Archimedean spiral — Wolfram MathWorld](https://mathworld.wolfram.com/ArchimedesSpiral.html) — Power-law generalization `r = a * theta^(1/n)` unifying Archimedean, Fermat, and hyperbolic spirals
- [Fermat's spiral — Wolfram MathWorld](https://mathworld.wolfram.com/FermatsSpiral.html) — Parabolic spiral `r^2 = a^2 * theta`, the n=2 case
- [GM Shaders Mini: Phi](https://mini.gmshaders.com/p/phi) — Golden-angle disk sampling with `sqrt(i/n)` radius normalization
- [2D Spiral Effect — Godot Shaders](https://godotshaders.com/shader/2d-spiral-effect/) — Logarithmic spiral distance field with rotation animation

## Algorithm

### 1. Perpetual Drift (rotation)

Accumulate a rotation offset on the CPU each frame:

```
rotationAccum += rotationSpeed * deltaTime
```

Pass `rotationAccum` as a uniform. In the shader, offset the polar angle before spiral computation:

```
angle = atan(uv.y, uv.x) + rotationAccum;
```

The pitch mapping stays correct because `whichTurn` and `cents` derive from the same shifted angle — the spiral rotates as a rigid body, and the FFT-to-pitch relationship holds at every orientation.

### 2. Organic Breathing (radial pulse)

Accumulate a breathing phase on the CPU:

```
breathAccum += breathRate * deltaTime
```

Compute a radial scale factor in the shader:

```
float breathScale = 1.0 + breathDepth * sin(breathAccum);
uv *= breathScale;
```

Applied before polar conversion, this uniformly scales the spiral inward/outward. A `breathDepth` of 0.1 gives ~10% expansion/contraction. The pitch mapping remains valid because scaling UV equivalently scales the radius used for turn detection.

### 3. Parametric Shape Exponent

Replace the linear Archimedean spiral `r ~ theta` with a power-law generalization:

```
// Current (Archimedean): offset = radius + (angle / TAU) * spacing
// Generalized: offset = pow(radius, shapeExponent) + (angle / TAU) * spacing
```

The exponent `shapeExponent` controls ring distribution:

| Exponent | Spiral Type | Visual Character |
|----------|-------------|------------------|
| 1.0 | Archimedean | Uniform spacing (current default) |
| 0.5 | Fermat-like | Tight core, expanding outer rings |
| 2.0 | Hyperbolic-like | Wide core, compressed edges |

The `whichTurn` computation remains `floor(offset / spacing)`, so pitch mapping adapts automatically — only the visual distribution of rings changes, not which frequencies they represent.

### Integration into Existing Shader

All three features insert before or at STEP 1 (spiral geometry). No changes to STEP 2 (pitch mapping), STEP 3 (drawing), or STEP 4 (output).

Insertion point — after UV computation, before `angle = atan(...)`:

1. Apply `breathScale` to UV
2. Compute `angle` with `rotationAccum` offset
3. Replace `radius` with `pow(radius, shapeExponent)` in the offset formula

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| rotationSpeed | float | -3.0 to 3.0 | 0.0 | Continuous spin rate (rad/s). Positive = CCW, negative = CW |
| breathRate | float | 0.1 to 5.0 | 1.0 | Breathing oscillation frequency (Hz) |
| breathDepth | float | 0.0 to 0.5 | 0.0 | Radial expansion amplitude (fraction of radius). 0 = disabled |
| shapeExponent | float | 0.3 to 3.0 | 1.0 | Power-law curvature. 1.0 = Archimedean (current behavior) |

## Modulation Candidates

- **rotationSpeed**: Varies spin rate — slow drift to rapid whirl
- **breathDepth**: Pulses the spiral in/out — subtle shimmer to dramatic throb
- **breathRate**: Changes breathing tempo — slow meditation to rapid flutter
- **shapeExponent**: Morphs spiral curvature — Fermat sunflower to hyperbolic funnel

## Notes

- Default values preserve current behavior — `rotationSpeed=0`, `breathDepth=0`, `shapeExponent=1.0` produce the identical output to the existing shader
- `rotationAccum` and `breathAccum` accumulate on the CPU in `PitchSpiralEffectSetup()`, matching the pattern used by other animated generators
- The shape exponent distorts visual spacing but not frequency mapping — a ring that "looks" wider still represents the same octave
- Performance impact: negligible — one `pow()`, one `sin()`, two multiplies per fragment
