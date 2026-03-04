# Spark Flash

Glowing four-arm crosshair sparks spawn at random positions across the screen, each flashing bright as its arms extend outward then retracting and fading in a sine-shaped lifecycle. Multiple sparks overlap with staggered timing, creating a field of twinkling neon crosses with bright star points at their centers.

## Classification

- **Category**: GENERATORS > Atmosphere
- **Pipeline Position**: Generator (blended onto accumulation buffer)
- **Section Index**: 13 (Atmosphere)

## Attribution

- **Based on**: "Extreme_spark" by Sheena
- **Source**: https://www.shadertoy.com/view/7fs3Rr
- **License**: CC BY-NC-SA 3.0

## References

- [Extreme_spark](https://www.shadertoy.com/view/7fs3Rr) - Complete reference shader (user-pasted)

## Reference Code

```glsl
#define PI 3.14159265359

float hash12(vec2 p){
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 res = iResolution.xy;
    vec2 uv  = fragCoord / res;
    float aspect = res.x / res.y;

    vec3 col = vec3(0.0);

    const int N = 7;
    float lifetime = 0.2;

    float epsLine = 0.00002;
    float epsStar = 0.00005;

    vec3 lineColor = vec3(1.0, 0.15, 0.5);
    vec3 starColor = clamp(lineColor * 1.4 + vec3(0.25), 0.0, 2.0);

    for (int i = 0; i < N; i++)
    {
        float fi     = float(i);
        float offset = fi * lifetime / float(N);
        float phase  = iTime + offset;

        float seed = floor(phase / lifetime);
        float lt   = fract(phase / lifetime);

        float px = hash12(vec2(seed, fi));
        float py = hash12(vec2(seed, fi + 50.0));

        float armLen     = sin(lt * PI);
        float brightness = sin(lt * PI);

        float vFade = smoothstep(armLen, armLen * 0.55, abs(uv.y - py));
        float hFade = smoothstep(armLen, armLen * 0.55, abs(uv.x - px));

        float vGlow = 1.0 / (pow(abs(uv.x - px), 2.0) + epsLine);
        float hGlow = 1.0 / (pow(abs(uv.y - py), 2.0) + epsLine);

        vec2 d = uv - vec2(px, py);
        d.x *= aspect;
        float star = 1.0 / (dot(d, d) + epsStar);

        col += vGlow * vFade * lineColor * brightness * 0.00035;
        col += hGlow * hFade * lineColor * brightness * 0.00035;
        col += star  * starColor * brightness * 0.00018;
    }

    col = col / (col + 1.0);
    fragColor = vec4(col, 1.0);
}
```

## Algorithm

The reference shader is simple and maps cleanly to the generator pipeline. Key adaptations:

**Keep verbatim:**
- `hash12()` function for deterministic random positions
- Inverse-square glow math for arms (`1.0 / (pow(abs(d), 2.0) + eps)`)
- Inverse-distance star point (`1.0 / (dot(d, d) + eps)`)
- Smoothstep fade envelope on arm length (`smoothstep(armLen, armLen * 0.55, ...)`)
- Sine lifecycle for arm length and brightness (`sin(lt * PI)`)
- Staggered phase offsets per spark (`fi * lifetime / float(N)`)

**Replace:**
- `lineColor` / `starColor` hardcoded colors → `gradientLUT` sampling. Each spark samples at `fract(float(i) / float(layers))` for per-spark color variety, or use FFT-derived `t` value
- `iTime` → `time` uniform
- `N` (hardcoded 7) → `layers` uniform (int)
- `lifetime` (hardcoded 0.2) → `lifetime` uniform
- `epsLine` / `epsStar` hardcoded → derived from `armThickness` and `starSize` uniforms (invert the relationship: smaller eps = thinner/tighter glow)
- `0.00035` / `0.00018` intensity scalars → `armBrightness` and `starBrightness` uniforms
- Add `armReach` uniform as a multiplier on `armLen` to control how far arms extend
- Reinhard tonemapping (`col / (col + 1.0)`) stays in shader — standard for glow-heavy generators

**FFT integration:**
- Each spark `i` maps to a frequency band via the standard frequency spread pattern
- FFT energy modulates per-spark `brightness` multiplicatively: `brightness = baseBright + energy`
- Gradient LUT color indexed by normalized frequency position: `float t = float(i) / float(layers - 1)`

**Parameter → uniform mapping:**
| Config field | Reference constant | Uniform |
|---|---|---|
| `layers` | `N = 7` | `layers` |
| `lifetime` | `0.2` | `lifetime` |
| `armThickness` | `epsLine = 0.00002` | `armThickness` |
| `starSize` | `epsStar = 0.00005` | `starSize` |
| `armBrightness` | `0.00035` | `armBrightness` |
| `starBrightness` | `0.00018` | `starBrightness` |
| `armReach` | (implicit 1.0) | `armReach` |

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| layers | int | 2-16 | 7 | Number of concurrent sparks |
| lifetime | float | 0.05-2.0 | 0.2 | Spark lifecycle duration (seconds) |
| armThickness | float | 0.00001-0.001 | 0.00002 | Cross arm glow width (smaller = thinner) |
| starSize | float | 0.00001-0.001 | 0.00005 | Center star point size (smaller = tighter) |
| armBrightness | float | 0.0001-0.002 | 0.00035 | Arm glow intensity multiplier |
| starBrightness | float | 0.00005-0.001 | 0.00018 | Star point intensity multiplier |
| armReach | float | 0.1-2.0 | 1.0 | Maximum arm extension distance |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.0 | FFT contrast curve |
| baseBright | float | 0.0-1.0 | 0.1 | Minimum brightness floor |

## Modulation Candidates

- **layers**: More sparks = denser field, fewer = sparse and dramatic
- **lifetime**: Short = staccato flashes, long = lingering glows
- **armThickness**: Thin = laser-precise crosses, thick = diffuse soft glows
- **starSize**: Small = pinpoint stars, large = blooming center dots
- **armBrightness**: Controls arm visibility relative to star
- **starBrightness**: Controls star visibility relative to arms
- **armReach**: Short arms = compact sparks, long arms = screen-spanning crosses

### Interaction Patterns

- **Layers x lifetime (cascading threshold)**: High layer count only creates visual density when lifetime is long enough for sparks to overlap. With short lifetime, even many sparks appear sparse because they fade before the next wave arrives. Audio driving lifetime creates a gate that determines whether the layer count produces fullness or isolation.
- **armBrightness x starBrightness (competing forces)**: These control which element dominates — the crosshair lines or the center star point. When arms are bright and stars dim, you get a grid-like crosshatch. When stars dominate, you get scattered point flashes. The visual character shifts fundamentally depending on which peaks.
- **armThickness x armReach (resonance)**: Both thin arms and short reach produce nearly invisible sparks. Both thick arms and long reach produce massive screen-filling crosses. The visual impact scales superlinearly when both increase together, creating rare dramatic flare moments.

## Notes

- Very lightweight shader — the loop body is simple inverse-square math with no texture lookups beyond FFT/gradient
- `hash12` is deterministic per seed, so sparks don't flicker or jump — they smoothly lifecycle
- The `0.55` factor in the smoothstep fade controls arm taper sharpness. Could be exposed as a parameter later if needed, but keeping it fixed for simplicity
- Reinhard tonemapping naturally prevents blowout when many sparks overlap
