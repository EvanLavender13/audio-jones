# Woodblock

Flat color fields with crisp black outlines, like a Japanese ukiyo-e print — each color layer slightly misregistered from the others, with directional wood grain texture visible in the inked areas and warm paper showing through in the gaps.

## Classification

- **Category**: TRANSFORMS > Graphic
- **Pipeline Position**: User-ordered in the transform chain
- **Type**: Transform effect — reads input texture, re-renders as woodblock print

## References

- [Godot Procedural Grain Wood Shader](https://godotshaders.com/shader/procedural-grain-wood-shader/) - MIT-licensed wood grain generation using value noise + sine waves (by きのもと 結衣)
- [danielscherzer/SHADER Wood.glsl](https://github.com/danielscherzer/SHADER/blob/master/Noise/Wood.glsl) - Wood grain via rotated space + sine lines
- [3D Game Shaders: Posterization](https://lettier.github.io/3d-game-shaders-for-beginners/posterization.html) - Luminance quantization technique
- Existing codebase: `shaders/toon.fs` — Sobel edge detection + posterization (proven, tested)
- Existing codebase: `shaders/risograph.fs` — Per-layer registration offset with spatial noise warp, paper texture, simplex noise

## Reference Code

### Wood grain generation (Godot shader, MIT license)

```glsl
vec2 random( vec2 pos )
{
    return fract(
        sin(
            vec2(
                dot(pos, vec2(12.9898,78.233))
            ,   dot(pos, vec2(-148.998,-65.233))
            )
        ) * 43758.5453
    );
}

float value_noise( vec2 pos )
{
    vec2 p = floor( pos );
    vec2 f = fract( pos );

    float v00 = random( p + vec2( 0.0, 0.0 ) ).x;
    float v10 = random( p + vec2( 1.0, 0.0 ) ).x;
    float v01 = random( p + vec2( 0.0, 1.0 ) ).x;
    float v11 = random( p + vec2( 1.0, 1.0 ) ).x;

    vec2 u = f * f * ( 3.0 - 2.0 * f );

    return mix( mix( v00, v10, u.x ), mix( v01, v11, u.x ), u.y );
}

// Core grain pattern: noise-displaced sine rings
void fragment()
{
    vec2 shift_uv = UV;
    shift_uv.x += value_noise( UV * random_scale );
    float x = shift_uv.x + sin( shift_uv.y * wave_scale );
    float f = mod( x * ring_scale + random( UV ).x * noise_scale, 1.0 );
    // f is the grain intensity (0..1)
}
```

### Sobel edge detection + posterization (from codebase toon.fs)

```glsl
// Posterization: quantize luminance via max-RGB
float greyscale = max(color.r, max(color.g, color.b));
float fLevels = float(levels);
float lower = floor(greyscale * fLevels) / fLevels;
float upper = ceil(greyscale * fLevels) / fLevels;
float level = (abs(greyscale - lower) <= abs(upper - greyscale)) ? lower : upper;
float adjustment = level / max(greyscale, 0.001);
vec3 posterized = color.rgb * adjustment;

// Sobel 3x3 edge detection
vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);
float edge = length(sqrt(sobelH * sobelH + sobelV * sobelV).rgb);
float outline = smoothstep(edgeThreshold - edgeSoftness, edgeThreshold + edgeSoftness, edge);
```

### Registration offset (from codebase risograph.fs)

```glsl
// Per-layer offset: slow sinusoidal drift
vec2 offC = misregAmount * vec2(sin(misregTime * 0.7), cos(misregTime * 0.9));
vec2 offM = misregAmount * vec2(sin(misregTime * 1.1 + 2.0), cos(misregTime * 0.6 + 1.0));

// Per-layer spatial warp: noise-based paper feed distortion
vec2 warpC = misregAmount * 0.15 * vec2(
    snoise(centered * 5.0 + vec2(0.0, mFrame)),
    snoise(centered * 5.0 + vec2(3.1, mFrame)));

// Sample each layer at offset position
vec3 rgbC = texture(texture0, uv + offC + warpC).rgb;
```

## Algorithm

All building blocks exist in the codebase or references above. The woodblock effect combines them into a single-pass fragment shader.

**Pipeline (single pass):**

1. **Compute registration offsets** — One offset per color layer (3 layers). Each layer samples the input texture at `uv + offset`. Adapt Risograph's sinusoidal drift approach but without the frame-jitter component (user requested static impression, so offsets should drift slowly, not jitter).

2. **Posterize each layer** — Quantize luminance using the toon.fs method (max-RGB → floor/ceil snap → scale original color). Apply independently to each misregistered sample.

3. **Sobel edge detection** — Run Sobel on the un-offset center sample to generate the keyblock outline. Use the toon.fs pattern with noise-varied thickness for organic line quality.

4. **Generate wood grain** — Use the Godot reference pattern: rotate UV by `grainAngle`, apply value noise displacement, then `mod(x * ringScale + noise, 1.0)` to produce directional grain bands. The grain only appears in inked areas (where the posterized color is dark enough).

5. **Composite** — Paper base color → subtractive layering of posterized color layers → multiplicative wood grain in inked areas → keyblock outline on top.

**Adaptation from references:**
- Keep toon.fs Sobel and posterization verbatim (proven in codebase)
- Keep Risograph's simplex noise function verbatim (proven in codebase)
- Keep Godot wood grain core pattern (value noise + sine + mod), adapt coordinate space to use `fragTexCoord` centered on `center` uniform
- Replace Risograph's CMY decomposition with direct posterized color layers (ukiyo-e uses opaque pigment layers, not transparent CMY inks)
- Replace Risograph's frame-jitter misregistration with smooth sinusoidal drift only (static impression)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| levels | int | 2–12 | 5 | Number of flat posterization steps |
| edgeThreshold | float | 0.0–1.0 | 0.25 | Keyblock outline edge sensitivity |
| edgeSoftness | float | 0.0–0.5 | 0.05 | Outline soft/hard transition |
| edgeThickness | float | 0.5–3.0 | 1.2 | Outline width multiplier |
| grainIntensity | float | 0.0–1.0 | 0.25 | Visibility of wood grain texture in inked areas |
| grainScale | float | 1.0–20.0 | 8.0 | Fineness of grain pattern |
| grainAngle | float | -PI–PI | 0.0 | Direction of wood grain lines (radians) |
| registrationOffset | float | 0.0–0.02 | 0.004 | Color layer misalignment amount |
| registrationSpeed | float | -PI–PI | 0.3 | Speed of registration drift animation (radians/s) |
| inkDensity | float | 0.3–1.5 | 1.0 | Ink coverage strength |
| paperTone | float | 0.0–1.0 | 0.3 | Paper warmth (0=white, 1=warm cream) |

## Modulation Candidates

- **levels**: Snaps between stark (2–3 levels) and detailed (8–12 levels) — dramatic visual shift
- **edgeThreshold**: Controls outline density — low shows every edge, high shows only major boundaries
- **grainIntensity**: Fades grain in/out — pristine fresh print vs weathered wood texture
- **grainAngle**: Rotates grain direction — subtle when slow, disorienting when fast
- **registrationOffset**: Color layers drift apart and overlap — the signature "misprint" quality
- **inkDensity**: Ink saturation from ghostly transparent to heavy saturated

### Interaction Patterns

- **levels × edgeThreshold** (resonance): Fewer posterization levels create hard color boundaries that Sobel also detects as edges — outline density amplifies naturally with fewer levels. At 2–3 levels with low threshold, the print becomes aggressively bold with thick outlines around every color region. At 8+ levels with high threshold, outlines nearly vanish and the image becomes a soft color study. Modulating both from the same source creates coordinated shifts between bold graphic and subtle tonal modes.

- **inkDensity × grainIntensity** (competing forces): Heavy ink overwhelms and hides the grain; light ink reveals it. They compete for control of the inked area's appearance — high density produces clean solid fills, low density with high grain produces a worn, printed-many-times quality where the wood block's surface dominates the ink.

## Notes

- Single-pass shader — no compute, no feedback, no extra render textures
- Wood grain is purely cosmetic overlay — it doesn't affect color separation or edge detection
- Registration offset is intentionally smooth/slow (no jitter) per the "static impression" design decision — this differentiates it from Risograph's deliberately unstable printing aesthetic
- Simplex noise (`snoise`) can be shared from Risograph reference — same function verbatim
- The number of "color layers" for registration is fixed at 3 (matching RGB channels) — this isn't CMY decomposition like Risograph, it's simply sampling the texture at 3 slightly different positions and blending
