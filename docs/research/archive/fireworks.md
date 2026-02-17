# Fireworks

Radiant particle bursts erupting from random positions, trailing glowing embers that arc downward under gravity while their streaky comet tails fade through the gradient palette into dying sparks. Multiple bursts overlap in time, each pulsing to a different frequency band.

## Classification

- **Category**: GENERATORS > Atmosphere
- **Pipeline Position**: Output stage (after drawables, before transforms)
- **Feedback**: Own ping-pong render texture pair (same pattern as Attractor Lines)

## References

- User-provided Shadertoy shader (see Reference Code below) — fireworks particle system with hash-based deterministic particles, gravity, drag, color lifecycle, and frame feedback
- `src/effects/attractor_lines.cpp` — ping-pong feedback pattern, exponential decay, render lifecycle

## Reference Code

```glsl
vec2 Hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float aspect = iResolution.x / iResolution.y;
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    vec4 lastFrame = texture(iChannel0, fragCoord / iResolution.xy);
    vec3 col = lastFrame.rgb * 0.92;

    for(float i=0.0; i<2.0; i++) {
        float t = iTime * 0.5 + i * 1.5;
        float id = floor(t);
        float ft = fract(t);

        if(ft < 0.9) {
            vec2 range = vec2(aspect * 0.45, 0.25);
            vec2 center = (Hash22(vec2(id, i)) - 0.5) * 2.0 * range;
            center.y += 0.2;

            vec3 pCol = sin(vec3(0.2, 0.5, 0.9) * id + id) * 0.5 + 0.5;
            pCol = max(pCol, 0.3);

           for(float j=0.0; j<80.0; j++) {
    vec2 r = Hash22(vec2(j, id));
    float angle = r.x * 6.283185;

    float individualSpeed = (0.2 + 1.2 * pow(r.y, 1.5));

    float drag = 1.0 - exp(-ft * 2.0);

    vec2 pos = center + vec2(cos(angle), sin(angle)) * individualSpeed * drag;

    pos.y -= ft * ft * 0.8;

    vec3 finalCol = mix(vec3(2.0), pCol, smoothstep(0.0, 0.15, ft));
    finalCol = mix(finalCol, vec3(0.8, 0.1, 0.0), smoothstep(0.65, 0.9, ft));

    float d = length(uv - pos);

    float size = 0.008 * (1.0 - ft * 0.5);
    float glow = size / (d + 0.002);
    glow = pow(glow, 1.7);

    float sparkle = sin(iTime * 20.0 + j) * 0.5 + 0.5;
    float sFactor = mix(1.0, sparkle, smoothstep(0.4, 0.9, ft));

    col += finalCol * glow * (1.0 - ft) * sFactor * 0.15;
}
        }
    }

    fragColor = vec4(col, 1.0);
}
```

## Algorithm

### Adaptation from Reference

**Keep verbatim:**
- `Hash22` function (deterministic particle hash)
- Particle physics: radial expansion with exponential drag `1 - exp(-ft * dragRate)` + quadratic gravity `ft * ft * gravity`
- Color lifecycle structure: white flash at ignition, main color in middle, ember fade at end
- Glow rendering: `size / (d + eps)` raised to a sharpness power
- Sparkle: sinusoidal per-particle flicker ramping in over lifetime

**Replace:**
- `iChannel0` feedback with own ping-pong render texture pair (Attractor Lines pattern)
- Fixed `0.92` decay with exponential half-life: `exp(-ln(2) * deltaTime / halfLife)`
- `pCol = sin(...)` random color with gradient LUT sampling: `texture(gradientLUT, vec2(t, 0.5)).rgb` where `t` derives from burst ID
- `iTime * 0.5` fixed rate with configurable `burstRate` uniform
- Hardcoded 2 burst slots with configurable `maxBursts` (int uniform)
- Hardcoded 80 particles with configurable `particlesPerBurst` (int uniform)
- Point glow only with adjustable streak: when `streakLength > 0`, evaluate distance to line segment between `pos` and `pos - normalize(vel) * streakLength` instead of distance to point. Velocity is analytically computed as the derivative of position.

**Add:**
- FFT integration: each burst slot `i` maps to a semitone index in the `baseFreq → maxFreq` range (log-spaced, like other generators). The sampled FFT energy scales that burst's glow intensity. Quiet bands produce dim bursts, loud bands produce bright bursts.
- Spread area parameter: controls how far from center bursts can spawn
- Upward bias parameter: controls vertical offset of burst centers (reference uses fixed +0.2)

### Ping-Pong Feedback

Follow the Attractor Lines pattern exactly:
- `RenderTexture2D pingPong[2]` + `int readIdx` in the effect struct
- Each frame: write to `pingPong[1 - readIdx]`, read from `pingPong[readIdx]`, then swap
- Shader receives `previousFrame` uniform = the read buffer
- New particles composited with `max(newColor, prev * decayFactor)`
- `decayFactor = exp(-0.693147 * deltaTime / decayHalfLife)` computed on CPU, passed as uniform
- Clear both buffers on init and resize

