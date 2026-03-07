# Fireworks v2

Ballistic particle bursts with episode lifecycle (rocket rise, explosion, pause), two burst shape types (circular and random), per-burst FFT reactivity and gradient LUT color. Replaces the current triple-nested-loop fireworks shader with a single `fireworks x sparks` loop using analytical projectile physics instead of lookback re-simulation.

## Classification

- **Category**: GENERATORS > Scatter
- **Pipeline Position**: Output stage (after drawables, before transforms)
- **Feedback**: Own ping-pong render texture pair (existing pattern preserved)

## Attribution

- **Based on**: "Fireworks (atz)" by ilyaev
- **Source**: https://www.shadertoy.com/view/wslcWN
- **License**: CC BY-NC-SA 3.0 (Shadertoy default)

## References

- `docs/research/archive/fireworks.md` — original fireworks research (ping-pong pattern, LUT integration, sparkle)
- `src/effects/fireworks.cpp` — current implementation (C++ lifecycle to preserve)

## Reference Code

```glsl
#define SPARKS 30
#define FIREWORKS 8.
#define BASE_PAUSE FIREWORKS / 30.
#define PI 3.14
#define PI2 6.28

float n21(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

vec2 randomSpark(float noise) {
    vec2 v0 = vec2((noise - .5) * 13., (fract(noise * 123.) - .5) * 15.);
    return v0;
}

vec2 circularSpark(float i, float noiseId, float noiseSpark) {
    noiseId = fract(noiseId * 7897654.45);
    float a = (PI2 / float(SPARKS)) * i;
    float speed = 10.*clamp(noiseId, .7, 1.);
    float x = sin(a + iTime*((noiseId-.5)*3.));
    float y = cos(a + iTime*(fract(noiseId*4567.332) - .5)*2.);
    vec2 v0 = vec2(x, y) * speed;
    return v0;
}


vec2 rocket(vec2 start, float t) {
    float y = t;
    float x = sin(y*10.+cos(t*3.))*.1;
    vec2 p = start + vec2(x, y * 8.);
    return p;
}

vec3 firework(vec2 uv, float index, float pauseTime) {
    vec3 col = vec3(0.);


    float timeScale = 1.;
    vec2 gravity = vec2(0., -9.8);

    float explodeTime = .9;
    float rocketTime = 1.1;
    float episodeTime = rocketTime + explodeTime + pauseTime;

    float ratio = iResolution.x / iResolution.y;

    float timeScaled = (iTime - pauseTime) / timeScale;

    float id = floor(timeScaled / episodeTime);
    float et = mod(timeScaled, episodeTime);

    float noiseId = n21(vec2(id+1., index+1.));

    float scale = clamp(fract(noiseId*567.53)*30., 10., 30.);
    uv *= scale;

    rocketTime -= (fract(noiseId*1234.543) * .5);

    vec2 pRocket = rocket(vec2(0. + ((noiseId - .5)*scale*ratio), 0. - scale/2.), clamp(et, 0., rocketTime));

    if (et < rocketTime) {
        float rd = length(uv - pRocket);
        col += pow(.05/rd , 1.9) * vec3(0.9, .3, .0);
    }


    if (et > rocketTime && et < (rocketTime + explodeTime)) {
        float burst = sign(fract(noiseId*44432.22) - .6);
        for(int i = 0 ; i < SPARKS ; i++) {
                vec2 center = pRocket;
                float fi = float(i);
                float noiseSpark = fract(n21(vec2(id*10.+index*20., float(i+1))) * 332.44);
                float t = et - rocketTime;
                vec2 v0;

                if (fract(noiseId*3532.33) > .5) {
                    v0 = randomSpark(noiseSpark);
                    t -= noiseSpark * (fract(noiseId*543.) * .2);
                } else {
                    v0 = circularSpark(fi, noiseId, noiseSpark);

                    if ( (fract(noiseId*973.22) - .5) > 0.) {
                        float re = mod(fi, 4. + 10. * noiseId);
                        t -= floor(re/2.) * burst * .1;
                    } else {
                        t -= mod(fi, 2.) == 0. ? 0. : burst * .5*clamp(noiseId, .3, 1.);
                    }
                }

                vec2 s = v0*t + (gravity * t * t) / 2.;

                vec2 p = center + s;

                float d = length(uv - p);

                if (t > 0.) {
                    float fade = clamp((1. - t/explodeTime), 0., 1.);
                    vec3 sparkColor = vec3(noiseId*.9, .5*fract(noiseId *456.33), .5*fract(noiseId *1456.33));
                    vec3 c = (.05 / d) * sparkColor;
                    col += c * fade;
                }
            }
    }


    return col;
}

void mainImage(out vec4 fragColor, in vec2 fragCoords) {
    vec2 uv = fragCoords.xy / iResolution.xy;
    uv -= .5;
    uv.x *= iResolution.x / iResolution.y;

    vec3 col = vec3(0.);

    for (float i = 0. ; i < FIREWORKS ; i += 1.) {
        col += firework(uv, i + 1., (i * BASE_PAUSE));
    }


    fragColor = vec4(col, 1.);
}
```

