# Wave Ripple

Pseudo-3D radial wave displacement. Summed sine waves create a height field; gradient displaces UVs for parallax effect. Optional height-based shading sells the 3D illusion.

## Classification

- **Category**: Motion (reorderable transform)
- **Core Operation**: Radial sine-sum height field → UV displacement
- **Visual Outcome**: Concentric waves rippling from center point

## References

- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/) - Domain distortion via `f(g(p))` pattern
- [Catlike Coding - Waves](https://catlikecoding.com/unity/tutorials/flow/waves/) - Gerstner wave math, multi-component summation
- [Wikipedia - Gerstner Wave](https://en.wikipedia.org/wiki/Gerstner_wave) - Formal trochoidal equations, computer graphics extension
- User-provided Shadertoy - Sine-sum height function with hash-based phase offsets

## Algorithm

### Height Field (Radial Sine Sum)

Compute height at each UV from summed radial sine waves:

```glsl
float getHeight(vec2 uv, vec2 origin, float time) {
    float dist = length(uv - origin);
    float height = 0.0;

    float freq = baseFrequency;
    float amp = 1.0;

    for (int i = 0; i < octaves; i++) {
        height += sin(dist * freq - time * speed) * amp;
        freq *= 2.0;      // frequency doubles per octave
        amp *= 0.5;       // amplitude halves per octave
    }

    return height;
}
```

### Steepness (Gerstner Crest/Trough Asymmetry)

Apply steepness to create sharper crests, flatter troughs:

```glsl
// steepness in [0, 1], 0 = symmetric sine, 1 = sharp crests
float shaped = height - steepness * height * height;
```

Alternatively, use the trochoidal form directly:
```glsl
// Horizontal displacement component from Gerstner
float horizontal = (steepness / freq) * cos(dist * freq - time * speed);
```

### UV Displacement

Compute gradient via partial derivatives, displace UV:

```glsl
vec2 gradient = vec2(dFdx(height), dFdy(height));
vec2 displacedUV = uv + gradient * strength;
```

For radial waves, the gradient points toward/away from origin:
```glsl
vec2 dir = normalize(uv - origin);
vec2 displacedUV = uv + dir * height * strength;
```

### Height-Based Shading (Optional)

Modulate brightness based on height for pseudo-3D appearance:

```glsl
vec3 color = texture(input, displacedUV).rgb;
float shade = 1.0 + height * shadeIntensity;  // shadeIntensity ~0.1-0.3
color *= shade;
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| strength | float | 0.0 - 0.5 | UV displacement intensity |
| frequency | float | 1.0 - 20.0 | Wave density (waves per unit) |
| speed | float | 0.0 - 5.0 | Animation rate |
| octaves | int | 1 - 4 | Sine wave layers |
| steepness | float | 0.0 - 1.0 | Crest sharpness (0=sine, 1=trochoidal) |
| originX | float | 0.0 - 1.0 | Wave source X (0.5 = center) |
| originY | float | 0.0 - 1.0 | Wave source Y (0.5 = center) |
| shadeEnabled | bool | - | Toggle height-based brightness |
| shadeIntensity | float | 0.0 - 0.5 | Brightness modulation strength |

### Modulatable Parameters

Register in `param_registry.cpp`: strength, frequency, speed, steepness, originX, originY, shadeIntensity.

## Distinction from Existing Effects

| Effect | Pattern | Mechanism |
|--------|---------|-----------|
| Sine Warp | Directional, cascading | Per-octave rotation, UV scale |
| Texture Warp | Self-referential | Feedback sampling |
| **Wave Ripple** | Radial, concentric | Height field gradient displacement |

## Notes

- Single wave origin for v1; multiple emitters can be added later
- Single depth layer; multi-layer parallax is expensive and not essential
- Steepness > 0.7 may cause UV folding artifacts at high strength
- Origin drift animation (like Kaleidoscope focal offset) can be added via sin/cos modulation
