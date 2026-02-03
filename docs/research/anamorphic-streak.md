# Anamorphic Streak

Horizontal light streaks extending from bright areas, simulating the optical artifact from anamorphic cinema lenses. Oval aperture elements in these lenses stretch point light sources into characteristic horizontal lines. Ranges from soft diffused glow to sharp defined streaks via configurable sharpness.

## Classification

- **Category**: TRANSFORMS > Optical
- **Pipeline Position**: Output stage, alongside Bloom (after transforms, before Clarity/FXAA)
- **Chosen Approach**: Balanced - dedicated horizontal Kawase blur with sharpness control, matching existing bloom architecture

## References

- [Anamorphic Bloom with Unreal Engine 4](https://www.froyok.fr/blog/2017-05-anamorphique-bloom-with-unreal-engine-4/) - Core concept: modify blur kernel to horizontal-only, adjust weight scaling for streak character
- [Anamorphic Bloom - Unity URP](https://echoesofsomewhere.com/2023/09/04/custom-post-process-effect-anamorphic-bloom/) - Parameters: brightness, threshold, width via iteration count
- [Screen Space Lens Flare - John Chapman](https://john-chapman.github.io/2017/11/05/pseudo-lens-flare.html) - Threshold extraction: `max(rgb - vec3(threshold), vec3(0.0))`

## Algorithm

### 1. Threshold Extraction

Extract bright pixels using soft knee threshold (matches existing bloom pattern):

```glsl
float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
float soft = brightness - threshold + knee;
soft = clamp(soft, 0.0, 2.0 * knee);
soft = soft * soft / (4.0 * knee + 0.0001);
float contribution = max(soft, brightness - threshold) / max(brightness, 0.0001);
vec3 extracted = color.rgb * contribution;
```

### 2. Horizontal Kawase Blur

Multi-pass horizontal-only Kawase blur. Each pass samples at increasing offsets:

```glsl
// Single pass - sample 4 points horizontally offset
vec2 texelSize = 1.0 / resolution;
float offset = (float(iteration) + 0.5) * texelSize.x * stretch;

vec3 sum = texture(tex, uv).rgb * 0.5;
sum += texture(tex, uv + vec2(-offset, 0.0)).rgb * 0.25;
sum += texture(tex, uv + vec2( offset, 0.0)).rgb * 0.25;
```

### 3. Sharpness Control

Sharpness modifies the kernel weight distribution:

- **Soft (0.0)**: Standard Kawase weights `[0.25, 0.5, 0.25]` - diffused glow
- **Sharp (1.0)**: Flattened weights `[0.33, 0.34, 0.33]` - defined lines

```glsl
float centerWeight = mix(0.5, 0.34, sharpness);
float sideWeight = (1.0 - centerWeight) * 0.5;
```

### 4. Composite

Additive blend with intensity control:

```glsl
vec3 result = original + streak * intensity;
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| enabled | bool | - | false | Enable/disable effect |
| threshold | float | 0.0-2.0 | 0.8 | Brightness cutoff for streak activation |
| knee | float | 0.0-1.0 | 0.5 | Soft threshold falloff |
| intensity | float | 0.0-2.0 | 0.5 | Streak brightness in final composite |
| stretch | float | 1.0-20.0 | 8.0 | Horizontal extent of streaks |
| sharpness | float | 0.0-1.0 | 0.3 | Kernel falloff: soft glow (0) to hard lines (1) |
| iterations | int | 2-6 | 4 | Blur pass count, affects streak smoothness |

## Modulation Candidates

- **intensity**: Streak prominence pulses
- **stretch**: Horizontal extent breathes in/out
- **sharpness**: Character shifts between dreamy and crisp
- **threshold**: Sensitivity to bright areas changes dynamically

## Notes

- Shares threshold extraction logic with existing Bloom - consider shared utility
- Horizontal-only blur skips vertical pass entirely, not just reduced weight
- Blue tint omitted for simplicity; users can apply via Color Grade if desired
- Performance: ~2-4 additional passes at reduced resolution (similar cost to Bloom)
