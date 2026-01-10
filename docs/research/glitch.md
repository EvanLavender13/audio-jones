# Glitch

Simulates analog and digital video corruption through UV distortion, chromatic aberration, and overlay noise. Single effect with toggleable sub-modes: CRT, Analog, Digital, VHS.

## Classification

- **Category**: UV Transform + Procedural Overlay
- **Core Operation**: Time-varying UV distortion (noise-based) + color channel separation + scanline overlay
- **Pipeline Position**: Reorderable transform (UV distortion modes) with overlay components

## References

- User-provided Shadertoy code - gradient noise, CRT barrel distortion, analog/digital glitch, scanlines
- [VHS tape effect](https://www.shadertoy.com/view/Ms3XWH) - vertical tracking bars, scanline noise, chromatic drift

## Algorithm

### Gradient Noise (shared utility)

```glsl
// Hash by David_Hoskins
vec3 hash33(vec3 p) {
    uvec3 q = uvec3(ivec3(p)) * uvec3(1597334673U, 3812015801U, 2798796415U);
    q = (q.x ^ q.y ^ q.z) * uvec3(1597334673U, 3812015801U, 2798796415U);
    return -1.0 + 2.0 * vec3(q) * (1.0 / float(0xffffffffU));
}

// Gradient noise by iq, returns [-1, 1]
float gnoise(vec3 x) {
    vec3 p = floor(x);
    vec3 w = fract(x);
    vec3 u = w * w * w * (w * (w * 6.0 - 15.0) + 10.0); // quintic interpolant

    // Sample gradients at 8 cube corners
    vec3 ga = hash33(p + vec3(0,0,0)); vec3 gb = hash33(p + vec3(1,0,0));
    vec3 gc = hash33(p + vec3(0,1,0)); vec3 gd = hash33(p + vec3(1,1,0));
    vec3 ge = hash33(p + vec3(0,0,1)); vec3 gf = hash33(p + vec3(1,0,1));
    vec3 gg = hash33(p + vec3(0,1,1)); vec3 gh = hash33(p + vec3(1,1,1));

    // Dot products with distance vectors
    float va = dot(ga, w - vec3(0,0,0)); float vb = dot(gb, w - vec3(1,0,0));
    float vc = dot(gc, w - vec3(0,1,0)); float vd = dot(gd, w - vec3(1,1,0));
    float ve = dot(ge, w - vec3(0,0,1)); float vf = dot(gf, w - vec3(1,0,1));
    float vg = dot(gg, w - vec3(0,1,1)); float vh = dot(gh, w - vec3(1,1,1));

    // Trilinear interpolation
    return 2.0 * (va + u.x*(vb-va) + u.y*(vc-va) + u.z*(ve-va) +
                  u.x*u.y*(va-vb-vc+vd) + u.y*u.z*(va-vc-ve+vg) +
                  u.z*u.x*(va-vb-ve+vf) + u.x*u.y*u.z*(-va+vb+vc-vd+ve-vf-vg+vh));
}

float gnoise01(vec3 x) { return 0.5 + 0.5 * gnoise(x); }
```

### Mode: CRT

Barrel distortion simulating curved CRT screen:

```glsl
vec2 crt(vec2 uv) {
    vec2 centered = uv * 2.0 - 1.0;  // [-1, 1]
    float r = length(centered);
    r /= (1.0 - curvature * r * r);  // push edges outward
    float theta = atan(centered.y, centered.x);
    return vec2(r * cos(theta), r * sin(theta)) * 0.5 + 0.5;
}
```

Optional vignette:
```glsl
float vig = 8.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
color *= pow(vig, 0.25) * 1.5;
```

### Mode: Analog

Horizontal line distortion + chromatic aberration:

```glsl
float y = uv.y * resolution.y;
float t = time;

// Noise-based horizontal shift
float distortion = gnoise(vec3(0.0, y * 0.01, t * 500.0)) * intensity;
distortion *= gnoise(vec3(0.0, y * 0.02, t * 250.0)) * intensity * 0.5;

// Occasional sync pulse artifacts
distortion += smoothstep(0.999, 1.0, sin((uv.y + t * 1.6) * 2.0)) * 0.02;
distortion -= smoothstep(0.999, 1.0, sin((uv.y + t) * 2.0)) * 0.02;

vec2 st = uv + vec2(distortion, 0.0);
vec2 eps = vec2(aberration / resolution.x, 0.0);

// Chromatic aberration
color.r = texture(input, st + eps + distortion).r;
color.g = texture(input, st).g;
color.b = texture(input, st - eps - distortion).b;
```

### Mode: Digital

Block-based random displacement:

```glsl
float bt = floor(t * 30.0) * 300.0;  // quantized time for blocky feel

// Block masks from noise threshold
float blockX = step(gnoise01(vec3(0.0, uv.x * 3.0, bt)), threshold);
float blockX2 = step(gnoise01(vec3(0.0, uv.x * 1.5, bt * 1.2)), threshold);
float blockY = step(gnoise01(vec3(0.0, uv.y * 4.0, bt)), threshold);
float blockY2 = step(gnoise01(vec3(0.0, uv.y * 6.0, bt * 1.2)), threshold);
float block = blockX2 * blockY2 + blockX * blockY;

// Random horizontal offset in glitched blocks
vec2 st = vec2(uv.x + sin(bt) * hash33(vec3(uv, 0.5)).x * block * offset, uv.y);

// Blend with chromatic aberration
color = mix(originalColor, glitchedColor, block);
```

### Mode: VHS

Simulates VHS tape playback artifacts: traveling tracking bars, per-scanline noise, and drifting color channels.

```glsl
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// Vertical tracking bar: creates bump in UV.x that travels vertically
float verticalBar(float pos, float uvY, float offset) {
    float range = 0.05;
    float x = smoothstep(pos - range, pos, uvY) * offset;
    x -= smoothstep(pos, pos + range, uvY) * offset;
    return x;
}

// Multiple traveling bars at different speeds
for (float i = 0.0; i < 0.71; i += 0.1313) {
    float d = mod(time * i, 1.7);                    // bar position cycles 0-1.7
    float o = sin(1.0 - tan(time * 0.24 * i));      // oscillating offset intensity
    o *= offsetIntensity;                            // scale by parameter
    uv.x += verticalBar(d, uv.y, o);
}

// Per-scanline horizontal noise (quantized Y for banding)
float uvY = floor(uv.y * noiseQuality) / noiseQuality;
float noise = rand(vec2(time * 0.00001, uvY));
uv.x += noise * noiseIntensity;

// Time-varying chromatic aberration (channels drift in/out of phase)
vec2 offsetR = vec2(0.006 * sin(time), 0.0) * colorOffsetIntensity;
vec2 offsetG = vec2(0.0073 * cos(time * 0.97), 0.0) * colorOffsetIntensity;

color.r = texture(input, uv + offsetR).r;
color.g = texture(input, uv + offsetG).g;
color.b = texture(input, uv).b;
```

Key differences from Analog mode:
- **Tracking bars**: Localized UV bumps that travel vertically (VHS head switching)
- **Quantized noise**: Per-scanline banding vs smooth gradient noise
- **Drifting aberration**: R/G oscillate independently over time vs static offset

### Overlay: Scanlines + Noise

```glsl
// White noise
float noise = hash33(vec3(fragCoord, mod(frame, 1000.0))).r * noiseAmount;

// Scanlines (horizontal bands)
float scanline = sin(4.0 * t + uv.y * resolution.y * 1.75) * scanlineAmount;

color += noise;
color -= scanline;
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| **Global** ||||
| glitchAmount | float | 0-1 | Master intensity, scales all sub-effects |
| **CRT Mode** ||||
| crtEnabled | bool | - | Enable CRT barrel distortion |
| curvature | float | 0-0.2 | Barrel distortion strength |
| vignetteEnabled | bool | - | Darken screen edges |
| **Analog Mode** ||||
| analogEnabled | bool | - | Enable horizontal distortion |
| analogIntensity | float | 0-0.1 | Horizontal shift amount |
| aberration | float | 0-20 | Chromatic aberration in pixels |
| **Digital Mode** ||||
| digitalEnabled | bool | - | Enable block glitching |
| blockThreshold | float | 0.1-0.9 | Probability of glitch blocks |
| blockOffset | float | 0-0.5 | Max horizontal displacement |
| **VHS Mode** ||||
| vhsEnabled | bool | - | Enable VHS tape artifacts |
| trackingBarIntensity | float | 0-0.05 | Strength of traveling tracking bars |
| scanlineNoiseIntensity | float | 0-0.02 | Per-scanline horizontal jitter |
| colorDriftIntensity | float | 0-2.0 | R/G channel drift amplitude |
| **Overlay** ||||
| scanlineAmount | float | 0-0.5 | Scanline darkness |
| noiseAmount | float | 0-0.3 | White noise intensity |

## Audio Mapping Ideas

- **glitchAmount** <- beat trigger: spike intensity on beat, decay between
- **blockThreshold** <- high freq energy: more treble = more glitch blocks
- **analogIntensity** <- bass energy: bass makes the picture shake
- **aberration** <- mid energy: color fringing tracks mids
- **trackingBarIntensity** <- beat trigger: tracking errors spike on beat
- **colorDriftIntensity** <- low-mid energy: color separation follows bass/mids
- Trigger digital mode only on beat, analog/VHS run continuously

## Notes

- Time quantization (`floor(t * 30.0)`) creates the "jumpy" digital feel
- Gradient noise at different scales creates organic-looking analog distortion
- Chromatic aberration: R shifts one direction, B shifts opposite, G stays centered
- CRT vignette formula: `8 * u * v * (1-u) * (1-v)` peaks at center
- Consider frame-skip effect: occasionally hold previous frame for stutter
- Block glitch works best with quantized time to avoid smooth interpolation
- VHS tracking bars use `smoothstep` to create smooth UV bumps that travel down the screen
- VHS color drift uses different frequencies for R/G (0.97x) so channels phase in/out of sync
- VHS scanline noise quantizes Y coordinate to create horizontal banding (unlike smooth gradient noise)
