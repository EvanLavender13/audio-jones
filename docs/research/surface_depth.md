# Surface Depth

Treats the input texture's luminance as a heightfield and applies parallax displacement, normal-based lighting, and optional fresnel rim glow. Bright pixels become raised surfaces, dark pixels become recessed voids. The viewer's angle can be animated to create a shifting perspective that reveals depth in otherwise flat textures. Absorbs and replaces the existing Heightfield Relief effect.

## Classification

- **Category**: TRANSFORMS > Optical
- **Pipeline Position**: Transform stage (replaces Heightfield Relief's slot)
- **Scope**: New `surface_depth.h/.cpp/.fs`, delete `heightfield_relief.h/.cpp/.fs`

## References

- [LearnOpenGL - Parallax Mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping) - Complete tutorial covering basic, steep, and POM variants with GLSL code
- [Steep Parallax Mapping (Brown University)](https://casual-effects.com/research/McGuire2005Parallax/index.html) - Original academic reference
- Shadertoy Xs2cDz - Simple heightmap ray-step with jitter and fake lighting
- Shadertoy 3ds3zf - POM on sphere with TBN, height-derived normals, Blinn-Phong lighting, fresnel

## Reference Code

### Simple ray-step (from Shadertoy Xs2cDz, adapted)

The cheapest depth displacement. Steps linearly through depth, breaks when accumulated depth exceeds the sampled height. Jitter reduces banding.

```glsl
// Core loop — steps through depth, breaks when height > depth
for (int i = 0; i < int(steps); i++) {
    float depth = (float(i) + jitter) / steps;
    uv = texcoord * depth;
    color = texture(iChannel0, uv + vec2(time, 0.)).rgb;
    color = vec3(color.r);
    if (1.0 - dot(color, vec3(0.33333) * 0.2) < depth) break;
}

// Fake directional derivative lighting
float diff = max(color.r - texture(iChannel0, uv + vec2(time, 0.) - .005).r, 0.) * 3.;
float diff2 = max(color.r - texture(iChannel0, uv + vec2(time, 0.) + .005).r, 0.) * 3.;
color *= vec3(.5) * color;
color += vec3(.4, .7, 1.) * diff * diff * 5. + vec3(1, .7, .4) * diff2 * diff2 * 5.;
```

### POM with interpolation (from Shadertoy 3ds3zf, adapted)

Smoother displacement via layer stepping with linear interpolation between the hit layer and the previous layer.

```glsl
float sampleDepth(in vec2 uv) {
    float height = pow(texture(iChannel0, uv).r, 2.2);
    return 1.0 - pow(height, 1.0 / 3.0);
}

vec2 parallax(in vec2 uv, in vec3 view) {
    float numLayers = PARALAX_QUALITY;
    float layerDepth = 1.0 / numLayers;

    vec2 p = view.xy * PARALAX_INTENSITY;
    vec2 deltaUVs = p / numLayers;

    float Texd = sampleDepth(uv);
    float d = 0.0;
    while (d < Texd) {
        uv -= deltaUVs;
        Texd = sampleDepth(uv);
        d += layerDepth;
    }

    // Linear interpolation between hit and previous layer
    vec2 lastUVs = uv + deltaUVs;
    float after = Texd - d;
    float before = sampleDepth(lastUVs) - d + layerDepth;
    float w = after / (after - before);
    return mix(uv, lastUVs, w);
}
```

### Height-derived normals (from Shadertoy 3ds3zf)

Derives surface normals from height differences between neighboring texels. Used for lighting.

```glsl
vec3 texNorm(in vec2 uv) {
    vec2 TexSize = vec2(textureSize(iChannel0, 0));
    vec2 TexelCoordsC = vec2(TexSize) * uv;
    vec2 TexelCoordsR = mod(TexelCoordsC + vec2( 1, 0), TexSize);
    vec2 TexelCoordsT = mod(TexelCoordsC + vec2( 0, 1), TexSize);
    vec2 TexelCoordsL = mod(TexelCoordsC + vec2(-1, 0), TexSize);
    vec2 TexelCoordsB = mod(TexelCoordsC + vec2( 0,-1), TexSize);

    float r = texelFetch(iChannel0, ivec2(TexelCoordsR), 0).r;
    float t = texelFetch(iChannel0, ivec2(TexelCoordsT), 0).r;
    float l = texelFetch(iChannel0, ivec2(TexelCoordsL), 0).r;
    float b = texelFetch(iChannel0, ivec2(TexelCoordsB), 0).r;

    float dx = l - r;
    float dy = b - t;
    return normalize(vec3(dx, dy, 0.03));
}
```

### Fresnel rim glow (from Shadertoy 3ds3zf variant)

```glsl
float fresnel(vec3 normal, vec3 view, float exponent) {
    return pow(1.0 - clamp(dot(normalize(normal), normalize(view)), 0., 1.), exponent);
}
// Usage: fresnel_color = fresnel(normal, vec3(0,0,1), FRESNEL_EXPONENT) * FRESNEL_INTENSITY;
```

### Existing Heightfield Relief (being absorbed)

```glsl
// Sobel gradient → normal → Blinn-Phong
// This is equivalent to depthMode=None + lighting=On in the new effect
vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);
float gx = dot(sobelH.rgb, vec3(0.299, 0.587, 0.114));
float gy = dot(sobelV.rgb, vec3(0.299, 0.587, 0.114));
vec3 normal = normalize(vec3(-gx, -gy, reliefScale));
vec3 lightDir = normalize(vec3(cos(lightAngle), sin(lightAngle), lightHeight));
float diffuse = dot(normal, lightDir) * 0.5 + 0.5;
vec3 halfDir = normalize(lightDir + viewDir);
float spec = pow(max(dot(normal, halfDir), 0.0), shininess);
float lighting = diffuse + spec;
vec3 result = texColor.rgb * mix(1.0, lighting, intensity);
```

## Algorithm

### Pipeline adaptation

The Shadertoy references operate on 3D geometry with TBN matrices. For this pipeline (flat 2D post-process):

- **No TBN matrix** — tangent space IS screen space. The view direction is constructed directly from the `viewAngle` parameter: `vec3 viewDir = normalize(vec3(viewAngleX, viewAngleY, 1.0))`
- **No 3D raymarching** — the `map()` sphere SDF from the references is not used. The parallax function operates directly on the input texture's UVs.
- **Height from luminance** — `dot(rgb, vec3(0.299, 0.587, 0.114))` converts to luminance, then apply `heightPower` curve. No sRGB linearization needed (render textures are already linear).
- **Normal derivation** — use the `texNorm()` approach (4-tap cross pattern) for POM mode, or Sobel (3x3, 9 taps) for None mode to match existing Heightfield Relief quality.

### Depth mode dispatch

```
depthMode == 0 (None):   sample at original UV, derive normals via Sobel
depthMode == 1 (Simple): ray-step loop, break on height > depth, no interpolation
depthMode == 2 (POM):    layer-step loop with linear interpolation between hit layers
```

For Simple and POM modes, after UV displacement, normals are derived from the displaced UV position (not the original). This ensures lighting responds to the parallax-shifted surface.

### Lighting

When `lighting` is enabled:
- Derive normal from height gradient (Sobel for None mode, 4-tap cross for Simple/POM)
- Compute light direction from `lightAngle` + `lightHeight`
- Lambertian diffuse: `dot(normal, lightDir) * 0.5 + 0.5` (biased to avoid pure black)
- Blinn-Phong specular: `pow(max(dot(normal, halfDir), 0.0), shininess)`
- Blend with original via `intensity`: `texColor.rgb * mix(1.0, diffuse + spec, intensity)`

### Fresnel

When `fresnel` is enabled:
- `f = pow(1.0 - clamp(dot(normal, vec3(0,0,1)), 0, 1), fresnelExponent)`
- Add `f * fresnelIntensity * fresnelColor` to the final result
- `fresnelColor` could be a fixed cold tint `vec3(0.5, 0.8, 1.0)` or derived from the texture

### Jitter (Simple mode)

Temporal + spatial hash jitter to reduce banding in the simple ray-step mode:
```glsl
float jitter = fract(sin(fract(time) + dot(texcoord, vec2(41.97, 289.13))) * 43758.5453);
jitter *= min(0.25 + length(texcoord - center), 1.0);  // reduce jitter near center
```

### Height interpretation

```glsl
float getHeight(vec2 uv) {
    float lum = dot(texture(texture0, uv).rgb, vec3(0.299, 0.587, 0.114));
    return pow(lum, heightPower);  // heightPower > 1 compresses midtones toward dark
}
float getDepth(vec2 uv) {
    return 1.0 - getHeight(uv);
}
```

## Parameters

### Depth

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `depthMode` | int (enum) | 0-2 | 1 | None / Simple / POM |
| `heightScale` | float | 0.0-0.5 | 0.15 | UV displacement magnitude. Maps to PARALAX_INTENSITY. |
| `heightPower` | float | 0.5-3.0 | 1.0 | Gamma curve for luminance→height. >1 makes midtones recede more. |
| `steps` | int | 8-64 | 15 | Ray-march quality (Simple and POM modes) |

### View

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `viewAngleX` | float | -1.0 to 1.0 | 0.0 | Horizontal viewing tilt |
| `viewAngleY` | float | -1.0 to 1.0 | 0.0 | Vertical viewing tilt |
| `viewLissajous` | DualLissajousConfig | - | {.amplitude = 0.0f} | Orbital animation for view angle |

### Lighting

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `lighting` | bool | - | true | Enable normal-based lighting |
| `lightAngle` | float | 0-2pi | 0.785 | Light direction in radians |
| `lightHeight` | float | 0.1-2.0 | 0.5 | Light elevation |
| `intensity` | float | 0.0-1.0 | 0.7 | Lighting blend strength |
| `reliefScale` | float | 0.02-1.0 | 0.2 | Normal flatness (higher = subtler normals) |
| `shininess` | float | 1.0-128.0 | 32.0 | Specular exponent |

### Fresnel

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `fresnel` | bool | - | false | Enable fresnel rim glow |
| `fresnelExponent` | float | 1.0-10.0 | 2.0 | Rim tightness (higher = tighter) |
| `fresnelIntensity` | float | 0.0-3.0 | 2.0 | Rim glow brightness |

## Modulation Candidates

- **heightScale**: Depth magnitude. Bass-driven for beat-synced depth pulses.
- **viewAngleX/Y**: Shifts the parallax perspective. Smooth modulation creates a "looking around" feel.
- **viewLissajous.amplitude**: How far the view wanders.
- **lightAngle**: Rotating light source. LFO for slow sweep.
- **intensity**: Lighting blend. Can fade relief in/out.
- **heightPower**: Changes which luminance ranges create depth. Modulating shifts what "pops."
- **fresnelIntensity**: Rim glow brightness. Treble-driven for sparkle.

### Interaction Patterns

- **Cascading threshold: heightScale + depthMode** — In None mode, heightScale does nothing (no displacement). Switching to Simple/POM "unlocks" heightScale as a visible parameter. Audio-driven heightScale only produces visual change when depthMode is active.
- **Competing forces: heightPower + lighting intensity** — High heightPower flattens midtones into the background, but strong lighting can pull them back out via specular highlights on subtle gradients. The tension between what "recedes" via depth and what "catches light" creates dynamic surface detail.
- **Resonance: viewAngle + lightAngle** — When view and light directions align, specular highlights appear where parallax displacement is strongest (the ridges facing you). When opposed, highlights appear on the far side of ridges while depth displacement shows the near side — creating a dramatic relief effect.

## Notes

- **Replaces Heightfield Relief**: `depthMode=None` + `lighting=true` reproduces the existing Heightfield Relief effect exactly. Delete `heightfield_relief.h/.cpp/.fs` and update any presets manually.
- **Effect descriptor**: Takes Heightfield Relief's slot in the Optical category (section 7). Use `EFFECT_FLAG_NONE`.
- **No sRGB correction**: Input is a linear render texture, not sRGB. The Shadertoy references apply `pow(r, 2.2)` linearization which we skip.
- **Normal derivation method**: Sobel (9-tap) for None mode preserves Heightfield Relief's existing quality. 4-tap cross for Simple/POM modes is cheaper and sufficient since the displaced UV already captures the surface structure.
- **Steps performance**: 15 steps at 1080p is ~15 texture samples per pixel per step in the loop. Profile and consider reducing default if it's too heavy. The simple mode's early-break helps — most pixels hit within a few steps.
- **View angle range**: ±1.0 maps to a ~45° tilt. The `normalize(vec3(vx, vy, 1.0))` construction means the z-component is always 1.0, so the angle is `atan(length(vxy))` — at ±1.0 that's 45°. This keeps the effect from going sideways.
