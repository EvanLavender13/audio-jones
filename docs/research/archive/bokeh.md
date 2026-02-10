# Bokeh

Simulates out-of-focus camera blur where bright spots bloom into soft circular highlights. Uses golden-angle spiral sampling weighted by brightness to create the characteristic "popping" bokeh orbs.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Samples input texture in Vogel disk pattern, weights by brightness
- **Pipeline Position**: Reorderable with other transforms

## References

- [GM Shaders - Phi/Golden Angle](https://mini.gmshaders.com/p/phi) - Complete Vogel disk sampling algorithm
- [David Hoskins - Bokeh Disc](https://www.shadertoy.com/view/4d2Xzw) - Brightness-weighted bokeh implementation

## Algorithm

### Golden Angle Disc Sampling

Distributes samples uniformly across a disc using the golden angle to minimize clustering.

**Constants:**
```glsl
#define GOLDEN_ANGLE 2.39996323  // tau * (2 - phi)
#define HALF_PI 1.5707963
```

**Sample Position Calculation:**
```glsl
// Direction uses cos with half-pi offset to get both x and y
vec2 dir = cos(float(i) * GOLDEN_ANGLE + vec2(0.0, HALF_PI));

// Square root distributes samples uniformly across disc area
float radius = sqrt(float(i) / float(numSamples)) * maxRadius;

vec2 sampleOffset = dir * radius;
```

**Complete Bokeh Blur:**
```glsl
vec3 BokehBlur(sampler2D tex, vec2 uv, float radius, int iterations)
{
    vec3 acc = vec3(0.0);
    vec3 div = vec3(0.0);

    for (int i = 0; i < iterations; i++)
    {
        vec2 dir = cos(float(i) * GOLDEN_ANGLE + vec2(0.0, HALF_PI));
        float r = sqrt(float(i) / float(iterations)) * radius;

        vec3 col = texture(tex, uv + dir * r).rgb;

        // Weight by brightness^4 for "popping" bokeh highlights
        vec3 weight = pow(col, vec3(4.0));

        acc += col * weight;
        div += weight;
    }

    return acc / max(div, vec3(0.001));
}
```

The brightness weighting (pow 4) causes bright pixels to dominate the average, creating the characteristic bright-spot bloom effect.

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| radius | float | 0.0 - 0.1 | Size of blur disc in UV space |
| iterations | int | 32 - 150 | Sample count (quality vs performance) |
| brightnessPower | float | 1.0 - 8.0 | Exponent for brightness weighting (higher = more "pop") |

## Audio Mapping Ideas

- **Beat → Radius**: Pulse blur size on kicks for dreamy throb
- **Bass Energy → Brightness Power**: Low frequencies increase highlight pop
- **RMS → Iterations**: Louder = higher quality (subtle)

## Implementation Notes

### Single-Pass Shader

Bokeh is a straightforward PostEffect - each pixel samples the input texture N times and outputs a weighted average. No intermediate render textures required.

### Performance

- 64 iterations: Good quality, ~64 texture samples per pixel
- 150 iterations: Reference quality, expensive
- Consider LOD/mipmap sampling for large radii to reduce aliasing

### Aspect Ratio

Multiply the sample offset x-component by aspect ratio to maintain circular bokeh on non-square resolutions:
```glsl
vec2 offset = dir * r;
offset.x *= resolution.y / resolution.x;
```
