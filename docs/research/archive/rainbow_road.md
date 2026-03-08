# Rainbow Road

Frequency-mapped light bars receding in perspective from one screen edge to the other. Each bar corresponds to an FFT frequency band — low frequencies get big bold bars up front, high frequencies shimmer as thin lines in the distance. Bars glow and swell with audio energy, swaying and curving with configurable wobble. A neon highway that breathes with the music.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Output stage with blend compositor
- **Compute Model**: Single-pass fragment shader

## Attribution

- **Based on**: "Rainbow Road" by XorDev
- **Source**: https://www.shadertoy.com/view/NlGfzz
- **License**: CC BY-NC-SA 3.0

## References

- [Segment - exact SDF](https://iquilezles.org/articles/distfunctions2d/) - Inigo Quilez's 2D distance functions, segment SDF used for bar width

## Reference Code

```glsl
// "Rainbow Road" by @XorDev [249 chars]
// https://www.shadertoy.com/view/NlGfzz
// License: CC BY-NC-SA 3.0

void mainImage(out vec4 O, vec2 I)
{
    //Resolution for scaling
    vec2 r = iResolution.xy, o;
    //Clear fragcolor
    O*=0.;
    //Render 50 lightbars
    for(float i=fract(-iTime); i<25.; i+=.5)
        //Offset coordinates (center of bar)
        o = (I+I-r)/r.y*i + cos(i*vec2(.8,.5)+iTime),
        //Light color
        O += (cos(i+vec4(0,2,4,0))+1.) / max(i*i,5.)*.1 / (i/1e3+
        //Attenuation using distance to line
        length(o-vec2(clamp(o.x,-4.,4.),i+o*sin(i)*.1-4.))/i);
}
```

## Algorithm

### Reference Breakdown (Original Version)

The reference's original (ungolfed) loop body does this per bar:

```glsl
// 1. Scale centered UV by loop index i for perspective, add sway
o = (I*2 - r) / r.y * i + cos(i * vec2(0.8, 0.5) + iTime);
// 2. Offset Y so bar i sits at world-Y = 0 in local coords
o.y += 4.0 - i;
// 3. Bar direction vector: mostly horizontal, slight per-bar tilt
d = vec2(4.0, sin(i) * 0.4);
// 4. Segment SDF: project point onto bar, clamp to segment endpoints
float proj = clamp(dot(o, d) / dot(d, d), -1.0, 1.0);
float dist = length(o - proj * d);
// 5. Accumulate glow: color * depth_fade / (distance_atten + epsilon)
O += (cos(i+vec4(0,2,4,0))+1.) / max(i*i, 5.) * 0.1 / (dist/i + i/1e3);
```

Key mechanics:
- **Perspective**: multiplying UV by `i` makes each bar's coordinate space proportionally larger. Distant bars (high `i`) map the same screen pixel to a larger world area, so the bar appears smaller.
- **Bar spacing**: `o.y += 4.0 - i` puts each bar at a different world-Y. Because depth = `i`, bar i appears at screen-Y = `(i - 4) / i = 1 - 4/i`. For large `i` this converges toward Y=1 — the vanishing point.
- **Segment SDF**: the bar is a line segment from `-d` to `+d`. Half-length = `length(d)` ~ 4 units. The `sin(i)*0.4` y-component tilts each bar slightly.
- **Depth fade**: `1/max(i*i, 5)` dims distant bars quadratically (floored at 5 for near bars). `dist/i` normalizes distance by depth so further bars maintain proportional glow radius.

### Adapted Shader (Exact Formulas)

```glsl
// Centered, aspect-corrected UV: (0,0) at center, Y spans -0.5 to +0.5
vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

// Direction: 0 = recede upward (big at bottom), 1 = recede downward (big at top)
if (direction == 1) uv.y = -uv.y;

vec3 color = vec3(0.0);

for (int i = 0; i < layers; i++) {
    float fi = float(i);

    // --- FFT energy (standard BAND_SAMPLES pattern) ---
    float t = fi / float(layers - 1);                  // 0..1 frequency position
    // ... standard band energy lookup (see generator_patterns.md) ...
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;                // drives BOTH glow and width

    // --- Perspective depth ---
    // depth = 1 at bar 0 (nearest), grows linearly per bar
    // perspective controls rate: higher = more extreme convergence
    float depth = 1.0 + fi * perspective;

    // --- Position in bar's local space ---
    vec2 p;
    p.x = uv.x * depth + sway * cos(fi * phaseSpread + time);
    p.y = (uv.y + 0.5) * depth - fi;
    // Why +0.5: shifts UV origin to bottom edge before scaling.
    // Bar i center is at screen-Y = fi / depth - 0.5
    //   = fi / (1 + fi * perspective) - 0.5
    // Bar 0: screen-Y = -0.5 (bottom edge)
    // Bar N: screen-Y converges toward 1/perspective - 0.5 (vanishing point)
    // With perspective=1.0, layers=32: bar 31 at Y = 31/32 - 0.5 = +0.47 (near top)

    // --- Bar segment: horizontal with per-bar tilt ---
    float hw = maxWidth * brightness;                   // half-width scales with energy
    float tilt = curvature * sin(fi * phaseSpread);     // vertical component at endpoints
    vec2 barEnd = vec2(hw, tilt * hw);                  // endpoint relative to bar center

    // --- Segment SDF ---
    float proj = clamp(dot(p, barEnd) / dot(barEnd, barEnd), -1.0, 1.0);
    float dist = length(p - proj * barEnd);

    // --- Glow with depth fade ---
    // depth*depth: quadratic falloff (distant bars dimmer)
    // dist/depth: normalize distance by depth (distant bars keep proportional glow radius)
    // 0.001: epsilon prevents division by zero
    float depthFade = 1.0 / max(depth * depth, 5.0);
    float glow = glowIntensity * depthFade / (dist / depth + 0.001);

    // --- Color from gradient LUT ---
    vec3 barColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
    color += glow * barColor * brightness;
}

fragColor = vec4(color, 1.0);
```

### What changed from reference

| Reference | Adapted | Why |
|-----------|---------|-----|
| `cos(i + vec4(0,2,4,0))` rainbow | `texture(gradientLUT, vec2(t, 0.5))` | Standard LUT coloring |
| Loop `i` = depth = spacing | `depth = 1 + i * perspective` | Decouples bar count from perspective strength |
| `cos(i * vec2(.8,.5) + time)` | `sway * cos(fi * phaseSpread + time)` | Separate sway amplitude and phase controls |
| `sin(i) * 0.4` bar tilt | `curvature * sin(fi * phaseSpread)` | Configurable curvature |
| Clamp `(-4, 4)` fixed width | `maxWidth * brightness` | Width driven by FFT energy |
| `o.y += 4.0 - i` | `p.y = (uv.y + 0.5) * depth - fi` | Bars span bottom-to-vanishing-point |
| No direction control | `uv.y = -uv.y` when direction=1 | Flip recession direction |

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| direction | int | 0-1 | 0 | 0 = recede upward (big at bottom), 1 = recede downward (big at top). Implemented: `if (direction==1) uv.y = -uv.y` |
| perspective | float | 0.1-3.0 | 1.0 | Depth multiplier per bar: `depth = 1 + i * perspective`. Higher = more extreme convergence toward vanishing point. At 1.0 with 32 layers, last bar has depth 32. |
| maxWidth | float | 0.5-8.0 | 4.0 | Bar half-width ceiling in world units. Actual half-width = `maxWidth * (baseBright + mag)`. At baseBright=0 and no energy, bars are invisible. |
| sway | float | 0.0-3.0 | 1.0 | Lateral offset amplitude: `sway * cos(i * phaseSpread + time)`. 0 = bars stay centered. |
| curvature | float | 0.0-1.0 | 0.1 | Per-bar tilt: bar endpoint y-component = `curvature * sin(i * phaseSpread) * hw`. 0 = perfectly horizontal bars. |
| phaseSpread | float | 0.1-3.0 | 0.8 | Multiplier on `i` for both sway cosine and curvature sine. Low = synchronized wave, high = each bar wobbles independently. |
| glowIntensity | float | 0.1-5.0 | 1.0 | Numerator of glow: `glowIntensity * depthFade / (dist/depth + 0.001)`. Scales overall brightness. |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest mapped frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Highest mapped frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT energy multiplier |
| curve | float | 0.1-3.0 | 1.5 | Energy response curve (contrast) |
| baseBright | float | 0.0-1.0 | 0.15 | Floor for `brightness = baseBright + mag`. Drives both glow intensity AND bar half-width (`maxWidth * brightness`). At 0, bars only appear when energy > 0. |
| layers | int | 4-64 | 32 | Number of bars / frequency bands. Each bar gets one band from the baseFreq-to-maxFreq log spread. |

## Modulation Candidates

- **perspective**: Deepens or flattens the perspective — high values compress bars toward the vanishing point
- **maxWidth**: Scales the upper bound of bar thickness — pulsing creates a breathing highway
- **sway**: Lateral drift intensity — modulation makes bars swing wider on hits
- **curvature**: Bar bending — adds wriggle on transients
- **phaseSpread**: Wobble coherence — low = synchronized wave, high = chaotic tangle
- **glowIntensity**: Overall glow brightness

### Interaction Patterns

- **Cascading threshold (baseBright x energy)**: At `baseBright = 0`, bars are completely invisible until their frequency band has energy. Quiet passages go dark; drops light up the entire road. At `baseBright > 0`, bars are always visible but swell with energy. This gates the entire visual density of the effect.
- **Competing forces (sway vs curvature)**: High sway with low curvature produces smooth synchronized lateral waves. Low sway with high curvature produces static but individually bent bars. Both high creates chaotic tangled filaments. The tension between them determines whether the road looks orderly or wild.
- **Resonance (phaseSpread x sway)**: When `phaseSpread` is near integer multiples, neighboring bars align their wobble — creating moments of synchronized motion that break apart as the value drifts. Modulating `phaseSpread` slowly creates a breathing order-to-chaos cycle.

## Notes

- Performance: Loop over `layers` bars per pixel. At 64 layers the segment SDF is cheap (clamp + length), but test on target hardware. 32 is a safe default.
- The segment SDF `length(p - vec2(clamp(p.x, -hw, hw), 0.0))` gives rounded bar caps for free.
- `baseBright` driving both brightness and width simultaneously means a single modulation route controls the bar's entire presence — no need to wire two params to get "bars appear/disappear."
