# Laser Dance Position Warp

Add position perturbation to the existing Laser Dance generator's raymarch loop, bending the rigid cosine lattice into organic tangled filaments before the distance field evaluates.

## Classification

- **Category**: GENERATORS > Filament (enhancement to existing Laser Dance)
- **Pipeline Position**: Same as Laser Dance — generator stage
- **Scope**: New config fields, uniforms, and one shader line. No new files.

## Attribution

- **Based on**: "Star Field Flight [351]" by diatribes (position perturbation technique)
- **Source**: https://www.shadertoy.com/view/3ft3DS
- **License**: CC BY-NC-SA 3.0
- **Lineage**: diatribes' shader is itself derived from Xor's "Laser Dance" (https://www.shadertoy.com/view/tct3Rf), which the existing effect is already based on.

## Reference Code

diatribes' full shader, showing the perturbation in context:

```glsl
void mainImage(out vec4 o, vec2 u) {
    float i,d,s,t = iTime*3.;
    vec3  p = iResolution;
    u = ((u-p.xy/2.)/p.y);
    for(o*=i; i++<80.;o += (1.+cos(d+vec4(4,2,1,0))) / s)
        p = vec3(u * d, d + t),
        p.xy *= mat2(cos(tanh(sin(t*.1)*4.)*3.+vec4(0,33,11,0))),
        p.xy -= vec2(sin(t*.07)*16., sin(t*.05)*16.),
        p += cos(t+p.y+p.x+p.yzx*.4)*.3,
        d += s = length(min( p = cos(p) + cos(p).yzx, p.zxy))*.3;
    o = tanh(o / d / 2e2);
}
```

The perturbation line:

```glsl
p += cos(t+p.y+p.x+p.yzx*.4)*.3
```

## Algorithm

Insert one line into the existing raymarch loop in `laser_dance.fs`, between the sample point calculation and the distance field evaluation:

```glsl
// Current (line 52-53):
vec3 p = z * rayDir + vec3(cameraOffset);

// NEW — position warp:
p += cos(time * warpSpeed + p.y + p.x + p.yzx * warpFreq) * warpAmount;

// Current (line 55-56):
vec3 q = cos(p + time) + cos(p / freqRatio).yzx;
float dist = length(min(q, q.zxy));
```

### What to keep from reference
- `cos(t + p.y + p.x + p.yzx * freq) * amount` — the perturbation structure verbatim
- `p.y + p.x` creates spatial variation; `p.yzx * freq` controls tightness

### What to replace
| Reference | Replacement |
|-----------|-------------|
| `t` (hardcoded `iTime*3.`) | `time * warpSpeed` (reuse existing time accumulator, scale by warp speed) |
| `.4` (hardcoded spatial frequency) | `warpFreq` uniform |
| `.3` (hardcoded amplitude) | `warpAmount` uniform |

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| warpAmount | float | 0.0-1.5 | 0.0 | Perturbation strength. 0 = clean beams (current behavior), higher = tangled organic filaments |
| warpSpeed | float | 0.1-3.0 | 1.0 | How fast the warp evolves. Multiplies existing time accumulator |
| warpFreq | float | 0.1-2.0 | 0.4 | Spatial tightness of the warp. Low = broad flowing bends, high = tight knotted tangles |

## Modulation Candidates

- **warpAmount**: intensity of the organic distortion — ramp up during choruses for chaos, drop for clean verses
- **warpSpeed**: rate of warp evolution — faster creates frantic motion, slower creates gentle drifting
- **warpFreq**: spatial density of the tangles — low values give broad sweeping bends, high values create fine knotted texture

### Interaction Patterns

- **warpAmount × freqRatio** (competing forces): `freqRatio` controls the regularity of the base lattice while `warpAmount` breaks it. Low freqRatio + high warpAmount creates maximally chaotic tangles; high freqRatio + low warpAmount gives clean geometric beams. Modulating both creates a tension between order and chaos.
- **warpFreq × warpAmount** (cascading threshold): At low `warpAmount`, changes to `warpFreq` are barely visible. As `warpAmount` increases past ~0.3, `warpFreq` starts dramatically changing the character — it gates the spatial complexity.

## Notes

- Default `warpAmount = 0.0` preserves the current Laser Dance look — this is purely additive.
- The perturbation adds one `cos()` evaluation per raymarch step (100 steps). Negligible cost.
- `warpSpeed` multiplies the existing `time` accumulator rather than needing its own — no new accumulators needed.
- UI: add a "Warp" separator section in the Laser Dance params, between Geometry and Audio.
