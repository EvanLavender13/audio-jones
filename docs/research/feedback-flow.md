# Feedback Flow

UV displacement driven by luminance gradients in the current frame. Creates organic flowing distortion that follows edges and contours. Unlike Reaction Warp, this has no internal state — it computes displacement purely from the current input, but the feedback loop provides temporal continuity.

## Classification

- **Category**: TRANSFORMS → Warp
- **Core Operation**: Compute luminance gradient, displace UVs perpendicular or parallel to gradient direction
- **Pipeline Position**: Transform stage (standard reorderable effect)

## How It Differs from Existing Effects

| Effect | What it does |
|--------|--------------|
| Gradient Flow | Displaces along luminance edges (Sobel-based) |
| Texture Warp | Self-referential cascading distortion |
| **Feedback Flow** | Gradient displacement that accumulates via feedback loop |

Feedback Flow is similar to Gradient Flow but designed to create **accumulating patterns** when the feedback stage is active. Each frame's displacement feeds into the next, creating evolving organic motion without explicit state management.

## Algorithm

### Core Displacement

```glsl
uniform sampler2D inputTex;
uniform float flowStrength;      // Displacement magnitude
uniform float flowAngle;         // 0 = perpendicular to edges, π/2 = along edges
uniform float gradientScale;     // Sampling distance for gradient

vec4 feedbackFlow(vec2 uv) {
    vec2 texelSize = 1.0 / resolution;
    vec2 offset = texelSize * gradientScale;

    // Sample luminance at neighboring pixels
    float lumL = getLuminance(texture(inputTex, uv - vec2(offset.x, 0)).rgb);
    float lumR = getLuminance(texture(inputTex, uv + vec2(offset.x, 0)).rgb);
    float lumD = getLuminance(texture(inputTex, uv - vec2(0, offset.y)).rgb);
    float lumU = getLuminance(texture(inputTex, uv + vec2(0, offset.y)).rgb);

    // Gradient (points toward brighter areas)
    vec2 gradient = vec2(lumR - lumL, lumU - lumD);
    float gradMag = length(gradient);

    if (gradMag < 0.001) {
        return texture(inputTex, uv);  // No edge, no displacement
    }

    // Normalize and rotate by flowAngle
    vec2 gradNorm = gradient / gradMag;
    float c = cos(flowAngle);
    float s = sin(flowAngle);
    vec2 flowDir = vec2(
        gradNorm.x * c - gradNorm.y * s,
        gradNorm.x * s + gradNorm.y * c
    );

    // Displacement scales with gradient magnitude and strength
    vec2 displacement = flowDir * gradMag * flowStrength * texelSize;
    vec2 warpedUV = uv + displacement;

    return texture(inputTex, warpedUV);
}
```

### Flow Angle Modes

- **0° (perpendicular)**: Flow moves across edges, creates spreading/smearing
- **90° (parallel)**: Flow moves along edges, creates swirling/tracing
- **45°**: Diagonal flow, creates spiral-like patterns

### With Feedback

When blur feedback is active, the displaced content gets blurred and fed back:
1. Frame N: Content has edges → gradient displacement
2. Blur applies, edges soften slightly
3. Frame N+1: Softened edges still have gradients → more displacement
4. Pattern accumulates into flowing, organic motion

The feedback decay rate controls how long patterns persist. Higher decay = tighter trails. Lower decay = longer flowing streams.

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| flowStrength | float | 0–20 | Displacement magnitude in pixels |
| flowAngle | float | 0–2π | Direction relative to gradient (0 = perpendicular, π/2 = parallel) |
| gradientScale | float | 1–5 | Sampling distance for gradient; larger = smoother, slower response |
| edgeThreshold | float | 0–0.1 | Minimum gradient magnitude to trigger flow |

## Audio Mapping Ideas

- **flowStrength** ← Bass energy: Stronger displacement on bass
- **flowAngle** ← Slow rotation from mid frequencies: Creates evolving spiral patterns
- **gradientScale** ← High frequencies: Finer detail response on treble

## Interaction with Feedback Settings

This effect's behavior depends heavily on feedback stage settings:

| Blur Decay | Flow Behavior |
|------------|---------------|
| Fast (short half-life) | Sharp, responsive trails |
| Slow (long half-life) | Long streaming patterns |
| None | Single-frame displacement only |

| Blur Diffusion | Flow Behavior |
|----------------|---------------|
| Low | Crisp edge following |
| High | Soft, dreamy flow |

## Notes

- No internal state — relies entirely on feedback loop for temporal effects
- Works best with some feedback blur active
- Complements existing warp effects (can chain with Sine Warp, etc.)
- Different from Gradient Flow: designed for feedback accumulation rather than instant edge displacement
- Flow angle is a good candidate for slow continuous rotation (Speed parameter)
