# Watercolor

Transforms an input image into a watercolor painting through gradient-flow stroke tracing. Strokes trace along luminance edges creating visible brush marks, while color migrates from dark to bright areas simulating wet paint wash. Paper granulation completes the effect.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Multi-sample gradient tracing for stroke outlines + color wash
- **Pipeline Position**: Output stage transforms (user-ordered with other Style effects)

## References

- [KinoAqua (Keijiro Takahashi)](https://github.com/keijiro/KinoAqua) - Unity post-process implementation of Flockaroo's technique
- Flockaroo (Florian Berger) Shadertoy "watercolor" - Gradient-flow stroke tracing with color wash (code provided by user, CC BY-NC-SA 3.0)

## Algorithm

### Gradient Computation

Central-difference luminance gradient at a given pixel position. The gradient points from dark toward bright.

```glsl
vec2 getGrad(vec2 pos, float delta)
{
    vec2 d = vec2(delta, 0.0);
    return vec2(
        dot((texture(inputTex, (pos + d.xy) / resolution).rgb -
             texture(inputTex, (pos - d.xy) / resolution).rgb), vec3(0.333)),
        dot((texture(inputTex, (pos + d.yx) / resolution).rgb -
             texture(inputTex, (pos - d.yx) / resolution).rgb), vec3(0.333))
    ) / delta;
}
```

### Stroke Loop (core algorithm)

Two pairs of trace positions per pixel: one pair traces perpendicular to gradient (outlines), another traces along gradient (color wash).

```glsl
#define SAMPLES 24

vec3 outlineAccum = vec3(0.0);
vec3 washAccum = vec3(0.0);
float outlineWeight = 0.0;
float washWeight = 0.0;

vec2 posA = pixelPos;  // outline trace forward
vec2 posB = pixelPos;  // outline trace backward
vec2 posC = pixelPos;  // wash trace (dark -> bright)
vec2 posD = pixelPos;  // wash trace (bright -> dark)

for (int i = 0; i < SAMPLES; i++)
{
    float falloff = 1.0 - float(i) / float(SAMPLES);

    // Gradient at each trace position
    vec2 grA = getGrad(posA, 2.0) + 0.0001 * (noise(posA) - 0.5);
    vec2 grB = getGrad(posB, 2.0) + 0.0001 * (noise(posB) - 0.5);
    vec2 grC = getGrad(posC, 2.0) + 0.0001 * (noise(posC) - 0.5);
    vec2 grD = getGrad(posD, 2.0) + 0.0001 * (noise(posD) - 0.5);

    // Outlines: step perpendicular to gradient (tangent direction)
    posA += strokeStep * normalize(vec2(grA.y, -grA.x));
    posB -= strokeStep * normalize(vec2(grB.y, -grB.x));

    float edgeStrength = clamp(10.0 * length(grA), 0.0, 1.0);
    float threshold = smoothstep(0.9, 1.1, luminance(posA) * 0.9 + noisePattern(posA));
    outlineAccum += falloff * mix(vec3(1.2), vec3(threshold * 2.0), edgeStrength);
    outlineWeight += falloff;

    // Color wash: step along gradient (dark areas lose color, bright areas gain)
    posC += 0.25 * normalize(grC) + 0.5 * (noise(pixelPos * 0.07) - 0.5);
    posD -= 0.50 * normalize(grD) + 0.5 * (noise(pixelPos * 0.07) - 0.5);

    float w1 = 3.0 * falloff;
    float w2 = 4.0 * (0.7 - falloff);
    washAccum += w1 * (sampleColor(posC) + 0.25 + 0.4 * noise(posC));
    washAccum += w2 * (sampleColor(posD) + 0.25 + 0.4 * noise(posD));
    washWeight += w1 + w2;
}

vec3 outline = outlineAccum / (outlineWeight * 2.5);
vec3 wash = washAccum / (washWeight * 1.65);

// Combine: outlines modulate wash color
vec3 result = clamp(clamp(outline * 0.9 + 0.1, 0.0, 1.0) * wash, 0.0, 1.0);
```

Key behaviors:
- Outline strokes follow edge tangents, creating visible directional brush marks
- Wash traces migrate color from dark regions to bright, simulating wet paint flow
- Noise perturbation prevents mechanical uniformity
- Higher SAMPLES = more defined strokes but more expensive

### Paper Texture

Multiply final color by paper grain. Centered around 1.0 so it both lightens and darkens (unlike the old implementation which only darkened):

```glsl
float paper = fbmNoise(uv * paperScale);
color *= mix(vec3(1.0), vec3(0.93, 0.93, 0.85) * (paper * 0.6 + 0.7), paperStrength);
```

The warm tint `(0.93, 0.93, 0.85)` simulates aged watercolor paper.

### Combined Pipeline

```glsl
void main()
{
    // 1. Stroke tracing - produces outline + wash
    vec3 strokeColor = traceStrokes(pixelPos, SAMPLES);

    // 2. Paper texture - grain over result
    vec3 result = applyPaper(strokeColor, uv, paperScale);

    finalColor = vec4(result, 1.0);
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| samples | int | 8-32 | Stroke trace iterations (quality vs performance) |
| strokeStep | float | 0.4-2.0 | Outline stroke length per iteration |
| washStrength | float | 0.0-1.0 | Color wash/bleed intensity |
| paperScale | float | 1.0-20.0 | Paper texture tiling frequency |
| paperStrength | float | 0.0-1.0 | Paper grain visibility |
| noiseAmount | float | 0.0-0.5 | Randomness in stroke paths |

## Modulation Candidates

- **strokeStep**: Larger values stretch strokes into long flowing marks; smaller values create tight hatching
- **washStrength**: Higher values push color further from dark areas, creating pronounced wet-into-wet bleed
- **samples**: More iterations produce smoother, more defined strokes; fewer create rougher marks
- **noiseAmount**: Higher values break stroke regularity, creating more organic hand-painted feel
- **paperStrength**: Reveals or hides the paper grain texture

## Notes

- The stroke tracing loop (24 iterations with 4 gradient evaluations each) dominates cost. Consider a "quality" toggle: 8 samples for real-time, 24 for high quality.
- Stroke tracing uses pixel-space coordinates. Resolution changes affect stroke appearance — normalize by resolution for consistency.
- Paper texture should both lighten and darken (centered around 1.0), unlike the old implementation which only darkened.
- The old Sobel-edge-darkening approach is superseded entirely. Stroke tracing inherently darkens edges through the outline accumulation.
- KinoAqua (Unity) implements this technique as a post-process, confirming it works in a real-time render pipeline. It notes the shader "hasn't been optimized enough for practical use" on mobile, but desktop GPU handles it.
- The Flockaroo shader uses a separate noise texture for randomness. Our implementation can use the existing hash-based procedural noise to avoid texture dependencies.
