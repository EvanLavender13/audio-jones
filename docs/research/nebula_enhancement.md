# Nebula Enhancement

Adds a noise type selector (kaliset vs domain-warped FBM), FBM-based dust lanes that carve dark absorption ribbons into the gas, and diffraction cross-spikes on bright stars. Existing parallax layers, FFT-reactive stars, and gradient LUT coloring remain unchanged.

## Classification

- **Category**: GENERATORS > Atmosphere (existing effect enhancement)
- **Pipeline Position**: Unchanged — output stage generators
- **Chosen Approach**: Balanced — adds FBM mode and two visual features without restructuring the 3-layer parallax system

## References

- `shaders/domain_warp.fs` — Existing codebase FBM with hash/noise/fbm functions
- `shaders/nebula.fs` — Current kaliset fractal implementation
- User-provided Shadertoy "signal/noise" — Domain-warped FBM gas, dust lanes, diffraction spikes, interference pulses (only gas/dust/spikes adopted)

## Reference Code

### Domain-warped FBM gas (from signal/noise shader)

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1, 0)), f.x),
        mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x),
        f.y
    );
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    mat2 rot = mat2(0.8, 0.6, -0.6, 0.8);
    for (int i = 0; i < 6; i++) {
        v += a * noise(p);
        p = rot * p * 2.0;
        a *= 0.5;
    }
    return v;
}

// 3-layer nested domain warping for cosmic dust
vec2 q = vec2(fbm(uv * 2.0 + time * 0.08), fbm(uv * 2.0 + vec2(5.2, 1.3)));
vec2 rr = vec2(fbm(uv * 2.0 + 4.0 * q + vec2(1.7, 9.2) + 0.12 * time),
               fbm(uv * 2.0 + 4.0 * q + vec2(8.3, 2.8) + 0.1 * time));
float f = fbm(uv * 2.0 + 4.0 * rr);
```

### Dust lanes (from signal/noise shader)

```glsl
float dust = fbm(uv * 3.5 + vec2(time * 0.05, -time * 0.03));
float dustLanes = smoothstep(0.35, 0.55, dust) * 0.6;

// Applied as darkening mask on gas color:
vec3 darkDust = vec3(0.02, 0.015, 0.03);
col = mix(col, darkDust, dustLanes * 0.7);
```

### Diffraction spikes (from signal/noise shader)

```glsl
// Inside the star loop, for stars above a brightness threshold:
if (h > 0.96) {
    float angle = atan(starF.y, starF.x);
    spikes += pow(max(cos(angle * 2.0), 0.0), 20.0) * exp(-starDist * 8.0) * 0.3;
    spikes += pow(max(cos(angle * 2.0 + PI * 0.5), 0.0), 20.0) * exp(-starDist * 8.0) * 0.3;
}
```

### Existing kaliset field (from nebula.fs)

```glsl
#define FOLD_OFFSET vec3(-0.5, -0.4, -1.5)