### Streak Rendering

When `streakLength > 0`, each particle's glow is elongated along its velocity vector:
- Velocity at time `ft`: `vel = dir * speed * dragRate * exp(-ft * dragRate) + vec2(0, -2*ft*gravity)`
- Compute tail position: `posTail = pos - normalize(vel) * streakLength * length(vel)`
- Use SDF to line segment `(pos, posTail)` instead of `length(uv - pos)`
- When `streakLength == 0`, falls back to point distance (no extra cost)

## Parameters

### Burst

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| burstRate | float | 0.3-5.0 | 1.5 | Bursts per second |
| maxBursts | int | 1-8 | 3 | Concurrent burst slots (loop iterations) |
| particles | int | 16-120 | 60 | Particles per burst (inner loop iterations) |
| spreadArea | float | 0.1-1.0 | 0.5 | How far from center bursts can spawn |
| yBias | float | -0.5-0.5 | 0.2 | Vertical offset of burst centers |

### Physics

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| burstRadius | float | 0.1-1.5 | 0.6 | Maximum expansion distance |
| gravity | float | 0.0-2.0 | 0.8 | Downward acceleration strength |
| dragRate | float | 0.5-5.0 | 2.0 | Exponential deceleration rate |

### Visual

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| glowIntensity | float | 0.1-3.0 | 1.0 | Particle peak brightness |
| particleSize | float | 0.002-0.03 | 0.008 | Base glow radius |
| glowSharpness | float | 1.0-3.0 | 1.7 | Glow falloff power (higher = tighter) |
| streakLength | float | 0.0-1.0 | 0.3 | Comet tail length (0 = round dots) |
| sparkleSpeed | float | 5.0-40.0 | 20.0 | Sparkle oscillation frequency |
| decayHalfLife | float | 0.1-5.0 | 1.0 | Trail persistence (seconds to half brightness) |

### Audio (standard generator set)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest frequency band |
| maxFreq | float | 1000-16000 | 14000 | Highest frequency band |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.0 | FFT contrast curve |
| baseBright | float | 0.0-1.0 | 0.1 | Minimum brightness floor |

## Modulation Candidates

- **burstRate**: tempo of explosions — fast = frantic barrage, slow = rare dramatic detonations
- **gravity**: 0 = symmetric energy orbs hovering in place, max = cascading waterfall of sparks
- **dragRate**: low = particles sail outward endlessly, high = tight contained puffs
- **burstRadius**: small = dense bright clusters, large = wide sparse fans
- **streakLength**: 0 = clean dots, high = dramatic comet tails
- **glowIntensity**: overall brightness, natural target for amplitude envelope
- **decayHalfLife**: short = crisp bursts, long = smeared luminous fog
- **particleSize**: larger particles overlap more, creating bloomy bright cores
- **spreadArea**: tight center clustering vs screen-filling chaos

### Interaction Patterns

**Gravity vs decayHalfLife (competing forces):** Strong gravity pulls embers downward quickly, but long decay preserves their afterglow above. The tension creates vertical curtains of light — recent particles fall while ghosts of old ones linger overhead. Modulating gravity with bass while decay stays long produces verse/chorus contrast: heavy bass sections rain sparks down, quieter sections let them float.

**DragRate vs streakLength (gating):** Streak length is computed from velocity — but high drag kills velocity fast. At high drag, even `streakLength=1.0` produces only short stubs because particles barely move. Low drag unleashes the full comet effect. Modulating drag creates moments where streaks suddenly extend or collapse.

**BurstRate vs particles (density cascade):** More bursts × more particles = exponentially more screen fill. At low values the screen is sparse and each burst is a distinct event. Both high creates a dense continuous wash. Modulating burstRate with rhythmic energy makes choruses dense while verses stay sparse.

## Notes

- Performance scales with `maxBursts × particles` — each pixel evaluates every particle. At 8 × 120 = 960 particles with streak SDF, this is heavy. Default of 3 × 60 = 180 is comfortable. Consider adding a note in UI or capping the product.
- The `Hash22` function makes particle trajectories fully deterministic from `(burstId, particleIndex)`. No random state needed between frames — only the previous frame texture persists.
- Gravity=0 with low drag creates symmetric energy orbs (the "stylized" mode). Gravity>0 with high drag creates tight realistic bursts. The blend the user wants lives in between.
- White flash at ignition uses `mix(vec3(whiteFlash), lutColor, smoothstep(0.0, flashDuration, ft))` — the whiteFlash intensity could be a parameter but keeping it baked at ~2.0 is simpler.
- Ember fade uses `mix(currentColor, vec3(0.8, 0.1, 0.0), smoothstep(emberStart, 0.9, ft))` — the ember color is a warm red-orange, baked in. This looks good with any LUT palette since it only appears as particles die.