## Algorithm

### Core Change: Analytical Ballistic Physics

The current shader re-simulates old bursts via a lookback loop (`MAX_TRAIL=5`) to make particles fall under gravity. This reference avoids that entirely: each particle's position is computed analytically from its initial velocity and elapsed time via `pos = center + v0*t + gravity*t^2/2`. No lookback needed — the ballistic formula gives the correct falling position at any age. The ping-pong decay adds glow trails behind the moving particles.

### Episode Lifecycle

Each firework slot cycles through three phases:
1. **Rocket** (`et < rocketTime`): A single glowing dot rises from the bottom with a wobbling sine path. Renders as a bright point glow.
2. **Explosion** (`et > rocketTime && et < rocketTime + explodeTime`): Sparks burst from the rocket's final position. Two shape types chosen by hash: circular (evenly spaced angles with time-varying rotation) or random (scattered directions with staggered ignition times).
3. **Pause** (remainder of `episodeTime`): Nothing renders. Each slot has a different pause offset so fireworks stagger naturally.

### Adaptation from Reference

**Keep verbatim:**
- `n21` hash function (deterministic from vec2 seed)
- `randomSpark` and `circularSpark` direction generators — these create the two burst shape types
- `rocket` rising path with sine wobble
- Episode timing structure: `id = floor(timeScaled / episodeTime)`, `et = mod(timeScaled, episodeTime)`
- Ballistic position: `s = v0*t + (gravity * t * t) / 2.0`
- Per-burst `noiseId` driving all randomized decisions (shape type, scale, timing, color)
- Circular spark stagger patterns (the `burst * .1` and `burst * .5` time offsets that create layered ring effects)

**Replace:**
- Hardcoded `sparkColor = vec3(noiseId*.9, ...)` with gradient LUT sampling: `texture(gradientLUT, vec2(burstT, 0.5)).rgb` where `burstT = fract(noiseId * 3.7)` gives each burst a different palette position
- Hardcoded glow `(.05 / d)` with configurable `particleSize / (d + eps)` raised to `glowSharpness` power (from current implementation — better glow control)
- `iTime` references with `time` uniform (accumulated on CPU)
- `iResolution` with `resolution` uniform
- Hardcoded constants with uniforms: `SPARKS` -> `particles`, `FIREWORKS` -> `maxBursts`, gravity magnitude, explode/rocket times
- No feedback in reference; add ping-pong decay from current implementation for glow trails behind falling particles

**Add:**
- Per-burst FFT reactivity: each burst slot `i` maps to a frequency band in the `baseFreq -> maxFreq` range (log-spaced). One FFT texture fetch per burst (not per particle). Sampled energy scales that burst's glow intensity. Quiet frequency bands = dim bursts, loud bands = bright bursts.
- Sparkle from current implementation: `sin(time * sparkleSpeed + j) * 0.5 + 0.5` per particle, ramping in over lifetime via `smoothstep`
- Color lifecycle from current implementation: white flash at ignition -> LUT color -> ember fade. Applied per-burst (the reference has flat color per burst, we add the temporal evolution)
- `spreadArea` uniform: scales the horizontal range of rocket launch positions
- `yBias` uniform: vertical offset of the launch/burst region

**Remove:**
- `MAX_TRAIL` lookback loop (the entire inner `k` loop) — ballistic formula handles falling, ping-pong handles trails
- Per-particle FFT texture fetch — replaced by one fetch per burst
- Per-particle gradient LUT fetch — replaced by one fetch per burst
- `dragRate` parameter — replaced by ballistic physics (no exponential drag)
- `burstRate` parameter — replaced by episode timing (`rocketTime + explodeTime + pauseTime`)

### Ping-Pong Feedback

Preserved from current implementation:
- `RenderTexture2D pingPong[2]` + `int readIdx` in the effect struct
- Each frame: write to `pingPong[1 - readIdx]`, read from `pingPong[readIdx]`, then swap
- Shader receives `previousFrame` uniform = the read buffer
- `decayFactor = exp(-0.693147 * deltaTime / decayHalfLife)` computed on CPU
- New particles composited additively on top of `prev * decayFactor`

