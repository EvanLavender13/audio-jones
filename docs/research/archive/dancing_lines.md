# Dancing Lines

A single line segment with Lissajous-driven endpoints snapping to a new position at a configurable rate, leaving a trailing stack of past positions. Each echo holds the line's pose from one tick ago, dimming with age, with per-echo color drawn from a gradient LUT and per-echo brightness driven by an FFT band. The newest echo glows brightest at the lowest band; older echoes fade backward through the gradient and up through the spectrum, like a strobed afterimage of the Qix arcade game.

## Classification

- **Category**: GENERATORS > Filament
- **Pipeline Position**: Output stage > Generators (before Transforms)

## Attribution

- **Based on**: "Dancing lines (Qix)" by FabriceNeyret2
- **Source**: https://www.shadertoy.com/view/ffSSRt
- **License**: CC BY-NC-SA 3.0 Unported

## References

- Shadertoy "Dancing lines (Qix)" by FabriceNeyret2 - source shader for the discrete-time-step trail-of-line-echoes technique
- `shaders/arc_strobe.fs` - sibling Lissajous-line generator providing the FFT band-averaging, gradient LUT, and `sdSegment` patterns to mirror

## Reference Code

```glsl
float line(vec2 p, vec2 a, vec2 b )
{
    p -= a;
    b -= a;
    return length(p - b * clamp(dot(p, b) / dot(b, b), 0., 1.) );
}

#define SC(a,b) vec2(sin(t*a), cos(t*b))

void mainImage( out vec4 O, vec2 u )
{
    vec2  R = iResolution.xy,
          U = ( u+u - R ) / R.y;
    float t, i;
    for ( O*=i ; i++ < 10.; O *= .95 )
         t = .3 * ( i + floor(iTime * 15.) ),
        O +=    smoothstep( 4./R.y, 2./R.y, line( U, SC(.39,.63), SC(.29,.69).yx ))
             * .3* ( 1. + sin( t * vec4( 3.13, 1.69, 2.67,0)));
}
```

## Algorithm

The reference is built from three nested ideas: (1) a single line whose two endpoints are parametric Lissajous samples of a scalar `t`, (2) a discrete time tick `floor(iTime * snapRate)` that snaps `t` rather than letting it sweep continuously, (3) a loop over `i = 0..trailLength` where each echo offsets `t` by `i`, fades by a per-step factor, and shows that older snapshot in its own color.

**Keep verbatim from the reference:**
- `line()` point-to-segment distance helper - identical to arc_strobe's `sdSegment`, can be reused directly.
- The pattern `t = TIME_SCALE * (i + floor(iTime * snapRate))` for per-echo `t`. `TIME_SCALE` is the `0.3` constant in the original; expose as `colorPhaseStep` so modulating it shifts color spacing between echoes.
- The two endpoints are sampled at the same `t` but with different frequency pairs and a `.yx` swizzle on the second endpoint - this is what makes the segment span a non-degenerate region.
- Per-echo fade via `O *= fadeFactor` after each loop iteration (`0.95` in the original; expose as `trailFade`).
- Smoothstep edge AA: `smoothstep(outerWidth/R.y, innerWidth/R.y, line(...))` for hairline-thin segments. Expose `lineThickness` as the inner edge.

