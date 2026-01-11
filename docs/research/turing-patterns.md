# Turing Patterns (DoG Approach)

Organic spot/stripe/maze displacement using Difference of Gaussians (DoG). Avoids Gray-Scott fragility. Accumulation texture perturbs patterns, keeping them reactive.

## Classification

- **Category**: TRANSFORMS → Warp
- **Core Operation**: Single compute shader updates Turing state AND outputs warped result
- **Pipeline Position**: Transform stage (reorderable). First compute shader in transforms.

## Why DoG Instead of Gray-Scott

| Gray-Scott | DoG Approach |
|------------|--------------|
| Two chemicals (A, B) | Single value |
| 4 sensitive parameters (diffA, diffB, feed, kill) | 2 intuitive parameters (blur radii) |
| Narrow "interesting" parameter ranges | Robust — ratio of radii controls pattern |
| Reaches equilibrium, stops evolving | Continuous with perturbation |

The DoG approximates the activator-inhibitor dynamic:
- **Small blur** = local activation (nearby similar values reinforce)
- **Large blur** = long-range inhibition (distant values suppress)
- The instability between these creates Turing patterns

## References

- [Softology: Multi-Scale Turing Patterns](https://softologyblog.wordpress.com/2011/07/05/multi-scale-turing-patterns/) — Algorithm explanation
- [Shadertoy: Reaction-Diffusion with DoG](https://www.shadertoy.com/view/Mdf3zR) — Single-buffer implementation
- [Observable: Multiscale Turing Patterns in WebGL](https://observablehq.com/@rreusser/multiscale-turing-patterns-in-webgl) — Advanced multi-scale version

## Algorithm

### Core Update (from Softology)

```
activator = blur(state, small_radius)
inhibitor = blur(state, large_radius)

if activator > inhibitor:
    state += small_amount
else:
    state -= small_amount
```

### Shadertoy Implementation

```glsl
// Two blur scales computed inline
vec3 blur1 = monoBlur(buffer, uv, blurSize1);  // Small (4px)
vec3 blur2 = monoBlur(buffer, uv, blurSize2);  // Large (20px)

// DoG reaction
vec3 col = prev - (blur2 - blur1 * 0.999);

// Noise injection (prevents settling)
col += (noise - 0.5) / 8.0;

// Clamp to prevent runaway
col = clamp(col, 0.0, 1.0);
```

The `0.999` factor on blur1 prevents exact cancellation and biases toward growth.

### Inline Blur Function

```glsl
float monoBlur(sampler2D tex, vec2 uv, vec2 scale, float step) {
    float result = 0.0;
    int count = 0;
    vec2 texelSize = 1.0 / resolution;

    for (float y = -scale.y; y < scale.y; y += step) {
        for (float x = -scale.x; x < scale.x; x += step) {
            vec2 offset = vec2(x, y);
            float dist = length(offset);
            float weight = 1.0 - smoothstep(0.0, scale.y * 2.0, dist);
            result += texture(tex, uv + offset * texelSize).r * weight;
            count++;
        }
    }
    return result / float(count);
}
```

Step size reduces samples: `blurSize / 4.0` means ~16 samples per blur instead of hundreds.

## Proposed Implementation

### Single Compute Shader (Does Everything)

One shader: updates Turing state, computes displacement, outputs warped result.

```glsl
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba16f, binding = 0) uniform image2D turingState;  // Persistent state
layout(binding = 1) uniform sampler2D inputTex;            // Content to warp
layout(rgba16f, binding = 2) writeonly uniform image2D outputTex;

uniform vec2 resolution;
uniform float activatorRadius;   // Small blur (e.g., 4.0)
uniform float inhibitorRadius;   // Large blur (e.g., 20.0)
uniform float reactionAmount;    // Step size (e.g., 0.01)
uniform float accumInfluence;    // How much input content perturbs
uniform float warpStrength;      // Displacement magnitude

const vec3 LUMA = vec3(0.299, 0.587, 0.114);

// Inline weighted blur of Turing state
float blur(vec2 uv, float radius) {
    float result = 0.0;
    float weight = 0.0;
    float step = max(1.0, radius / 4.0);
    vec2 texelSize = 1.0 / resolution;

    for (float y = -radius; y <= radius; y += step) {
        for (float x = -radius; x <= radius; x += step) {
            vec2 offset = vec2(x, y);
            float dist = length(offset);
            if (dist <= radius) {
                float w = 1.0 - dist / radius;
                ivec2 sampleCoord = ivec2(mod(gl_GlobalInvocationID.xy + offset, resolution));
                result += imageLoad(turingState, sampleCoord).r * w;
                weight += w;
            }
        }
    }
    return result / max(weight, 0.001);
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    if (coord.x >= int(resolution.x) || coord.y >= int(resolution.y)) return;

    vec2 uv = (vec2(coord) + 0.5) / resolution;
    vec2 texelSize = 1.0 / resolution;

    // === STEP 1: Update Turing state ===

    float state = imageLoad(turingState, coord).r;

    // DoG reaction
    float activator = blur(uv, activatorRadius);
    float inhibitor = blur(uv, inhibitorRadius);
    float delta = (activator > inhibitor) ? reactionAmount : -reactionAmount;
    state += delta;

    // Perturbation from input content
    float inputLuma = dot(texture(inputTex, uv).rgb, LUMA);
    state += (inputLuma - 0.5) * accumInfluence;

    // Clamp
    state = clamp(state, 0.0, 1.0);

    // Write updated state
    imageStore(turingState, coord, vec4(state, 0.0, 0.0, 1.0));

    // === STEP 2: Compute gradient for displacement ===

    ivec2 coordL = ivec2(mod(vec2(coord) + vec2(-1, 0), resolution));
    ivec2 coordR = ivec2(mod(vec2(coord) + vec2( 1, 0), resolution));
    ivec2 coordD = ivec2(mod(vec2(coord) + vec2(0, -1), resolution));
    ivec2 coordU = ivec2(mod(vec2(coord) + vec2(0,  1), resolution));

    float gradX = (imageLoad(turingState, coordR).r - imageLoad(turingState, coordL).r) * 0.5;
    float gradY = (imageLoad(turingState, coordU).r - imageLoad(turingState, coordD).r) * 0.5;

    // === STEP 3: Sample input at displaced UV, write output ===

    vec2 warpedUV = uv + vec2(gradX, gradY) * warpStrength * texelSize;
    vec4 warped = texture(inputTex, warpedUV);

    imageStore(outputTex, coord, warped);
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| activatorRadius | float | 2–10 | Small blur radius; affects pattern detail |
| inhibitorRadius | float | 10–40 | Large blur radius; affects pattern spacing |
| reactionAmount | float | 0.005–0.05 | Step size; higher = faster evolution |
| accumInfluence | float | 0–0.5 | How much accumulation perturbs pattern |
| decayFactor | float | 0.99–0.999 | Prevents runaway; lower = faster fade |
| warpStrength | float | 0–50 | Displacement magnitude (for warp output) |

**Key ratio**: `inhibitorRadius / activatorRadius` controls pattern type:
- ~3:1 → spots
- ~5:1 → stripes/mazes
- Higher → larger features

## Audio Mapping Ideas

- **reactionAmount** ← Bass energy: Faster pattern evolution on bass
- **inhibitorRadius** ← Mid frequencies: Pattern scale shifts
- **accumInfluence** ← Beat detection: Pulse perturbation on beats
- **warpStrength** ← Overall volume: Stronger displacement at higher volume

## Notes

- Blur radius of 20px with step=5 means ~50 samples per pixel — acceptable but not cheap
- Consider mipmap approximation for large blur if performance is an issue
- Pattern takes 50-200 frames to develop from noise; input perturbation speeds this up
- Needs persistent `turingState` texture that survives between frames
- First compute shader in transform stage — pipeline may need minor adjustment
