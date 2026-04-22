---
name: Escher Droste
description: True self-tiling Droste warp - input texture repeats infinitely at log-spaced scales along a spiral, distinct from the existing spiral-zoom Droste
---

# Escher Droste

The input image recurses into itself along a logarithmic spiral. Each turn of the spiral, the same image reappears at scale 1/k, so looking toward the vanishing point reveals infinite nested copies of the frame. The Escher warp adds a rotation through log-polar space, turning the pure zoom-only Droste into the classic "Print Gallery" spiral. A continuous inward zoom reveals the tile structure.

## Classification

- **Category**: TRANSFORMS > Motion (`MOT`, section 3)
- **Pipeline Position**: Warp-style transform, same stage as `droste_zoom`. Reads the incoming stage texture, writes warped output.

## Attribution

- **Based on**: "Escher's Droste warp" by perlinw
- **Source**: https://www.shadertoy.com/view/sfBSWm
- **License**: CC BY-NC-SA 3.0
- **Original inspiration**: 3Blue1Brown, "What does it feel like to invent math?" / complex mapping f(z) = z^c (https://www.youtube.com/watch?v=ldxFjLJ3rVY)

## References

- Shadertoy shader linked above - source of the log-polar + Escher spiral transform with seam-corrected `textureGrad` sampling
- Bart de Smit & Hendrik Lenstra, "The Mathematical Structure of Escher's Print Gallery" (AMS Notices, 2003) - mathematical basis for the complex-log warp used in the shader

## Reference Code

```glsl
// based on 3blue1brown's explanation of complex mapping f(z) = z^c
// at: www.youtube.com/watch?v=ldxFjLJ3rVY

#define TWO_PI 6.28318

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    //scale factor k
    //of which Escher used 256
    float k = 256.0;
    float L = log(k);

    //
    //log-polar space transform
    float r = length(uv);
    float a = atan(uv.y, uv.x);
    float u = log(r);
    float v = a;

    //apply Escher warp
    //converts zoom-only Droste into a spiral Droste
    float warpU = u + v * L / (TWO_PI);
    float warpV = v - u * L / (TWO_PI);

    //
    //zoom inf.
    warpU -= iTime * 0.5;

    //div by periods to map back, and tile
    vec2 texCoord = vec2(warpU / L, warpV / (TWO_PI));

    //note that this causes a seam at the jump between 0 and 2pi,
    //so u could instead use:
    //vec3 col = textureLod(iChannel0, texCoord, 0.0).rgb;
    //but that in turn will cause shimmering artefacts at deeper zoom depths
    //so
    //vec3 col = texture(iChannel0, fract(texCoord)).rgb;

    //as suggested by @Bingle
    vec2 dx = dFdx(texCoord);
    vec2 dy = dFdy(texCoord);
    dx -= floor(dx + 0.5);
    dy -= floor(dy + 0.5);

    vec3 col = textureGrad(iChannel0, texCoord, dx, dy).rgb;


    //col *= smoothstep(0.0, 0.01, r);
    fragColor = vec4(col, 1.0);
}
```

## Algorithm

Transcribe the reference mechanically, applying the substitution table below. Do not reinterpret.

### Substitution table (reference -> codebase)

| Reference | Replacement | Notes |
|-----------|-------------|-------|
| `fragCoord - 0.5 * iResolution.xy` | `fragTexCoord * resolution - 0.5 * resolution - center` | Screen-centered pixel coords with optional 2D center drift from `DualLissajousConfig` |
| `/ iResolution.y` | `/ resolution.y` | Aspect-correct normalization, same as reference |
| `iResolution.xy` | `resolution` | vec2 uniform |
| `iTime * 0.5` | `zoomPhase` | CPU-accumulated phase: `cfg->zoomPhase += cfg->zoomSpeed * deltaTime` (speed accumulation rule) |
| `k = 256.0` hard-coded | `scale` uniform | Modulatable parameter |
| `L / (TWO_PI)` in warpU/V | `(L / TWO_PI) * spiralStrength` | Parameterized: `0` = zoom-only Droste tiling, `1` = Escher spiral (reference), signed for opposite twist |
| `iChannel0` | `texture0` | Input stage texture |
| (not in reference) | `warpV += rotationOffset` before dividing by TWO_PI | Static angle rotation of the tiled pattern |
| `//col *= smoothstep(0.0, 0.01, r)` commented out | `col *= (innerRadius > 0.0) ? smoothstep(0.0, innerRadius, length(uv)) : 1.0` | Fade mask for the singular center (parameter controls radius, 0 disables) |

### Keep verbatim

- `float r = length(uv); float a = atan(uv.y, uv.x); float u = log(r); float v = a;` - log-polar transform
- `warpU = u + v * (L / TWO_PI) * spiralStrength; warpV = v - u * (L / TWO_PI) * spiralStrength;` - Escher warp structure, with `spiralStrength` scalar applied to the coupling
- `vec2 texCoord = vec2(warpU / L, warpV / TWO_PI);` - map back to tile UV space
- Derivative seam correction (`dx -= floor(dx + 0.5); dy -= floor(dy + 0.5);`) and `textureGrad` call - required to hide the atan2 seam at theta = +/- pi without shimmering at deep zoom

### Coordinate convention

Per the codebase coordinate rule: `fragTexCoord` is (0,0) at bottom-left. The reference's `uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y` already centers coordinates, so transcribe it as `uv = (fragTexCoord * resolution - 0.5 * resolution - center) / resolution.y` in our shader. All spatial math (log, atan2, length) uses `uv` in centered space. The output tile `texCoord` stays in its [0,1]-tiling log-polar UV space - no re-centering needed because `textureGrad` expects raw UVs.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `scale` | float | 4.0 - 1024.0 | 256.0 | Tile scale factor k. Larger = deeper recursion visible per screen |
| `zoomSpeed` | float | -2.0 - 2.0 | 0.5 | Signed log-radial zoom rate. Positive zooms in, negative out |
| `spiralStrength` | float | -2.0 - 2.0 | 1.0 | 0 = zoom-only Droste tiling, 1 = Escher spiral, negative flips twist |
| `rotationOffset` | float (radians) | -PI to +PI | 0.0 | Static angle, rotates whole tiling pattern |
| `center` | DualLissajousConfig | per config | amplitude 0 | 2D drift of the tiling focus point |
| `innerRadius` | float | 0.0 - 0.5 | 0.05 | Smoothstep fade radius near singular center. 0 disables mask |

## Modulation Candidates

- **zoomSpeed**: continuous signed rate, ideal for bass pump (faster inward on kicks) or envelope-followed tempo
- **spiralStrength**: character shift between straight falling and swirling into a drain
- **rotationOffset**: slow wobble changes where the spiral "points," shifts the whole composition
- **scale**: larger values deepen the recursion; modulation creates breathing tile density
- **innerRadius**: open up on quieter sections to reveal the vortex, clamp down on loud sections to mask it

### Interaction Patterns

- **spiralStrength x zoomSpeed (character shift)**: Zoom is radial motion, spiral is rotational-per-log-radius coupling. With spiralStrength near 0, zooming feels like falling straight into the center. With spiralStrength near 1, zooming becomes a rotational drain. Modulating spiralStrength while zoomSpeed stays constant changes the *character* of motion - not the rate, but what kind of motion it is. Mapping spiralStrength to a slow LFO and zoomSpeed to audio energy decouples "how fast is the music" from "does it spin or fall."

- **centerOffset x zoomSpeed (resonance)**: When the center drifts and zoom is fast, the vanishing point diverges from where the frame is falling toward - the whole tile sheet lurches sideways. At slow zoom the drift is imperceptible. Modulating center drift amplitude with audio creates rare "wrenched" moments only at high-energy passages.

- **scale x innerRadius (threshold)**: Large scale pulls more recursion into view near the center, where the log-polar math goes singular. Small innerRadius exposes that singularity; large innerRadius hides it. Gating innerRadius on a slow envelope follower while scale is audio-driven means quiet sections reveal the singular vortex, loud sections mask it behind the fade.

## Notes

- **Seam handling is load-bearing**: the `dx -= floor(dx + 0.5)` trick on `dFdx/dFdy` results is what makes deep zoom look clean. Do not replace `textureGrad` with `texture(..., fract(...))` - that alternative shimmers at deep recursion (the Shadertoy comments document this).
- **Singular center**: `log(r)` diverges at `r = 0`. The `innerRadius` smoothstep is mandatory for aesthetic results. The reference clamps `r` implicitly via `smoothstep`; we do the same.
- **Tile source**: in our pipeline the "tile" IS the previous stage's output. The effect works best when the incoming frame has bold, recognizable structure - the whole point is seeing the frame repeat. Chained after a simple generator it produces the Print Gallery look; chained after chaotic output it produces a disorienting fractal smear.
- **No branches / shearCoeff**: deliberately omitted. The existing `droste_zoom` covers the kaleidoscopic-spiral look; this effect is the straight Escher tiling. Keeping it lean preserves the visual distinction between the two.
- **Shares no shader code with `droste_zoom.fs`**: the existing effect samples back into Cartesian space (spiral warp). This effect samples in tiled UV space (true recursion). Same math in the middle, different endings.
