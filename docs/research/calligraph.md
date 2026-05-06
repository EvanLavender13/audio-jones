# Calligraph

Ink seeded along a lissajous-traced curve continuously deposits into a feedback field where procedural curl noise advects the trails into swirling, decaying calligraphic forms. The viewer sees a curved line being drawn and re-drawn while its earlier impressions drift and fade, building an evolving inky memory of the curve's history.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator (drawn before transforms)
- **Compute Model**: Ping-pong render textures (replaces Shadertoy `iChannel0` self-feedback)

## Attribution

- **Based on**: "Inkelly" by leon
- **Source**: https://www.shadertoy.com/view/4fl3Wl
- **License**: CC BY-NC-SA 3.0

## References

- [Inkelly](https://www.shadertoy.com/view/4fl3Wl) — feedback ink simulation with curl-noise advection over a procedural fbm-of-gyroid noise field

## Reference Code

```glsl
// Inkelly
// leon denise 2023-12-27

// other variations
// https://www.shadertoy.com/view/4cs3Rs
// https://www.shadertoy.com/view/4cX3WS

// feedback displace pass (Buffer A)

float delay = 4.;

// crazy noise
float gyroid (vec3 seed) { return dot(sin(seed),cos(seed.yzx)); }
float fbm (vec2 pos)
{
    float t = floor(iTime/delay);
    float t2 = t*1.354;
    vec3 p = vec3(pos, t);
    float result = 0., a = .5;
    for (int i = 0; i < 3; ++i, a /= 2.) {
        result += abs(gyroid(p/a)*a);
    }
    result = sin(result*6.283+t2-pos.x);
    return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 p = (2.*fragCoord-iResolution.xy)/iResolution.y;

    // curl noise
    vec2 e = vec2(4./iResolution.y,0);
    vec2 curl = vec2(fbm(p+e.xy)-fbm(p-e.xy), fbm(p+e.yx)-fbm(p-e.yx)) / (2.*e.x);
    curl = vec2(curl.y, -curl.x);

    // spawn shape
    p += curl*.05;
    float dist = max(abs(p.x)-2.,abs(p.y));
    //dist = abs(length(p)-.5);
    float mask = smoothstep(.01, 0., dist);

    // displace
    curl *= 0.005;
    vec4 frame = texture(iChannel0, uv + curl);

    // feedback
    mask = max(mask, frame.r - iTimeDelta);

    fragColor = vec4(mask, curl, 1);
}

// render pass (Image)

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float d = length(uv-.5);

    // blue noise
    vec4 blu = texture(iChannel1, fragCoord/iChannelResolution[1].xy);

    // background
    d += blu.x*.2;
    vec3 color = vec3(1)*smoothstep(2., 0., d);

    // edge
    float feather = .02;
    vec3 ep = vec3(1./iChannelResolution[0].xy,0);
    #define T(u) smoothstep(0., feather, texture(iChannel0, uv+u).r)
    float mr = T(.0);
    float edge = abs(T(ep.xz)-mr)+abs(T(-ep.xz)-mr)+abs(T(ep.zy)-mr)+abs(T(-ep.zy)-mr);
    color *= vec3(1.-clamp(edge/2., 0., 1.));

    fragColor = vec4(color,1.0);
}
```

## Algorithm

Two-pass generator using ping-pong render textures. Pass A evolves the ink field; the render pass converts the field into colored output.

**Pass A (feedback) — keep verbatim from reference**:
- `gyroid(vec3)` function — copy unchanged
- `fbm(vec2)` function structure (3-iteration loop with `a /= 2`, `abs(gyroid(p/a)*a)` accumulation, final `sin(result*6.283+t2-pos.x)`) — copy unchanged
- Curl computation via finite-difference of `fbm` at `+/-e` offsets followed by 90-degree rotation `curl = vec2(curl.y, -curl.x)` — copy unchanged
- Spawn distortion `p += curl * spawnDistort` (literal `0.05` becomes a config field)
- Feedback advection `texture(prevFrame, uv + curl * advectScale)` (literal `0.005` becomes a config field)
- Saturating accumulation `mask = max(spawnMask, prev.r - decayRate * deltaTime)` (the reference's `iTimeDelta` becomes `decayRate * deltaTime` so decay can be tuned)

**Pass A — substitutions**:
- Replace stepped time `float t = floor(iTime/delay); float t2 = t*1.354;` with continuous time `float t = phase; float t2 = t * 1.354;` where `phase` is CPU-accumulated as `phase += morphSpeed * deltaTime` per project speed-accumulation convention
- Replace horizontal-bar SDF `dist = max(abs(p.x)-2., abs(p.y))` with SDF to lissajous curve: loop over `lissajousSamples` parametric points along `t in [0, 2*PI]`, evaluate the lissajous at each `t`, track the minimum distance from the current pixel `p` to those samples, take sqrt for the distance. Spawn mask: `smoothstep(lineThickness, 0., minDist)`. Lissajous evaluation uses the embedded `DualLissajousConfig` per project convention for 2D parametric coordinates
- Replace `texture(iChannel0, ...)` with the previous ping-pong texture
- Replace `iResolution` with the project's `resolution` uniform; centered coordinates per project convention: `vec2 p = (fragTexCoord * resolution - resolution * 0.5) / resolution.y * 2.0` (height-normalized centered coords matching the reference's `(2*fragCoord - iResolution)/iResolution.y`)

**Render pass — keep verbatim**:
- 4-tap edge detection on the mask field via the `T(u)` macro pattern. The `feather`, `ep`, `mr`, `edge` lines copy verbatim. `feather` becomes a config field `edgeFeather`

**Render pass — substitutions**:
- Drop the paper background `color = vec3(1) * smoothstep(2., 0., d)` and the blue-noise dither (`iChannel1`). The AudioJones aesthetic is glow-on-black, not ink-on-paper
- Replace the constant white line color `vec3(1)` with `texture(gradientLUT, vec2(t, 0.5)).rgb` where `t = mask` (gradient LUT sampled by current ink density)
- Multiply by FFT brightness using `t = mask` per `memory/generator_patterns.md` continuous-per-pixel BAND_SAMPLES=4 pattern: `freq = baseFreq * pow(maxFreq/baseFreq, t)`, average 4 adjacent bins, `mag = pow(clamp(energy/4 * gain, 0, 1), curve)`, `brightness = baseBright + mag`
- Final fragment color: `texture(gradientLUT, vec2(mask, 0.5)).rgb * (baseBright + mag) * edge`

**Lissajous SDF — note**:
- The lissajous is sampled as a discrete polyline rather than computing exact parametric distance. Min-distance to N samples is the SDF approximation. Pixel quality scales with `lissajousSamples`
- GLSL 330 supports dynamic loop bounds per `memory/feedback_glsl_loops.md`; use `for (int i = 0; i < lissajousSamples; ++i)` directly, no hardcoded max + break

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| morphSpeed | float | 0.0 - 2.0 | 0.25 | Rate the curl noise field morphs |
| decayRate | float | 0.05 - 5.0 | 1.0 | How fast deposited ink fades per second |
| spawnDistort | float | 0.0 - 0.2 | 0.05 | Curl-noise displacement of the spawn shape itself |
| advectScale | float | 0.0 - 0.02 | 0.005 | Curl-noise advection strength on the feedback field |
| lineThickness | float | 0.005 - 0.1 | 0.02 | Thickness of the lissajous spawn line in centered NDC |
| edgeFeather | float | 0.005 - 0.1 | 0.02 | Edge-detection feather width on the render pass |
| lissajousSamples | int | 16 - 256 | 64 | Polyline samples along the lissajous for SDF approximation |
| lissajous | DualLissajousConfig | - | embedded | Lissajous curve geometry |
| color | ColorConfig | - | embedded | Gradient LUT for line coloring |
| baseFreq | float | 27.5 - 440 | 55 | FFT band lower bound (Hz) |
| maxFreq | float | 1000 - 16000 | 14000 | FFT band upper bound (Hz) |
| gain | float | 0.1 - 10.0 | 2.0 | FFT magnitude multiplier |
| curve | float | 0.1 - 3.0 | 1.5 | FFT magnitude power curve |
| baseBright | float | 0.0 - 1.0 | 0.15 | Minimum brightness floor before FFT add |
| blendIntensity | float | 0.0 - 5.0 | 1.0 | Standard generator blend strength |
| blendMode | EffectBlendMode | enum | SCREEN | Compositing mode (codebase additive-like blend) |

## Modulation Candidates

- **morphSpeed**: slows or accelerates the curl field's evolution
- **decayRate**: short decay leaves crisp lines, long decay leaves long inky memory
- **lineThickness**: thin precise lines vs. thick floods of ink
- **spawnDistort**: small values keep the spawn shape recognizable, larger values let the seed drift
- **advectScale**: small = stable lines, large = trails fly off in long curl-driven streamers
- **edgeFeather**: sharp ink edges vs. soft brushed edges
- **lissajous frequencies / phases / amplitudes**: reshape the seed curve continuously

### Interaction Patterns

- **decayRate vs. spawnDistort** (competing forces): Slow decay holds the seed shape's history; high spawnDistort scrambles the seed before it can imprint. Slow decay + low distort builds a clean lissajous portrait. Slow decay + high distort produces dense ghosting where the seed has been dragged through many positions. Fast decay + low distort gives a clean live curve. Fast decay + high distort gives almost nothing - the seed never settles before fading.
- **morphSpeed vs. advectScale** (resonance): Low morphSpeed + high advectScale = steady streamers in a slowly-changing flow field. High morphSpeed + low advectScale = the field reorganizes faster than ink can be carried through it. Both high = unrecognizable chaos. Both low = nearly static.
- **lineThickness vs. edgeFeather** (cascading threshold): Edge detection only produces visible lines when the mask transitions over a width comparable to `edgeFeather`. If `lineThickness` is below `edgeFeather`, the seed's interior never registers as filled and the rendered output collapses to faint smudges. Above the threshold, the line crisps up into clear edges.

## Notes

- Single-channel feedback (red channel) is sufficient — match the reference. R8 or R16F formats work for the ping-pong textures.
- The saturating accumulation `mask = max(spawnMask, prev.r - decay)` is non-additive: the seed paints over the prior field rather than blending. Keep this; additive blending would saturate to white instantly.
- Per-pixel cost: 6 fbm evaluations (one for the curl finite-difference along each axis = 4 calls, plus 2 for the symmetric sample) and `lissajousSamples` distance computations. Consider `EFFECT_FLAG_HALF_RES` if the loop hurts framerate at full resolution.
- Ping-pong feedback owns render textures, so register with `REGISTER_GENERATOR_FULL` and include `EFFECT_FLAG_NEEDS_RESIZE` per project conventions.