### Performance

Current: `maxBursts(8) x MAX_TRAIL(5) x particles(120)` = 4,800 evals/pixel, each with 2 texture fetches.
New: `maxBursts(6) x particles(40)` = 240 evals/pixel, with 1 texture fetch per burst (not per particle).
Roughly **20x reduction** in per-pixel work.

## Parameters

### Burst

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| maxBursts | int | 1-8 | 6 | Concurrent firework slots |
| particles | int | 10-60 | 40 | Sparks per burst |
| spreadArea | float | 0.1-1.0 | 0.5 | Horizontal range of launch positions |
| yBias | float | -0.5-0.5 | 0.0 | Vertical offset of burst region |

### Timing

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| rocketTime | float | 0.3-2.0 | 1.1 | Duration of rising rocket phase (seconds) |
| explodeTime | float | 0.3-2.0 | 0.9 | Duration of explosion phase (seconds) |
| pauseTime | float | 0.0-2.0 | 0.5 | Gap between episodes per slot (seconds) |

### Physics

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| gravity | float | 0.0-20.0 | 9.8 | Downward acceleration |
| burstSpeed | float | 5.0-20.0 | 10.0 | Initial outward velocity of sparks |
| rocketSpeed | float | 2.0-12.0 | 8.0 | Upward velocity of rising rocket |

### Visual

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| glowIntensity | float | 0.1-3.0 | 1.0 | Particle peak brightness |
| particleSize | float | 0.01-0.1 | 0.05 | Base glow radius (in scaled UV space) |
| glowSharpness | float | 1.0-3.0 | 1.9 | Glow falloff power |
| sparkleSpeed | float | 5.0-40.0 | 20.0 | Sparkle oscillation frequency |
| decayHalfLife | float | 0.05-2.0 | 0.5 | Trail persistence in seconds |

### Audio

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency band |
| maxFreq | float | 1000-16000 | 14000 | Highest FFT frequency band |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness floor |

## Modulation Candidates

- **gravity**: 0 = sparks hover in symmetric orbs, max = heavy cascading rain of embers
- **burstSpeed**: small = tight dense clusters, large = wide explosive fans
- **explodeTime**: short = snappy pops, long = slow majestic blooms
- **rocketTime**: short = sparks appear from nowhere, long = dramatic anticipation
- **glowIntensity**: overall brightness, natural target for amplitude envelope
- **decayHalfLife**: short = crisp sparks, long = smeared luminous trails
- **particleSize**: larger = bloomy overlapping cores, smaller = pinpoint stars
- **spreadArea**: tight center clustering vs screen-filling chaos

### Interaction Patterns

**Gravity vs decayHalfLife (competing forces):** Strong gravity pulls sparks downward quickly, but long decay preserves their afterglow above. The tension creates vertical curtains of light — recent particles fall while ghosts linger overhead. Modulating gravity with bass while decay stays long produces verse/chorus contrast.

**BurstSpeed vs gravity (competing forces):** High burst speed flings sparks far before gravity takes hold, creating wide arcs. Low burst speed with high gravity produces tight downward streams. The ratio between them controls the shape of the ballistic parabola — wide fan vs narrow fountain.

**ExplodeTime vs pauseTime (density cascade):** Long explode + short pause = continuous overlapping bursts filling the screen. Short explode + long pause = isolated dramatic events with dark gaps. Modulating explodeTime with rhythmic energy makes choruses dense while verses stay sparse.

## Notes

- Performance at defaults: 6 slots x 40 sparks = 240 evals/pixel with zero texture fetches in the inner loop (FFT and LUT sampled once per burst outside the spark loop). Comfortable even at high resolution.
- The reference's `scale` variable (10-30, derived from hash) zooms UV space per firework so each burst has a different apparent size. This is a multiplier on uv before distance calculation — keeps the glow math resolution-independent.
- The two burst types (circular vs random) are chosen per-burst by `fract(noiseId*3532.33) > 0.5`. Circular bursts have evenly spaced angles with time-varying rotation, creating spinning ring patterns. Random bursts scatter in all directions with staggered ignition times, creating organic scatter. Both use the same ballistic physics.
- The circular spark's `iTime` dependency in the reference creates spinning rings. In our adaptation this becomes the `time` accumulator. The rotation speed varies per burst via hash.
- Rocket wobble uses `sin(y*10 + cos(t*3)) * 0.1` — a nested trig for organic sway. Keep this verbatim.
- The `burst` variable in the reference (`sign(fract(noiseId*44432.22) - .6)`) creates +1/-1 stagger direction for circular sparks, making some rings expand in alternating layers. This is a key detail for visual variety.
