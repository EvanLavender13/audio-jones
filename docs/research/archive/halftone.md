# Halftone

Simulates print halftoning by converting the image to CMYK, sampling each channel on a rotated grid, and rendering dots sized proportionally to ink density. Creates a classic newspaper/comic book aesthetic.

## Classification

- **Category**: TRANSFORMS → Color
- **Core Operation**: RGB→CMYK conversion, per-channel rotated grid sampling, radial dot generation, CMYK→RGB recombination
- **Pipeline Position**: Transform stage (user-ordered)

## References

- [libretro CMYK Halftone Dot](https://github.com/libretro/common-shaders/blob/master/misc/cmyk-halftone-dot.cg) - Public domain Cg shader adapted from Stefan Gustavson's WebGL demo
- [Shadertoy CMYK Halftone](https://www.shadertoy.com/view/Mdf3Dn) - Interactive CMYK halftone with mouse-controlled scale/rotation

## Algorithm

### RGB to CMYK Conversion

```glsl
vec4 rgb2cmyki(vec3 c) {
    float k = max(max(c.r, c.g), c.b);
    return min(vec4(c.rgb / k, k), 1.0);
}

vec3 cmyki2rgb(vec4 c) {
    return c.rgb * c.a;
}
```

This inverted CMYK representation stores CMY as ratios and K as the maximum channel value.

### Screen Angles

Standard printing angles prevent moiré interference:
- **Cyan**: 15°
- **Magenta**: 75°
- **Yellow**: 0°
- **Black (K)**: 45°

```glsl
mat2 rotm(float r) {
    float cr = cos(r);
    float sr = sin(r);
    return mat2(cr, -sr, sr, cr);
}

mat2 mc = rotm(rotation + radians(15.0));
mat2 mm = rotm(rotation + radians(75.0));
mat2 my = rotm(rotation);
mat2 mk = rotm(rotation + radians(45.0));
```

### Grid Sampling

For each CMYK channel, snap to rotated grid cell center and sample texture:

```glsl
vec2 grid(vec2 px, float S) {
    return px - mod(px, S);
}

vec4 halftone(vec2 fc, mat2 m, float S) {
    vec2 smp = (grid(m * fc, S) + 0.5 * S) * m;  // Rotate, grid, unrotate
    float s = min(length(fc - smp) / (DOTSIZE * 0.5 * S), 1.0);
    vec3 texc = texture(inputTex, (smp + origin) / resolution).rgb;
    texc = pow(texc, vec3(2.2));  // Gamma decode
    vec4 c = rgb2cmyki(texc);
    return c + s;  // Add distance to ink values
}
```

### Dot Generation with Smoothstep

Apply smoothstep for anti-aliased dot edges:

```glsl
vec4 ss(vec4 v, float threshold, float softness) {
    return smoothstep(threshold - softness, threshold + softness, v);
}

// Final combination
vec3 c = cmyki2rgb(ss(vec4(
    halftone(fc, mc, S).r,
    halftone(fc, mm, S).g,
    halftone(fc, my, S).b,
    halftone(fc, mk, S).a
), 0.888, 0.288));

c = pow(c, vec3(1.0 / 2.2));  // Gamma encode
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| dotScale | float | 2.0–20.0 | Grid cell size in pixels; larger = bigger dots |
| dotSize | float | 0.5–2.0 | Dot radius multiplier within cell |
| rotation | float | 0–2π | Base rotation applied to all screen angles |
| threshold | float | 0.5–1.0 | Smoothstep center for dot edge |
| softness | float | 0.1–0.5 | Smoothstep width; higher = softer edges |

## Audio Mapping Ideas

- **dotScale** ← Bass energy: Larger dots on bass hits
- **rotation** ← Slow rotation accumulation from mid frequencies
- **threshold** ← Overall volume: More ink coverage at higher volume

## Notes

- Gamma-correct color space: Decode before CMYK conversion, encode after recombination
- The inverted CMYK representation (K = max channel) differs from print-standard CMYK but produces visually correct results
- Screen angles are additive to base rotation; this allows animated rotation while preserving angle offsets
- Consider a "mono" mode using only K channel for simpler black-and-white halftone
