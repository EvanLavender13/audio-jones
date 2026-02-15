# Random Walk Trail

A drifting cursor that wanders unpredictably through the scene, leaving shapes behind on the feedback texture. Ranges from smooth floating dust-mote motion to jerky staccato jumps depending on a smoothness parameter. An alternative motion mode for the existing parametric trail drawable.

## Classification

- **Category**: Drawable enhancement (ParametricTrail motion mode)
- **Pipeline Position**: Drawable stage — same as existing Lissajous trail
- **Compute Model**: CPU-side position computation, one shape drawn per frame

## References

- User-provided Shadertoy shader — deterministic random walk via hash chain with `sdSegment` rendering
- [Hash Functions for GPU Rendering (JCGT)](https://jcgt.org/published/0009/03/02/paper.pdf) — comparison of hash quality for graphics
- [Nathan Reed: Quick and Easy GPU Random Numbers](https://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/) — Wang hash, PCG recommendation

## Reference Code

User-provided Shadertoy shader (the core random walk logic):

```glsl
float h21 (vec2 a) {
    return fract(sin(dot(a.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

const float m = 50.;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    // discrete time steps
    float t = floor(20. * iTime) - 1000.;

    vec2 p = vec2(0.);
    float d = 1.;
    float k = 0.01;
    for (float i = 0.; i < m; i++) {
        t++;

        // next point: hash gives random direction, step scales with position in chain
        vec2 p2 = p + 0.2 * (i/m) * (vec2(h21(k * vec2(t, t + 1.)),
                                          h21(k * vec2(t, t - 1.))) - 0.5);

        p2 = clamp(p2, -0.48, 0.48);

        float d2 = min(0.5 * length(uv - p2), 2. * sdSegment(uv, p, p2));

        p = p2;
        d = min(d, d2);
        d += 0.0005;
    }

    float s = smoothstep(-0.03, 0.03, -d + 0.005);
    s = clamp(s, 0., 1.);
    s *= 4. * s;
    vec3 col = vec3(s);

    fragColor = vec4(col,1.0);
}
```

## Algorithm

**Key difference from reference:** The Shadertoy shader recomputes the entire walk history every frame (stateless rendering). Our trail drawable doesn't need that — the feedback texture accumulates the visual trail. We only need to compute **one new position per frame** and advance the walk state.

### Adaptation to codebase

**What to keep from reference:**
- Deterministic hash-based direction: each tick produces a reproducible (dx, dy) step from a seed/counter
- Discrete time stepping with configurable tick rate

**What to change:**
- Replace `h21` sin-dot hash with a CPU-side integer hash (splitmix-style) for better distribution and no platform-dependent float precision issues
- No distance field rendering — we draw a single shape at the current position (existing `DrawPoly` path)
- Add smooth interpolation: lerp between previous and current discrete positions using fractional time within the tick
- Add boundary mode selection (clamp/wrap/drift)
- Walk state stored in the drawable struct, not recomputed from history

### Walk computation (per frame)

1. Accumulate time: `timeAccum += deltaTime * tickRate`
2. While `timeAccum >= 1.0`: advance one discrete step
   - Hash current `tickCounter` twice (for x and y) to get direction in [-1, 1]
   - Scale by `stepSize` to get `(dx, dy)`
   - Store previous position, compute new position
   - Apply boundary constraint (clamp / wrap / drift)
   - Increment `tickCounter`
   - Subtract 1.0 from `timeAccum`
3. Compute output position:
   - `smoothness = 0.0`: snap to current discrete position (jerky)
   - `smoothness = 1.0`: lerp(prevPos, currentPos, fractionalTime) (smooth glide)
   - Intermediate values: lerp(discretePos, smoothPos, smoothness)

### Hash function (CPU-side)

Use splitmix-style integer hash — fast, good distribution, deterministic:

```
uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x45d9f3b;
    x ^= x >> 16;
    x *= 0x45d9f3b;
    x ^= x >> 16;
    return x;
}
```

Convert to float in [0, 1]: `(float)(hash(seed) & 0xFFFFFF) / (float)0xFFFFFF`

Two calls per step: `hash(tickCounter * 2)` for dx, `hash(tickCounter * 2 + 1)` for dy.

### Boundary modes

| Mode | Behavior |
|------|----------|
| Clamp | `clamp(pos, -bound, bound)` — hard stop at edges |
| Wrap | `fmod` wrap — exits one side, enters opposite |
| Drift | Blend toward origin: `pos = lerp(pos, origin, driftStrength * dt)` applied after each step |

### Motion mode selector

Add `TrailMotionType` enum (`TRAIL_MOTION_LISSAJOUS`, `TRAIL_MOTION_RANDOM_WALK`) to `ParametricTrailData`. The render function dispatches to either `DualLissajousUpdate()` or the new random walk update based on this enum. UI shows the appropriate parameter section for the active mode.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| motionType | enum | Lissajous / Random Walk | Lissajous | Selects motion algorithm |
| stepSize | float | 0.001–0.1 | 0.02 | Distance per discrete step (modulatable) |
| tickRate | float | 1.0–60.0 | 20.0 | Discrete steps per second |
| smoothness | float | 0.0–1.0 | 0.5 | 0=jerky snaps, 1=smooth glide (modulatable) |
| boundaryMode | enum | Clamp / Wrap / Drift | Drift | How walk handles screen edges |
| driftStrength | float | 0.0–2.0 | 0.3 | Pull toward center (only in Drift mode) |
| seed | int | 0–9999 | 0 | Deterministic walk seed |

## Walk State (runtime, not serialized)

| Field | Type | Purpose |
|-------|------|---------|
| walkX, walkY | float | Current discrete walk position |
| prevX, prevY | float | Previous discrete position (for interpolation) |
| timeAccum | float | Fractional time within current tick |
| tickCounter | uint32_t | Step counter for hash input |

## Modulation Candidates

- **stepSize**: Drives how far each step goes. Audio-linked step size makes the walk lurch on beats and creep during quiet passages.
- **smoothness**: Low values create staccato motion, high values create flowing drift. Modulating this shifts the visual texture of the trail.
- **tickRate**: Controls step frequency. Lower = lazy drift, higher = frantic scatter.

### Interaction Patterns

**Cascading threshold (stepSize × smoothness):** When stepSize is audio-driven and smoothness is low, the walk only produces visible jumps on strong beats — quiet passages show near-stillness. With high smoothness, the same audio mapping creates flowing acceleration/deceleration instead. The smoothness parameter transforms what "reactive" looks like.

**Competing forces (stepSize × driftStrength):** Large step size pushes the cursor outward while drift pulls it back toward center. At equilibrium the walk orbits in a confined region; when audio spikes step size past the drift threshold, the cursor breaks free temporarily before being pulled back. Creates a "breathing" territorial range.

## Notes

- The walk state (position, tickCounter) resets when switching motion modes or loading a preset
- `tickRate` is not modulatable to avoid time-stepping discontinuities (same reasoning as Lissajous frequency params)
- Seed of 0 could use a default like `hash(drawableId)` so multiple trail drawables walk differently
- The existing `gateFreq` parameter still works with random walk mode — it gates shape drawing, not position computation, so the walk continues advancing even when gated