**Replace with codebase patterns:**
- The frequency constants `(.39, .63)` and `(.29, .69)` - replace with the shared `DualLissajousConfig` (`freqX1`, `freqY1`, `freqX2`, `freqY2`, `offsetX2`, `offsetY2`, `amplitude`). The two endpoints now share the same Lissajous config but use index-shifted phase, exactly like arc_strobe's two-endpoint sampling.
- The hardcoded `i++ < 10.` upper bound - replace with `trailLength` uniform driving a `for (int i = 0; i < trailLength; i++)` loop. GLSL 330 supports dynamic bounds.
- The fade `O *= .95` - replace with explicit per-echo multiplier `fadeAccum *= trailFade` accumulated outside the color sum.
- The color modulation `sin(t * vec4(3.13, 1.69, 2.67, 0))` - replace with gradient LUT sampling. Color the i-th echo from the same FFT band index it modulates: `texture(gradientLUT, vec2(t0, 0.5)).rgb` where `t0 = i / trailLength`. Mirrors arc_strobe's pattern of "color and frequency share the same normalized band index."
- Per-echo brightness from the `(1. + sin(t * vec4(...)))` baseline - replace with `baseBright + mag` where `mag` is the band-averaged FFT magnitude at the i-th log-spaced band. Use the standard pattern from arc_strobe: log-spaced `baseFreq..maxFreq` divided into `trailLength` bands, `BAND_SAMPLES = 4` averaging within each band, `gain` and `curve` shaping.
- Coordinate normalization `(u+u - R) / R.y` - replace with the codebase's standard centered-aspect-corrected UV: `(fragTexCoord * resolution * 2.0 - resolution) / min(resolution.x, resolution.y)` (matches arc_strobe).
- `lissajous(t)` helper function lifted directly from arc_strobe: takes a scalar `t`, evaluates dual-frequency sin/cos with the Lissajous config, returns aspect-corrected 2D point. Reuse the same shape; the second endpoint reuses the same function but at `t + endpointOffset` (replacing the `.yx` swizzle of the original; `endpointOffset` becomes a config field analogous to arc_strobe's `orbitOffset`).

**Parameter mapping:**

| Reference token | Replacement |
|-----------------|-------------|
| `iTime * 15.` | `accumTime * snapRate` (CPU-accumulated time, `snapRate` Hz) |
| `i++ < 10.` | `i < trailLength` (uniform int) |
| `O *= .95` | `fadeAccum *= trailFade` |
| `.3 * (i + floor(...))` | `colorPhaseStep * (float(i) + floor(accumTime * snapRate))` |
| `(.39, .63)`, `(.29, .69).yx` | `lissajous(t)` and `lissajous(t + endpointOffset)` from shared `DualLissajousConfig` |
| `4./R.y, 2./R.y` | `lineThickness + GLOW_WIDTH`, `lineThickness` (smoothstep AA edges) |
| `sin(t * vec4(3.13, 1.69, 2.67, 0))` | `texture(gradientLUT, vec2(t0, 0.5)).rgb` |
| `(1. + sin(...))` | `baseBright + mag` where `mag` is per-echo FFT band magnitude |

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `snapRate` | float | 1-60 | 15 | Hz at which the line snaps to its next position. Below ~5Hz feels like staircase motion, above ~30Hz approaches smooth orbit. |
| `trailLength` | int | 1-32 | 10 | Number of echoes drawn. 1 = single dancing line, 32 = thick fading ribbon of ghosts. |
| `trailFade` | float | 0.5-0.99 | 0.95 | Per-echo brightness multiplier. Lower = sharper recency emphasis; higher = longer afterimage. |
| `colorPhaseStep` | float | 0.05-1.0 | 0.3 | Index-and-time multiplier passed into the gradient sample. Smaller = closer color spacing between echoes; larger = wider color rotation per snap. |
| `lineThickness` | float | 0.0005-0.02 | 0.002 | Inner edge of the smoothstep, in normalized screen-height units. |
| `endpointOffset` | float | 0.1-3.14159 | 1.5708 | Offset along the parametric `t` between the two endpoints. Smaller = stubby segments; larger = endpoints spread across the figure. |
| `baseBright` | float | 0-1 | 0.15 | Always-on brightness floor per echo before FFT modulation. Standard codebase default. |
| `glowIntensity` | float | 0-3 | 1.0 | Output brightness multiplier applied after summing all echoes. |
| `lissajous` | DualLissajousConfig | - | `{amplitude=0.5, freqX1=2, freqY1=3, freqX2=0, freqY2=0, offsetX2=0, offsetY2=0}` | Shared Lissajous figure config. With freqX2=freqY2=0 it degenerates to a single sin/cos, matching the original's two-frequency endpoints; with both filled it gives a richer two-pair figure. |
| `color` | ColorConfig | - | (gradient mode default) | Standard generator color/blend config (gradient LUT, blend mode, blend intensity). |
| `baseFreq` | float | 27.5-440 | 55 | Lowest FFT band frequency mapped to the newest (i=0) echo. Standard audio convention. |
| `maxFreq` | float | 1000-16000 | 14000 | Highest FFT band frequency mapped to the oldest echo. Standard audio convention. |
| `gain` | float | 0.1-10 | 2.0 | FFT magnitude scalar before curve. Standard audio convention. |
| `curve` | float | 0.1-3 | 1.5 | FFT magnitude exponent for response shaping. Standard audio convention. |

## Modulation Candidates

- `snapRate`: snap cadence speeds up or slows down. Modulating from a beat envelope produces tempo-locked staccato; modulating from an LFO produces drifting strobe.
- `trailLength`: thicker or thinner ghost trail. Audio-driven ramp swells the trail on energy.
- `trailFade`: per-echo decay rate. Lower fade with bass envelope makes the trail snap to short during drops.
- `lineThickness`: line weight pulses on hits.
- `colorPhaseStep`: color spread between echoes shifts, turning a unified hue trail into a rainbow staircase.
- `endpointOffset`: line length pulses, stretching from short stub to wide span.
- `lissajous.freqX1` / `freqY1` / `freqX2` / `freqY2`: figure morphs between rosette/loop/figure-eight shapes.
- `lissajous.amplitude`: figure breathes in and out from the center.
- `glowIntensity`: overall radiance pulses.

### Interaction Patterns

- **`snapRate` x `trailLength` (resonance)**: At low snap rate with high trail length, the same Lissajous tick is visible across many echoes simultaneously, producing a slow-march feel. At high snap rate with low trail length, the ghost evaporates almost immediately, producing crisp single-line dance. When both modulate in opposite directions to the same audio source, the visual collapses from "thick slow snake" to "single fast firefly" - mood shift from heavy to light synchronized with the music.
- **`trailFade` x `trailLength` (cascading threshold)**: trailFade alone determines decay; trailLength alone determines max possible echoes. Together: at low fade the higher echoes are invisible regardless of trailLength. Modulating trailFade up unlocks the deeper trail that trailLength was already promising. Visually: chorus sections "open" to reveal the full trail length that verses had been clipping invisibly.
- **`endpointOffset` x `lissajous.freqX1` (competing forces)**: The first frequency stretches the figure across angular space; endpointOffset stretches each echo's segment along the parametric path. When freq is high and endpointOffset is small, you see many short fast-moving stubs. When freq is low and endpointOffset is large, you see a few long slow-moving spans. Modulating these in opposite directions keeps the visible "line density" constant while morphing between geometric extremes.

## Notes

- The original uses `floor(iTime * 15.)` purely on a smoothly-incrementing wall clock. CPU-side, accumulate `accumTime += deltaTime` (frame-rate independent) and pass it to the shader. The `floor(accumTime * snapRate)` snap then happens on the GPU. Do NOT compute `floor` on CPU - it must apply per-echo using `i + floor(...)`, so all echoes share the same tick boundary while indexing different positions on the trail.
- `lineThickness` and the smoothstep AA window are both in `R.y`-normalized units (screen-height fraction). Hardcode the AA window as a small offset above `lineThickness` (e.g., `+ 0.001` or similar) - the original's `4./R.y` outer to `2./R.y` inner is a fixed 2-pixel AA band that should be preserved.
- The original's `t = .3 * (i + floor(iTime * 15.))` ties the color phase, the position phase, and the trail index together with the same `t`. With FFT and gradient LUT, position phase still uses `t` (same tick, advancing index gives the line's history), but color is now sampled by FFT band index `i / trailLength` rather than by `t` itself. This decouples color cycling from the snap clock - color depends only on which echo you're looking at.
- The accumulation `O *= .95` happens BEFORE the smoothstep contribution is added each iteration, meaning the newest echo (i=0) gets multiplied by 0.95 the most times (it's been multiplied N-1 times by the end of the loop). In our reformulation, accumulate `fadeAccum` separately and apply per-iteration: `result += color * brightness * fadeAccum; fadeAccum *= trailFade;`. Newest echo (i=0) gets fadeAccum=1.0, oldest gets `trailFade^(N-1)`. This inverts the original's loop order but produces the correct visual: newest = brightest.
- Newest echo maps to `baseFreq` (lowest band) to match the codebase convention. The "fast brightening" character of bass-driven content syncs with the most-recent line's already-bright fade state. If users want the inverse, they wire baseFreq/maxFreq externally.
- Half-resolution rendering is acceptable here - thin lines benefit from a higher render-target resolution but the overall composition is sparse, so half-res with the standard generator compositor upsampling should be fine. Default to full-res; user can flag-tune later.
- Pipeline position: standard generator. Uses `STANDARD_GENERATOR_OUTPUT(dancingLines)` macro for the color/blend section in UI.
