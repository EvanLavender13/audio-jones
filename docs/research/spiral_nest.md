# Spiral Nest

Nested spirals winding outward from a bright center, each arm containing smaller spirals within it. The structure evolves over time as color waves ripple along the arms and the zoom drifts continuously, creating a hypnotic tunnel-like motion. Audio drives brightness radially - bass pulses illuminate the inner arms, treble lights up the outer edges.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Output stage (after transforms)
- **Section Index**: 10 (Geometric)

## Attribution

- **Based on**: "Spiral of Spirals 2" by KilledByAPixel (Frank Force)
- **Source**: https://www.shadertoy.com/view/lsdBzX
- **License**: CC BY-NC-SA 3.0

## References

- [Spiral of Spirals 2](https://www.shadertoy.com/view/lsdBzX) - Complete reference implementation with Archimedean spiral mapping and HSV color cycling

## Reference Code

```glsl
//////////////////////////////////////////////////////////////////////////////////
// Spiral of Spirals - Copyright 2018 Frank Force
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//////////////////////////////////////////////////////////////////////////////////

const float pi = 3.14159265359;

vec3 hsv2rgb(vec3 c)
{
    float s = c.y * c.z;
    float s_n = c.z - s * .5;
    return vec3(s_n) + vec3(s) * cos(2.0 * pi * (c.x + vec3(1.0, 0.6666, .3333)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy;
    uv -= iResolution.xy / 2.0;
    uv /= iResolution.x;
   
    vec4 mousePos = (iMouse.xyzw / iResolution.xyxy);
     uv *= 100.0;
    if (mousePos.y > 0.0)
        uv *= 4.0 * mousePos.y;
    
    float a = atan(uv.y, uv.x);
    float d = length(uv);
    
    // make spiral
    float i = d;
    float p = a/(2.0*pi) + 0.5;
    i -= p;
    a += 2.0*pi*floor(i);
    
    // change over time
    float t = .05*(iTime +  400.0*mousePos.x);
    //t = pow(t, 0.4);
    
    float h = 0.5*a;
    h *= t;
    //h *= 0.1*(floor(i)+p);
    h = 0.5*(sin(h) + 1.0);
    h = pow(h, 3.0);
    h += 4.222*t + 0.4;
    
    float s = 2.0*a;
    s *= t;
    s = 0.5*(sin(s) + 1.0);
    s = pow(s, 2.0);
    
    //float h = d*.01 + t*1.33;
    //float s = sin(d*.1 + t*43.11);
    //s = 0.5*(s + 1.0);
    
    // fixed size
    a *= (floor(i)+p);
    
    // apply color
    float v = a;
    v *= t;
    v = sin(v);
    v = 0.5*(v + 1.0);
    v = pow(v, 4.0);
    v *= pow(sin(fract(i)*pi), 0.4); // darken edges
    v *= min(d, 1.0); // dot in center
    
    //vec3 c = vec3(h, s, v);
    vec3 c = vec3(h, s, v);
    fragColor = vec4(hsv2rgb(c), 1.0);
}
```

## Algorithm

### Substitution Table

| Reference | Ours | Reason |
|-----------|------|--------|
| `fragCoord.xy - iResolution.xy / 2.0` then `/ iResolution.x` | `(fragTexCoord * resolution - resolution * 0.5) / resolution.x` | Shader coordinate convention |
| `uv *= 100.0; if (mousePos.y > 0.0) uv *= 4.0 * mousePos.y;` | `uv *= zoom;` | zoom uniform replaces mouse Y. Includes CPU-accumulated zoomSpeed via exponential: `zoom * exp(zoomAccum)` |
| `float t = .05*(iTime + 400.0*mousePos.x);` | `float t = timeAccum;` | CPU-accumulated: `timeAccum += animSpeed * deltaTime`. Replaces both iTime scaling and mouse X |
| `hsv2rgb(vec3(h, s, v))` | `texture(gradientLUT, vec2(fract(h), 0.5)).rgb * v * brightness * glowIntensity` | Gradient LUT replaces HSV coloring. `h` computation becomes gradient index. `v` stays as brightness mask |
| Saturation computation (`s = 2.0*a; s *= t; ...`) | Remove entirely | Only meaningful in HSV context. Brightness structure from `v` is sufficient |
| `hsv2rgb` function | Remove entirely | Replaced by gradientLUT |
| (none) | Add FFT brightness contribution | New: `t_fft = clamp(2.0 * d / zoom, 0.0, 1.0)` drives frequency lookup |

### Keep Verbatim

These lines form the core spiral-of-spirals structure and must not be altered:

```
float a = atan(uv.y, uv.x);
float d = length(uv);
float i = d;
float p = a / (2.0 * pi) + 0.5;
i -= p;
a += 2.0 * pi * floor(i);
```

The nested structure multiplication:

```
a *= (floor(i) + p);
```

The value (brightness) computation:

```
float v = a;
v *= t;
v = sin(v);
v = 0.5 * (v + 1.0);
v = pow(v, 4.0);
v *= pow(sin(fract(i) * pi), 0.4);
v *= min(d, 1.0);
```

The hue computation (repurposed as gradient LUT index):

```
float h = 0.5 * a;
h *= t;
h = 0.5 * (sin(h) + 1.0);
h = pow(h, 3.0);
h += 4.222 * t + 0.4;
```

### FFT Integration

The gradient LUT index (`fract(h)`) is angle-based to preserve the spiral color cycling. The FFT index is distance-based for radial frequency mapping. This is an intentional split - the spiral structure has two meaningful dimensions (angular for color pattern, radial for frequency response), and locking both to the same index would sacrifice one.

```glsl
float t_fft = clamp(2.0 * d / zoom, 0.0, 1.0);
float freq = baseFreq * pow(maxFreq / baseFreq, t_fft);
float bin = freq / (sampleRate * 0.5);
float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```

### Final Composition

```glsl
vec3 color = texture(gradientLUT, vec2(fract(h), 0.5)).rgb;
fragColor = vec4(color * v * brightness * glowIntensity, 1.0);
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| zoom | float | 10.0-400.0 | 100.0 | Number of spiral arms visible (higher = more detail) |
| zoomSpeed | float | -2.0-2.0 | 0.1 | Continuous exponential zoom rate. Accumulated on CPU as `zoomAccum += zoomSpeed * dt`, applied as `zoom * exp(zoomAccum)` |
| animSpeed | float | 0.01-1.0 | 0.05 | Time evolution rate. Controls how fast color and brightness patterns shift along the spiral arms |
| glowIntensity | float | 0.5-10.0 | 2.0 | Overall brightness multiplier |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency (Hz) |
| maxFreq | float | 1000.0-16000.0 | 14000.0 | Highest FFT frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness (visible without audio) |
| gradient | ColorConfig | - | MODE_GRADIENT | Color palette (sampled along spiral angle) |
| blendMode | EffectBlendMode | - | SCREEN | Compositing mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Blend strength |

## Modulation Candidates

- **zoom**: Pulsing arm density - bass hits compress the spiral, treble opens it up
- **zoomSpeed**: Modulating the zoom rate creates acceleration/deceleration of the tunnel effect
- **animSpeed**: Speeds up or slows down the color evolution - fast during drops, slow during ambient sections
- **glowIntensity**: Direct brightness control, pairs naturally with any audio source

### Interaction Patterns

**zoomSpeed + animSpeed (resonance)**: When both accelerate simultaneously, the visual experience compounds - the tunnel rushes forward while colors shift faster. Modulating both from the same audio source (e.g., bass energy) creates dramatic "warp speed" moments during drops that naturally relax during quieter passages. Neither parameter alone produces the full effect.

**gain + glowIntensity (competing forces)**: Gain amplifies the contrast between audio-active and silent spiral arms (bright peaks, dark gaps). GlowIntensity lifts the entire output uniformly. High gain with low glow yields stark, punchy audio reactivity. Low gain with high glow yields a gentle, always-visible wash. Routing opposing audio sources to each creates push-pull dynamics where the spiral fights between contrast and evenness.

## Notes

- The `zoomAccum` grows unbounded. At `zoomSpeed = 2.0`, `exp(zoomAccum)` reaches float precision issues after ~5 days of continuous runtime. Acceptable for real-time visualization.
- The `pow(v, 4.0)` creates sharp brightness peaks. Combined with `baseBright`, this means the effect is mostly dark with bright spiral veins unless `baseBright` is raised.
- Center masking (`min(d, 1.0)`) creates a dark dot at screen center. This is a fixed 1-pixel-radius feature of the reference; at high zoom levels it becomes invisible.
- The `h` computation includes `4.222 * t + 0.4` which creates continuous hue drift through the gradient palette over time, independent of the spiral structure. This is the "living" quality of the effect.
