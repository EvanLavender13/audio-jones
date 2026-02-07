# Muons

Raymarched volumetric generator that produces glowing particle trails curving through 3D space — thin luminous filaments spiraling and forking like charged particles in a bubble chamber. Trails appear at varying depths with parallax, colored via a user-defined gradient LUT.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Generator scratch texture → BlendCompositor → transform chain
- **Chosen Approach**: Full — the original shader is already minimal (10 march steps, single pass), so there's little to cut. We add configurability on top.

## References

- [XorDev "Muons" shader](https://www.shadertoy.com/view/PLACEHOLDER) — Source shader (code provided by user, not fetchable)
- [Volumetric Raymarching — GM Shaders (XorDev)](https://mini.gmshaders.com/p/volumetric) — Density field marching, color accumulation by dividing by distance, dot-product sine turbulence

## Algorithm

The effect raymarches from each pixel into a 3D volume. At each step, it evaluates a turbulence-warped distance field of concentric spherical shells. Where the ray crosses a thin shell, it accumulates bright color. The turbulence bends the shells into chaotic curved paths that read as particle trails.

### Ray Setup

Per-pixel ray direction from screen UV, normalized against resolution:

```glsl
vec3 rayDir = normalize(vec3(uv * 2.0 - 1.0, 0.0) - vec3(0, 0, -1));
```

Camera sits at `z = -cameraDistance` looking forward. Each march step advances `z` along the ray.

### March Loop (per step)

1. **Sample point**: `p = z * rayDir`, offset by camera distance
2. **Rotation axis**: `a = normalize(cos(vec3(7, 1, 0) + time - s))` — time-varying axis that also depends on accumulated path length `s`, so each depth layer rotates differently
3. **Rodrigues rotation**: `a = a * dot(a, p) - cross(a, p)` — rotates the sample point around the axis, creating spiraling coordinate frames
4. **Turbulence accumulation** (N octaves):
   ```
   for d in 1..octaves:
       a += sin(a * d + time).yzx / d
   ```
   FBM-like layering with `.yzx` swizzle to break axis alignment. Higher octaves add finer branching. Amplitude falls off as `1/d`.
5. **Shell distance**: `s = length(a)`, then `d = thickness * abs(sin(s))` — concentric spherical shells in warped space. `abs(sin())` produces thin rings; `thickness` controls wire gauge.
6. **Step**: `z += d` (adaptive — steps shrink near shells for sharp crossings)

### Color Accumulation

At each step, accumulate:
```
color += sampleColor / d / s
```

- Dividing by `d` (step distance) brightens thin shell crossings — the thinner the ring, the brighter the flash
- Dividing by `s` (radial distance in warped space) attenuates distant structure
- `sampleColor` sampled from the gradient LUT at position `fract(z * colorFreq + time * colorSpeed)`

### Tonemapping

`tanh(accumulated / exposureScale)` — soft rolloff compresses huge brightness spikes into displayable range.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| speed | float | 0.0–2.0 | 0.3 | Animation rate of turbulence evolution and trail drift |
| marchSteps | int | 4–40 | 10 | Trail density — more steps reveal more filaments |
| turbulenceOctaves | int | 1–12 | 9 | Path complexity — fewer = smooth curves, more = chaotic branching |
| turbulenceStrength | float | 0.0–2.0 | 1.0 | Amplitude of turbulence displacement |
| ringThickness | float | 0.005–0.1 | 0.03 | Wire gauge of trails — smaller = thinner, sharper filaments |
| cameraDistance | float | 3.0–20.0 | 9.0 | Depth into the volume — closer zooms in, farther shows more trails |
| colorFreq | float | 0.5–50.0 | 33.0 | How fast color cycles along ray depth (higher = more rainbow banding) |
| colorSpeed | float | 0.0–2.0 | 0.5 | Rate color scrolls through the LUT over time |
| brightness | float | 0.1–5.0 | 1.0 | Overall intensity multiplier before tonemapping |
| exposure | float | 500–10000 | 3000 | Tonemapping divisor — lower = brighter/more saturated |
| gradient | ColorConfig | — | GRADIENT | Trail color palette via LUT |
| blendMode | EffectBlendMode | — | SCREEN | Compositing mode onto scene |
| blendIntensity | float | 0.0–5.0 | 1.0 | Compositing strength |

## Modulation Candidates

- **speed**: Animation tempo — slowing/surging trail evolution
- **turbulenceStrength**: Path chaos — calm smooth arcs vs frantic branching
- **ringThickness**: Trail weight — ghostly wisps vs bold strokes
- **brightness**: Overall glow intensity
- **colorSpeed**: Color drift rate through the palette
- **blendIntensity**: Fade the entire generator in/out

## Notes

- March step count directly impacts GPU cost. At 40 steps with 12 turbulence octaves, each pixel executes ~480 sin calls. Keep defaults conservative (10 steps × 9 octaves = 90 sin calls/pixel).
- The `abs(sin(length))` distance field produces evenly-spaced shells. Could explore `abs(sin(length * shellFreq))` as a future parameter to control shell spacing independently from thickness.
- The rotation axis formula `cos(vec3(7,1,0) + time - s)` uses magic constants from the original. These produce visually pleasing asymmetric rotation. No need to parameterize — they define the character of the effect.
- Adaptive step size (stepping by the shell distance) naturally concentrates samples near visible structure. This is why 10 steps produces surprisingly dense-looking results.
