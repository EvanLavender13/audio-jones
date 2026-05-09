# Diamond Weave

A tiled grid of diamond contours pulsing with concentric ringing patterns, slowly twisting under a radial swirl. Each ring takes its color from a gradient LUT and reacts to a matched FFT frequency band, so ring brightness tracks the spectrum from bass at the inner contour to treble at the outer corners.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Output stage (Generators), composited with blend mode

## Attribution

- **Based on**: "Pixel Weave" by TekF (https://www.shadertoy.com/view/XdS3RK), itself a derivative of "no title" by gaz (https://www.shadertoy.com/view/Md2Gzy)
- **License**: CC BY-NC-SA 3.0

## References

- "no title" by gaz (https://www.shadertoy.com/view/Md2Gzy) - seed shader: diamond shape function + time-multiplied sin glow rings
- "Pixel Weave" by TekF (https://www.shadertoy.com/view/XdS3RK) - refinement: pixel-space tiling, radial twist, per-cell hue offset, brightness floor

## Reference Code

### gaz "no title"

```glsl
vec3 hsv(float h, float s, float v)
{
  return mix(vec3(1.0),clamp((abs(fract(
    h+vec3(3.0, 2.0, 1.0)/3.0)*6.0-3.0)-1.0), 0.0, 1.0),s)*v;
}

float shape(vec2 p)
{
    return abs(p.x)+abs(p.y)-1.0;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy/iResolution.xy;
    vec2 pos = uv*2.0-1.0;
    pos.x *= iResolution.x/iResolution.y;
    pos = pos*cos(0.00005)+vec2(pos.y,-pos.x)*sin(0.00005);
    pos = mod(pos*4.0, 2.0)-1.0;
    float c= 0.05/abs(sin(0.3*iTime*shape(3.0*pos)));
    vec3 col = hsv(fract(0.1*iTime),1.0,1.0);
    fragColor = vec4(col*c,1.0);
}
```

### TekF "Pixel Weave"

```glsl
vec3 hsv(float h, float s, float v)
{
  return mix(vec3(1.0),clamp((abs(fract(h+vec3(3.0, 2.0, 1.0)/3.0)*6.0-3.0)-1.0), 0.0, 1.0),s)*v;
}

float shape(vec2 p)
{
    return abs(p.x)+abs(p.y)-1.0;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord.xy-iResolution.xy*.5;
    float a = .777+iTime*.0001*(1.0+.3*pow(length(pos.xy/iResolution.y),2.0));
    pos = pos*cos(a)+vec2(pos.y,-pos.x)*sin(a);
    pos = mod(pos/80.0, 2.0)-1.0;
    float h= abs(sin(0.3*iTime*shape(3.0*pos)));
    float c= 0.05/h;
    vec3 col = hsv(fract(0.1*iTime+h),1.0,1.0);
    fragColor = vec4(col*(c*.5+.5),1.0);
}
```

## Algorithm

Adapt TekF's "Pixel Weave" (the more developed of the two) into the standard generator pipeline.

**Keep verbatim:**
- `shape(p) = abs(p.x) + abs(p.y) - 1.0` diamond distance function
- Pixel-space centered coordinates: `pos = fragCoord - resolution*0.5`
- Radial twist rotation: `a = twistBase + timeAccum * twistRate * (1.0 + twistRadial * pow(length(pos / resolution.y), 2.0))`
- Cell tiling: `pos = mod(pos / cellSize, 2.0) - 1.0` (cellSize replaces hardcoded 80)
- Diamond ringing index: `h = abs(sin(phaseSpeed * t * shape(3.0 * pos)))` where `t` is the log-scaled time accumulator
- Diamond glow: `c = 0.05 / h`
- Brightness floor: `c * 0.5 + 0.5`

**Replace:**
- `iTime` driving the sin frequency growth → `t = log(1.0 + timeAccum)`. Prevents temporal aliasing decay to noise (same fix as Spiral Nest).
- `iTime` in twist angle and global drift → raw `timeAccum` (these don't cause aliasing).
- `hsv(fract(0.1*iTime + h), 1.0, 1.0)` → `texture(gradientLUT, vec2(fract(driftSpeed*timeAccum + h), 0.5)).rgb`. Preserves global drift + per-ring hue offset; LUT replaces HSV.
- Add FFT brightness via standard BAND_SAMPLES pattern, indexed by `h`.

**Final composition:** `color * (c * 0.5 + 0.5) * brightness * glowIntensity`

**Substitution table:**

| Reference | Effect |
|-----------|--------|
| `iTime` (in `sin(0.3*iTime*shape(...))`) | `log(1.0 + timeAccum)` |
| `iTime` (in twist `a` and hue drift) | `timeAccum` (raw) |
| Hardcoded `80` (cell scale) | `cellSize` config field |
| Hardcoded `0.3` (phase speed) | `phaseSpeed` config field |
| Hardcoded `0.0001` (twist rate) | `twistRate` config field |
| Hardcoded `0.3` (twist radial coefficient) | `twistRadial` config field |
| Hardcoded `0.777` (twist base angle) | `twistAngle` config field |
| Hardcoded `0.1` (hue drift speed) | `driftSpeed` config field |
| `hsv(fract(0.1*iTime + h), 1, 1)` | `gradientLUT` sampled at `fract(driftSpeed*timeAccum + h)` |
| Constant brightness | BAND_SAMPLES FFT lookup at index `h` |

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| cellSize | float | 20-200 | 80 | Pixel size of each diamond tile |
| phaseSpeed | float | 0.05-1.0 | 0.3 | Rate of ringing pattern evolution (multiplied into log-time) |
| twistAngle | float | -PI to PI | 0.777 | Base rotation of the tile grid |
| twistRate | float | 0.0-0.001 | 0.0001 | Speed of slow continuous twist over time |
| twistRadial | float | 0.0-1.0 | 0.3 | How much faster outer pixels twist vs center |
| driftSpeed | float | 0.0-0.5 | 0.1 | Global gradient drift speed |
| glowIntensity | float | 0.0-2.0 | 1.0 | Output brightness multiplier |
| baseFreq | float | 27.5-440 | 55 | FFT base frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000 | FFT max frequency (Hz) |
| gain | float | 0.1-10 | 2.0 | FFT energy gain |
| curve | float | 0.1-3 | 1.5 | FFT response curve |
| baseBright | float | 0-1 | 0.15 | FFT baseline brightness |

## Modulation Candidates

- **phaseSpeed**: pulses the ringing animation rate, rings tighten/loosen with the beat
- **cellSize**: zoom/breathe — tile grid expands and contracts
- **twistAngle**: rotates the entire grid
- **twistRate** or **twistRadial**: shifts the swirl character on cue
- **driftSpeed**: accelerates/freezes color drift
- **glowIntensity**: overall brightness gating

### Interaction Patterns

- **phaseSpeed + glowIntensity (resonance)**: when both spike together, rings simultaneously sharpen (faster ringing → more rings visible) AND brighten — rare bright moments where the pattern goes electric
- **twistRate + cellSize (competing forces)**: large cells with fast twist look like a slowly tumbling wallpaper; small cells with slow twist feel rigid. Modulating one against the other shifts character between drift and stillness

## Notes

- The log-time fix means the pattern animates fast at startup and asymptotically slows. Visual "energy" decays naturally; modulating `phaseSpeed` from audio can re-inject motion bursts.
- `c = 0.05 / h` divides by zero when `h` approaches zero (on diamond contours). This is the source of the bright glow rings; the brightness floor (`c*0.5 + 0.5`) clamps total output. No explicit guard needed — GLSL handles 1/0 deterministically as +Inf and the multiplication path tolerates it, but a small epsilon (`max(h, 1e-4)`) is safer if banding appears.
- Standard generator output: gradient LUT + blend mode + blend intensity slider via `STANDARD_GENERATOR_OUTPUT(diamondWeave)`.