float field(vec3 p, float s, int iterations) {
    float strength = 7.0 + 0.03 * log(1.e-6 + fract(sin(time) * 4373.11));
    float accum = s / 4.0;
    float prev = 0.0;
    float tw = 0.0;
    for (int i = 0; i < iterations; i++) {
        float mag = dot(p, p);
        p = abs(p) / mag + FOLD_OFFSET;
        float w = exp(-float(i) / 7.0);
        accum += w * exp(-strength * pow(abs(mag - prev), 2.2));
        tw += w;
        prev = mag;
    }
    return max(0.0, 5.0 * accum / tw - 0.7);
}
```

## Algorithm

### Noise type selector

Add `uniform int noiseType;` (0 = kaliset, 1 = FBM). Both modes evaluate per-layer, reusing the existing 3-layer parallax structure.

**Kaliset mode (noiseType == 0):** Keep `field()` verbatim from existing code.

**FBM mode (noiseType == 1):** Add `hash()`, `noise()`, `fbm()` from reference code (with octave rotation matrix). Evaluate the 3-layer nested domain warp per depth layer:
- Replace `field(p, s, iterations)` call with `fbm(layerUV * 2.0 + 4.0 * rr)` using the nested warp pattern
- The `frontScale`/`midScale`/`backScale` uniforms control UV divisor for each layer (same as kaliset mode)
- `frontIter`/`midIter`/`backIter` become the FBM octave count per layer (reuse the uniforms, clamp to 2-8 range in FBM mode)
- The LUT color mapping stays identical — `t` value from either noise type feeds the same `texture(gradientLUT, vec2(...))` calls

**Branching approach:** Use a single `if (noiseType == 0) { ... } else { ... }` per layer evaluation. GPU branch divergence is not a concern since all fragments take the same path (uniform-based branch).

### Dust lanes

Always active in both modes. Add FBM-based absorption after gas color computation:

- Evaluate `fbm(layerUV * dustScale + dustDrift)` once (not per-layer — single global dust mask)
- `dustDrift` derives from the existing `time` uniform with slow coefficients
- Apply as darkening: `gasColor = mix(gasColor, darkTint, dustLanes * dustStrength)`
- `darkTint` is a near-black color sampled from the low end of the gradient LUT: `texture(gradientLUT, vec2(0.02, 0.5)).rgb * 0.1`

### Diffraction spikes

Add to `starLayer()` function. For each star that passes the brightness threshold:

- Compute `angle = atan(sp.y, sp.x)`
- Add two perpendicular spike lobes: `pow(max(cos(angle * 2.0), 0.0), spikeSharpness)` and the same offset by PI/2
- Modulate by `exp(-d2 * spikeDecay)` for falloff from star center
- Multiply by `spikeIntensity` uniform
- Add spike contribution to the star's color output (same tint as the star core)

## Parameters

### New parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| noiseType | int | 0-1 | 0 | 0 = kaliset fractal, 1 = domain-warped FBM |
| dustScale | float | 1.0-8.0 | 3.5 | FBM frequency for dust lanes — higher = finer ribbons |
| dustStrength | float | 0.0-1.0 | 0.4 | Opacity of dark absorption lanes |
| dustEdge | float | 0.05-0.3 | 0.1 | Smoothstep width — sharper or softer lane boundaries |
| spikeIntensity | float | 0.0-2.0 | 0.5 | Brightness of diffraction cross flares |
| spikeSharpness | float | 5.0-40.0 | 20.0 | Exponent controlling spike thinness |

### Existing parameters (unchanged)

All current `NebulaConfig` fields remain. `frontIter`/`midIter`/`backIter` reinterpret as FBM octave counts (clamped 2-8) when `noiseType == 1`.

## Modulation Candidates

- **dustStrength**: Modulate to reveal/hide dark lanes with audio — quiet passages show more gas, loud sections carve darker absorption
- **dustScale**: Shifts between broad dark regions and fine ribbon detail
- **spikeIntensity**: Pulse diffraction flares to the beat
- **noiseType**: Integer modulation snaps between kaliset and FBM modes — dramatic visual shift per song section

### Interaction Patterns

- **Cascading Threshold**: `dustStrength` gates gas visibility — as dust rises, gas that was previously bright gets carved into isolated bright islands. Stars shine through the dark lanes unaffected, so high dust makes stars the dominant visual while low dust lets gas dominate. The balance between gas and stars shifts with this single parameter.
- **Competing Forces**: `dustScale` vs layer `scale` parameters — fine dust ribbons on coarse gas layers create moiré-like interference patterns, while matched scales produce coherent dark regions. The visual texture shifts depending on which scale dominates.

## Notes

- FBM mode is cheaper than kaliset at low octave counts (5 octaves vs 20+ kaliset iterations) but the 3-layer nested domain warp calls `fbm()` ~7 times per layer (21 total), each with N octaves. At 6 octaves that's 126 noise evaluations per fragment — comparable to kaliset's cost.
- Dust lanes add one `fbm()` call (~6 noise evaluations) regardless of mode. Negligible compared to gas computation.
- Diffraction spikes add ~4 trig calls per bright star. Only triggers for stars above the hash threshold (~4% of grid cells), so cost is minimal.
- The `hash()` / `noise()` / `fbm()` functions match the existing `domain_warp.fs` implementation with one addition: the inter-octave rotation matrix `mat2(0.8, 0.6, -0.6, 0.8)` that reduces axis-aligned banding. This rotation is standard practice (IQ's article) and produces visibly softer clouds.
