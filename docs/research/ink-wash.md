# Ink Wash

Emphasizes image contours by darkening edges (ink pooling) while texturing flat areas with paper-like grain noise. The combination makes edges appear to glow — flat regions get pushed down by granulation while edges stay clean, and directional color bleed pulls bright values into edge zones.

## Classification

- **Category**: TRANSFORMS > Style
- **Pipeline Position**: Output stage, user-ordered transforms

## References

- Internal: old `watercolor.fs` implementation (commit `0abe845`), stripped of watercolor-specific framing
- Original research doc (commit `1353e17:docs/research/watercolor.md`)

## Algorithm

### Edge Detection (Sobel)

3x3 Sobel operator on luminance. Produces edge magnitude 0-1.

```glsl
float sobelEdge(vec2 uv, vec2 texel)
{
    float n[9];
    n[0] = getLuminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
    n[1] = getLuminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
    n[2] = getLuminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
    n[3] = getLuminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
    n[4] = getLuminance(texture(texture0, uv).rgb);
    n[5] = getLuminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
    n[6] = getLuminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
    n[7] = getLuminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
    n[8] = getLuminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

    float sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    float sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    return sqrt(sobelH * sobelH + sobelV * sobelV);
}
```

### Edge Darkening

Multiply color by inverse edge strength. Simulates ink pooling at contours.

```glsl
color *= 1.0 - edge * strength;
```

### Paper Granulation

FBM noise applied to flat areas only (edge mask excludes contours). Edges stay bright relative to textured surroundings.

```glsl
float paper = fbmNoise(uv * 20.0);  // fixed scale
float granMask = 1.0 - edge;        // spare edges from noise
color *= mix(1.0, paper, granulation * granMask);
```

### Color Bleed

Directional blur along the luminance gradient at edges. Pulls bright colors into edge zones.

```glsl
vec2 edgeDir = normalize(vec2(sobelH, sobelV) + 0.001);
vec3 bleed = vec3(0.0);
for (int i = -2; i <= 2; i++) {
    bleed += texture(texture0, uv + edgeDir * float(i) * 5.0 * texel).rgb;
}
bleed /= 5.0;
color = mix(color, bleed, edge * bleedStrength);
```

### Combined Pipeline

```glsl
void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;
    vec3 color = texture(texture0, uv).rgb;

    // 1. Edge detection
    float edge = sobelEdge(uv, texel);

    // 2. Edge darkening (ink pooling)
    color *= 1.0 - edge * strength;

    // 3. Paper granulation (flat areas only)
    float paper = fbmNoise(uv * 20.0);
    color *= mix(1.0, paper, granulation * (1.0 - edge));

    // 4. Color bleed at edges
    vec2 grad = sobelGradient(uv, texel);
    vec2 bleedDir = normalize(grad + 0.001);
    vec3 bleed = directionalBlur(uv, bleedDir, 5.0, texel);
    color = mix(color, bleed, edge * bleedStrength);

    finalColor = vec4(color, texture(texture0, uv).a);
}
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| strength | float | 0.0-2.0 | 1.0 | Edge darkening intensity — higher values ink harder at contours |
| granulation | float | 0.0-1.0 | 0.5 | Paper noise in flat areas — makes edges pop by contrast |
| bleedStrength | float | 0.0-1.0 | 0.5 | Bright color smearing into edge zones |
| bleedRadius | float | 1.0-10.0 | 5.0 | How far colors spread at edges (texels) |

Hardcoded (not worth exposing):
- Paper scale: 20.0 (FBM frequency)
- Bleed kernel: 5 taps

## Modulation Candidates

- **strength**: Higher on beats pulses the ink borders in and out
- **granulation**: Modulate with high-frequency energy for reactive paper texture

## Notes

- No pre-blur (softness). The old watercolor used a box filter that dominated cost. Edge detection on raw input produces sharper, more defined contours.
- No color quantization. That's a separate effect (Palette Quantization) already in the inventory.
- Paper scale is fixed at 20.0 — matches the old preset value and produces good grain density. Exposing it adds configurability without visual payoff.
- The Sobel gradient direction is computed alongside edge magnitude. Store both in the edge detection pass to avoid redundant texture fetches for the bleed step.
