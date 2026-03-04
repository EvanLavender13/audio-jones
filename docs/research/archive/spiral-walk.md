# Spiral Walk

A chain of glowing line segments spiraling outward from center, each turned by a fixed angle from the last and growing progressively longer. The angle step determines the spiral's form — tight coils, right-angle zigzags, golden-angle flowers, or geometric stars — and drifts over time so the pattern continuously morphs between shapes. Segments glow with gradient-LUT color and inverse-distance halos, with FFT frequency bands mapped along the chain so different parts of the spiral pulse to different parts of the spectrum.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Output stage (generators blend before transforms)

## Attribution

- **Based on**: "Kungs vs. Cookin" by jorge2017a3
- **Source**: https://www.shadertoy.com/view/sfl3zr
- **License**: CC BY-NC-SA 3.0

## References

- [2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) - IQ's sdSegment function used for point-to-segment distance (already used in filaments.fs)
- Existing codebase: `shaders/filaments.fs` — glow rendering pattern (`GLOW_WIDTH / (GLOW_WIDTH + dist)`, additive accumulation, `tanh` tonemapping)

## Reference Code

```glsl
// "Kungs vs. Cookin" by jorge2017a3
// https://www.shadertoy.com/view/sfl3zr
// License: CC BY-NC-SA 3.0

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    vec3 col = vec3(0.0); // fondo negro profundo

    //-----------------------------------------
    // Ángulo suave cada 5 seg
    //-----------------------------------------

    float block = iTime / 5.0;
    float stepIndex = floor(block);
    float progress = fract(block);
    float smoothT = smoothstep(0.0, 1.0, progress);

    float A1 = 90.0 + stepIndex * 45.0;
    float A2 = 90.0 + (stepIndex + 1.0) * 45.0;

    float A = radians(mix(A1, A2, smoothT));

    //-----------------------------------------
    // Espiral
    //-----------------------------------------

    vec2 prev = vec2(0.0);
    float L = 0.0;
    float DL = 0.02;

    for(int n = 0; n < 100; n++)
    {
        float angle = float(n) * A;
        vec2 next = prev + vec2(cos(angle), sin(angle)) * L;

        vec2 pa = uv - prev;
        vec2 ba = next - prev;
        float h = clamp(dot(pa,ba)/dot(ba,ba),0.0,1.0);
        float d = length(pa - ba*h);

        // Línea con núcleo fuerte
        float core = smoothstep(0.003, 0.0, d);

        //  Glow grande alrededor
        float glow = 0.02 / (d*d + 0.005);
         float line = smoothstep(0.004, 0.0, d);

        //  Color neón animado

        float hue = float(n)*0.05 + iTime*0.4;
        vec3 neon = 0.5 + 0.5*cos(6.2831*(hue + vec3(0.0,0.33,0.67)));
        //col += neon * (core*2.0 + glow*0.5);
        //col += neon * ( glow*0.5);
        col += neon * (core*2.0  + glow*0.015);



        prev = next;
        L += DL;
    }

    // arcoiris- Pequeño pulso global
    col *= 1.0 + 0.2*sin(iTime*2.0);

    fragColor = vec4(col,1.0);
}
```

## Algorithm

**Keep from reference:**
- Segment chain construction: each segment starts where the previous ended, angle increments by `n * A`, length grows by `DL` per step
- IQ sdSegment distance function (already in filaments.fs)

**Replace:**
- Time-based angle morphing (`block = iTime / 5.0` block) → `angleOffset + rotationAccum` where `rotationAccum += rotationSpeed * deltaTime` on CPU
- Hardcoded `100` iterations → `segments` uniform (int)
- Hardcoded `DL = 0.02` → `growthRate` uniform
- Rainbow hue cycling (`0.5 + 0.5*cos(...)`) → gradient LUT texture sampling at `float(n) / float(segments)`
- `core + glow` dual rendering → filaments-style single inverse-distance glow: `GLOW_WIDTH / (GLOW_WIDTH + dist)`
- Static brightness → `glowIntensity * (baseBright + mag)` where `mag` comes from FFT texture

**FFT mapping:**
- Map segments to frequency bands in log space from `baseFreq` to `maxFreq` (same approach as filaments)
- Each segment samples the FFT texture at its corresponding frequency position
- Inner segments (low index) pulse with bass, outer segments (high index) with treble

**Glow rendering (match filaments pattern):**
```
float dist = segm(uv, prev, next);
float glow = GLOW_WIDTH / (GLOW_WIDTH + dist);
vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
result += color * glow * glowIntensity * (baseBright + mag);
```
Final output: `tanh(result)` for soft clamp.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| segments | int | 10-200 | 100 | Number of line segments in the chain |
| growthRate | float | 0.005-0.1 | 0.02 | Length increment per segment — controls spiral spread |
| angleOffset | float | -PI..PI | PI/2 | Base angle step between segments (radians) |
| rotationSpeed | float | -PI..PI | 0.0 | Rotation rate for continuous angle morphing (rad/s) |
| glowIntensity | float | 0.5-10.0 | 2.0 | Brightness multiplier on glow |
| baseFreq | float | 27.5-440 | 55.0 | Lowest FFT frequency mapped to first segment |
| maxFreq | float | 1000-16000 | 14000 | Highest FFT frequency mapped to last segment |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 0.7 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum ember brightness for quiet segments |

## Modulation Candidates

- **angleOffset**: Morphs the spiral between completely different geometric forms — coils, stars, flowers
- **rotationSpeed**: Controls how fast the shape morphs; modulating creates acceleration/deceleration
- **growthRate**: Breathing effect — spiral expands and contracts
- **segments**: Density shifts — sparse skeletal outlines vs. dense intricate patterns
- **glowIntensity**: Overall brightness punch
- **gain**: Audio sensitivity — how much the spectrum drives segment brightness

### Interaction Patterns

- **growthRate vs segments (competing forces)**: Growth rate pushes segments outward while more segments pack them denser. Low growth + high count = compact dense knot. High growth + low count = sparse skeletal frame. Modulating both creates a breathing density that shifts between intimate and expansive.
- **angleOffset vs segments (cascading threshold)**: Certain angle values produce geometric star patterns only when segment count is high enough to complete the figure. At low counts the pattern is ambiguous; crossing a segment count threshold suddenly crystallizes a recognizable star or flower. Audio gating segment count could make shapes snap into focus on loud hits.
- **rotationSpeed vs gain (resonance)**: When rotation speed drifts the angle slowly past geometric "sweet spots" (90, 120, 137.5 degrees), audio gain spikes at those moments create resonant flares where the pattern locks into a recognizable form AND lights up bright simultaneously.

## Notes

- Loop iteration count (segments) is the main GPU cost. 200 segments means 200 distance computations per fragment. Should perform fine at typical resolutions but monitor on lower-end hardware.
- The angle step `float(n) * A` means segments don't fan uniformly — early segments cluster near center and later ones spread wide. This is inherent to the turtle-walk construction and gives the spiral its characteristic density gradient.
- `angleOffset` default of PI/2 (90 degrees) matches the reference shader's starting angle and produces a visually interesting right-angle spiral.
