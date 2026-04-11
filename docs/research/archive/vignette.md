# Vignette

Darkens (or lightens) the edges of the frame with configurable shape, creating depth and focus like a camera lens falloff or a spotlight effect.

## Classification

- **Category**: TRANSFORMS > Optical
- **Pipeline Position**: Transform chain (user-ordered), same slot as Bloom / Bokeh

## References

- [GPUImage Vignette (BradLarson)](https://github.com/BradLarson/GPUImage/blob/master/framework/Source/GPUImageVignetteFilter.m) - Clean smoothstep with center/start/end/color
- [TyLindberg/glsl-vignette](https://github.com/TyLindberg/glsl-vignette) - Advanced version with SDF-based roundness control (based on iq distance functions)
- [spite/Wagner vignette](https://github.com/spite/Wagner/blob/master/fragment-shaders/vignette-fs.glsl) - Minimal falloff + amount parameterization
- [FXShaders ArtisticVignette](https://github.com/luluco250/FXShaders/blob/master/Shaders/ArtisticVignette.fx) - Shape modes (radial, box, directional) with blend mode selection

## Reference Code

GPUImage vignette (desktop version):

```glsl
uniform sampler2D inputImageTexture;
varying vec2 textureCoordinate;

uniform vec2 vignetteCenter;
uniform vec3 vignetteColor;
uniform float vignetteStart;
uniform float vignetteEnd;

void main()
{
    vec4 sourceImageColor = texture2D(inputImageTexture, textureCoordinate);
    float d = distance(textureCoordinate, vec2(vignetteCenter.x, vignetteCenter.y));
    float percent = smoothstep(vignetteStart, vignetteEnd, d);
    gl_FragColor = vec4(mix(sourceImageColor.rgb, vignetteColor, percent), sourceImageColor.a);
}
```

TyLindberg advanced vignette with SDF roundness:

```glsl
float sdSquare(vec2 point, float width) {
    vec2 d = abs(point) - width;
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float vignette(vec2 uv, vec2 size, float roundness, float smoothness) {
    uv -= 0.5;
    float minWidth = min(size.x, size.y);
    uv.x = sign(uv.x) * clamp(abs(uv.x) - abs(minWidth - size.x), 0.0, 1.0);
    uv.y = sign(uv.y) * clamp(abs(uv.y) - abs(minWidth - size.y), 0.0, 1.0);
    float boxSize = minWidth * (1.0 - roundness);
    float dist = sdSquare(uv, boxSize) - (minWidth * roundness);
    return 1.0 - smoothstep(0.0, smoothness, dist);
}
```

Wagner minimal vignette:

```glsl
uniform sampler2D tInput;
uniform float falloff;
uniform float amount;
varying vec2 vUv;

void main() {
  vec4 color = texture2D(tInput, vUv);
  float dist = distance(vUv, vec2(0.5, 0.5));
  color.rgb *= smoothstep(0.8, falloff * 0.799, dist * (amount + falloff));
  gl_FragColor = color;
}
```

## Algorithm

Combine GPUImage's clean color-mixing approach with the TyLindberg SDF for shape control:

- Keep the GPUImage `mix(sourceColor, vignetteColor, percent)` blend verbatim - this allows both darkening (black color) and lightening (white color) or tinted vignettes
- Keep the TyLindberg `sdSquare` + roundness approach for shape - `roundness = 1.0` gives circular, `roundness = 0.0` gives rectangular
- Replace `texture2D` with `texture()`
- Use `fragTexCoord` for UV
- Distance calculation in UV space (0-1), not pixel space
- Compute distance using the SDF, then `smoothstep(0.0, softness, dist)` for the falloff
- Multiply by `intensity` to control how far toward vignetteColor the edges blend

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| intensity | float | 0.0-1.0 | 0.5 | How strongly edges blend toward the vignette color |
| radius | float | 0.0-1.0 | 0.5 | Distance from center where darkening begins |
| softness | float | 0.01-1.0 | 0.4 | Width of the falloff gradient - low is hard edge, high is gradual |
| roundness | float | 0.0-1.0 | 1.0 | Shape: 0=rectangular, 1=circular |

## Modulation Candidates

- **intensity**: vignette pulses with audio - edges darken on beats, lighten between them
- **radius**: the clear center area breathes in and out - large radius on quiet passages shows the full image, small radius on loud sections creates a spotlight
- **softness**: hard vs gradual edge - snappy staccato transitions vs smooth breathing

### Interaction Patterns

- **Competing forces (radius vs intensity)**: Large radius with high intensity creates a narrow dark frame. Small radius with high intensity creates a spotlight. Audio driving radius down (shrinking the clear area) while intensity rises (darkening more) creates a dramatic iris-closing effect on beats, then opening back up in quiet passages.

## Notes

- Extremely cheap - one distance calculation and a smoothstep per pixel. No loops, no extra texture lookups.
- The SDF roundness approach (from iq's distance functions) is elegant - it smoothly interpolates between circle and rounded rectangle using a single parameter.
- Vignette color as a vec3 uniform allows creative uses beyond darkening: white for a light leak, colored for a tinted frame.
- This is probably the simplest effect in the entire codebase. Good candidate for a first implementation if batching all five.
