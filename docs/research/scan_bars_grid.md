# Scan Bars Grid Mode

A hash-randomized 2D grid pattern added as mode 3 to the existing Scan Bars generator. Instead of 1D repeating stripes (linear, spokes, rings), grid mode produces a 2D lattice where cell boundaries are pseudo-randomly scattered by a hash function. The result is an organic, irregular grid that slowly morphs over time, with palette-driven color per cell and FFT-reactive brightness.

## Classification

- **Category**: GENERATORS > Scatter
- **Pipeline Position**: Same as Scan Bars (existing generator)
- **Scope**: New mode branch in `shaders/scan_bars.fs`, mode combo update in `scan_bars.cpp`

## Attribution

- **Based on**: "KuKo #343" by kukovisuals, hash function by @Fabrice
- **Source**: https://www.shadertoy.com/view/sclSRN
- **License**: CC BY-NC-SA 3.0

## References

- [KuKo #343](https://www.shadertoy.com/view/sclSRN) - Source shader with hash grid + feedback trail

## Reference Code

```glsl
// From @Fabrice
#define H(n) fract( 1e1 * sin( n.x+n.y/.7 +vec2(1,12.34) ) )

vec3 black = vec3(0);
vec3 red = vec3(0.22,0.8405,0.962);

float grid(vec2 p)
{
    vec2 o = abs(p);
    return min(o.x, o.y) - 0.02;
}

void mainImage( out vec4 O, vec2 I )
{
    vec2 uv = (2.0 * I - iResolution.xy) / iResolution.y;
    vec3 col = vec3(0);

    float d1 = grid(H(uv * vec2(3. + (iTime * 0.0023))));
    d1 = smoothstep(0.,0.01,d1);
    col = mix(red, black, d1);

    // Previous "smoke/light" field
    vec3 prev = texture(iChannel0, I / iResolution.xy).rgb * .89;
    if(iFrame==0) col = vec3(0);
    O = vec4(col + prev , 1.0);
}
```

## Algorithm

### Substitution Table

| Reference | Keep/Replace | Our Equivalent |
|-----------|-------------|----------------|
| `(2.0 * I - iResolution.xy) / iResolution.y` | Replace | `fragTexCoord - 0.5` with `uv.x *= resolution.x / resolution.y` (already in shader) |
| `3.0` (grid scale) | Replace | `barDensity` uniform |
| `iTime * 0.0023` (hash drift) | Replace | `scrollPhase` (CPU-accumulated: `scrollPhase += scrollSpeed * deltaTime`) |
| `H(n)` hash function | Keep | Verbatim: `fract(1e1 * sin(n.x + n.y / 0.7 + vec2(1, 12.34)))` |
| `grid(p)` SDF | Keep | Verbatim: `min(abs(p.x), abs(p.y)) - threshold` |
| `0.02` (line threshold) | Replace | `sharpness * 0.5` - maps sharpness range 0.0-1.0 to threshold 0.0-0.5 |
| `smoothstep(0., 0.01, d1)` | Replace | `smoothstep(0.0, sharpness * 0.25, gridDist)` - edge softness proportional to thickness |
| `mix(red, black, d1)` | Replace | LUT color sampling with chaos, masked by grid SDF (see below) |
| `iChannel0` feedback (0.89 decay) | Remove | Handled by blend compositor at pipeline level |

### Grid Mode Shader Branch (mode == 3)

Inside the existing mode switch in `main()`:

1. **Rotate UV** (reuse `angle`): `vec2 gridUV = rotate2D(angle) * uv`
2. **Apply convergence warp** to UV before hashing:
   ```
   gridUV.x += convergence * safeTan(abs(gridUV.x - convergenceOffset) * convergenceFreq)
   gridUV.y += convergence * safeTan(abs(gridUV.y - convergenceOffset) * convergenceFreq)
   ```
3. **Hash**: `vec2 h = H(gridUV * barDensity + scrollPhase)`
4. **Grid SDF**: `float gridDist = min(abs(h.x), abs(h.y)) - sharpness * 0.5`
5. **Mask**: `float mask = 1.0 - smoothstep(0.0, sharpness * 0.25, gridDist)` (invert so lines are bright)
6. **Color coord**: `coord = h.x + h.y` - feeds into existing STEP 3 (LUT chaos) and STEP 4 (FFT)

The H() hash function should be defined once near the top of the shader (after safeTan, before main).

### Mask Inversion Note

The reference uses `mix(red, black, d1)` where `d1=1` is away from lines (black) and `d1=0` is on lines (red). Our pipeline multiplies `color * mask * react`, so mask must be 1 on lines and 0 away. The `1.0 - smoothstep(...)` inversion achieves this.

### Changes Summary

- `shaders/scan_bars.fs`: Add `H()` hash function, add `mode == 3` branch in STEP 1, replace STEP 2 mask with grid SDF mask when `mode == 3`
- `src/effects/scan_bars.cpp`: Add `"Grid"` to mode combo (index 3), no new uniforms or params

## Parameters

No new parameters. Existing scan_bars params reused:

| Parameter | Grid Mode Mapping |
|-----------|-------------------|
| `barDensity` | Grid cell scale (replaces hardcoded 3.0) |
| `sharpness` | Grid line thickness and edge softness (replaces hardcoded 0.02/0.01) |
| `scrollSpeed` | Hash drift rate (replaces iTime * 0.0023) |
| `angle` | Grid rotation (applied to UV before hashing) |
| `convergence` | UV warping before hash (bunches grid cells) |
| `convergenceFreq` | Spatial frequency of convergence warping |
| `convergenceOffset` | Focal point of convergence |
| `colorSpeed` | LUT index drift rate (same as other modes) |
| `chaosFreq` | Color chaos frequency using hash-derived coordinate |
| `chaosIntensity` | How wildly adjacent cells jump across palette |
| `snapAmount` | Time quantization (same as other modes) |
| Audio params | Same FFT brightness mapping |

## Modulation Candidates

- **barDensity**: Coarser/finer grid cells. Low values = large sparse cells, high = dense mesh.
- **sharpness**: Line weight. Low = hairline traces, high = thick neon bars.
- **scrollSpeed**: Grid evolution rate. Creates morphing/breathing when modulated.
- **convergence**: Grid cell bunching. Creates focal density shifts.
- **chaosIntensity**: Color scatter across cells. Low = monochrome regions, high = rainbow chaos.

### Interaction Patterns

- **barDensity x convergence (competing forces)**: Higher density fills the viewport with cells, but convergence bunches them toward the focal point. The tension creates dense clusters surrounded by stretched voids. Audio on barDensity expands/contracts the grid, while convergence anchors the structure.
- **sharpness x gain (cascading threshold)**: With thin lines (low sharpness), only loud FFT bands produce visible traces. Thicker lines (high sharpness) remain visible even at low energy. Modulating sharpness with a slow LFO creates breathing line weight that gates how much of the audio spectrum is visible.

## Notes

- The `H()` hash is cheap (two sin + fract), no performance concern
- The hash produces repeating patterns at large UV scales, but barDensity range 1-50 keeps cells in the non-degenerate range
- Grid mode ignores `barCoord` scroll pattern from STEP 2 since it computes its own 2D mask; the `scrollPhase` uniform drives hash drift instead
- Frame feedback / trailing glow is achieved through the blend compositor (screen or additive blending), not in the shader itself
