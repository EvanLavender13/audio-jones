# Impressionist

Painterly effect that renders the image as overlapping circular brush dabs. Each splat samples color from the input, applies internal stroke hatching, and includes a subtle outline. Edge darkening and paper grain complete the hand-painted look.

Based on the post-processing pass from a Shadertoy cherry blossom demo.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Grid-scattered circular splats sampling input colors, with internal stroke texture, edge detection darkening, and paper grain overlay.
- **Pipeline Position**: Transform stage (user-ordered with other transforms)

## References

- User-provided Shadertoy code (cherry blossom tree with painterly post-process)

## Algorithm

### Grid-Based Splat Placement

Divides screen into grid cells, places multiple random splats per cell:

```glsl
for (int n = 0; n < 2; ++n) {                    // Two passes: large then small splats
    float maxr = mix(1./22., 1./4., 1.-float(n)) * 0.4;  // Size range per pass
    ivec2 cell = ivec2(floor(uv / maxr));

    for (int i = -1; i < 2; ++i)                 // 3x3 neighbor cells
        for (int j = -1; j < 2; ++j) {
            for (int k = 0; k < 11; ++k) {       // 11 splats per cell
                vec2 splatPos = (cell + vec2(rand(), rand())) * maxr;
                // ... render splat
            }
        }
}
```

### Individual Splat Rendering

Each splat:
1. Samples input color at splat center
2. Computes distance from current pixel to splat center
3. Applies internal stroke texture
4. Blends with soft edge and outline

```glsl
vec3 splatColor = texture(input, splatPos).rgb;
float r = mix(0.25, 0.99, pow(rand(), 4.)) * maxr;  // Random radius
float d = length(pixelPos - splatPos);

// Rotate for stroke direction
float ang = rand() * PI * 2.0;
vec2 rotated = rotate2D(pixelPos - splatPos, ang);

// Internal stroke lines
float stroke = cos(rotated.x * 1200.0) * 0.5 + 0.5;

// Soft circle fill
float fill = clamp((r - d) / feather, 0.0, 1.0);
col = mix(col, splatColor, opacity * stroke * fill);

// Circle outline
float outline = clamp(1.0 - abs(r - d) / 0.002, 0.0, 1.0);
col = mix(col, splatColor * 0.5, outlineStrength * outline);
```

### Edge Darkening

Gradient-based edge detection darkens areas with high contrast:

```glsl
vec2 e = vec2(0.001, 0.0);
vec2 jittered = uv + (noise(uv * 18.0) - 0.5) * 0.01;  // Wobble for hand-drawn feel

float c0 = luminance(texture(input, jittered).rgb);
float c1 = luminance(texture(input, jittered + e.xy).rgb);
float c2 = luminance(texture(input, jittered + e.yx).rgb);

float edge = max(abs(c1 - c0), abs(c2 - c0));
col *= mix(0.1, 1.0, 1.0 - clamp(edge * edgeStrength, 0.0, maxDarken));
```

### Paper Grain

High-frequency noise for canvas/paper texture:

```glsl
float grain = mix(0.9, 1.0, valueNoise(uv * grainScale));
col = sqrt(col * grain) * exposure;  // Gamma + grain + exposure
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| splatCount | int | 4-16 | Splats per grid cell |
| splatSizeMin | float | 0.01-0.1 | Minimum splat radius |
| splatSizeMax | float | 0.05-0.25 | Maximum splat radius |
| strokeFreq | float | 400-2000 | Internal hatching line density |
| strokeOpacity | float | 0-1 | Visibility of internal strokes |
| outlineStrength | float | 0-0.5 | Darkness of splat outlines |
| edgeStrength | float | 0-8 | Edge darkening intensity |
| edgeMaxDarken | float | 0-0.3 | Maximum edge darkening amount |
| grainScale | float | 100-800 | Paper texture frequency |
| grainAmount | float | 0-0.2 | Paper texture intensity |

## Modulation Candidates

- **splatSizeMax**: Larger splats create looser, more abstract rendering. Pulsing with bass creates breathing brush strokes.
- **strokeFreq**: Higher frequency = finer internal hatching. Could create shimmering texture variation.
- **edgeStrength**: More edge darkening emphasizes contours. Reactive edges could highlight transients.
- **strokeOpacity**: Fading strokes in/out changes from flat color dabs to textured strokes.

## Notes

- Two-pass approach (large splats then small) builds up coverage without excessive overdraw.
- The 3x3 neighbor cell check ensures splats near cell boundaries render correctly.
- Random rotation per splat (`ang`) varies stroke direction—creates organic feel.
- Original uses brightness to influence splat opacity (`pow(a, 0.8)`)—brighter areas get more visible splats.
- Performance scales with splatCount × grid cells. May need LOD for high resolutions.
- Distinct from Oil Paint (which smooths) and Watercolor (which bleeds). This preserves visible discrete brush marks.
