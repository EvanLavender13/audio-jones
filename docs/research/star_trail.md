# Star Trail

Hundreds of glowing stars orbit the screen center at different radii and speeds, leaving persistent luminous trails that fade over time. Differential rotation creates spiraling arcs -- inner stars orbit faster, outer stars lag behind. Each star reacts to its own FFT frequency bin, pulsing brighter when its mapped frequency is active. A configurable mapping mode determines how stars relate to the spectrum: by radial distance (bass=inner, treble=outer), by hash (scattered), or by angular position (frequency wedges).

## Classification

- **Category**: GENERATORS > Scatter
- **Pipeline Position**: Generator layer (blend composited onto scene)
- **Compute Model**: Ping-pong render textures for trail persistence (same as Attractor Lines / Muons)

## Attribution

- **Based on**: "StarTrail" by hdrp0720
- **Source**: https://www.shadertoy.com/view/wXcyz7
- **License**: CC BY-NC-SA 3.0

## References

- "StarTrail" by hdrp0720 (Shadertoy) - complete particle orbit + trail persistence system, pasted below

## Reference Code

### Common

```glsl
#define NUM_PARTICLES 1000
#define DATA_WIDTH 32

vec3 getParticleColor(int id) {
    int colorIdx = id % 6;
    if (colorIdx == 0) return vec3(1.0, 0.65, 0.65);
    if (colorIdx == 1) return vec3(1.0, 0.78, 0.55);
    if (colorIdx == 2) return vec3(0.98, 0.92, 0.60);
    if (colorIdx == 3) return vec3(0.65, 0.90, 0.70);
    if (colorIdx == 4) return vec3(0.60, 0.85, 1.0);
    return vec3(0.75, 0.70, 0.95);
}

float hash11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}
```

### Buffer A (Particle Simulation)

```glsl
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 iFrag = ivec2(fragCoord);
    int id = iFrag.y * DATA_WIDTH + iFrag.x;

    if (id >= NUM_PARTICLES)
    {
        fragColor = vec4(0.);
        return;
    }

    if (iFrame == 0)
    {
        float r1 = hash11(float(id) * 12.345) * 2.0 - 1.0;
        float r2 = hash11(float(id) * 67.890) * 2.0 - 1.0;

        float aspect = iResolution.x / iResolution.y;
        float scale = max(aspect, 1.0);
        fragColor = vec4(r1*scale, r2*scale, 0.0, 1.0);
        return;
    }

    vec4 p = texelFetch(iChannel0, iFrag, 0);
    vec3 pos = p.xyz;

    float dt = 1.0 / 60.0;

    float dist = length(pos.xy);
    float speed = 0.5 + (sin(dist * 10.0) * 0.1);

    vec3 velocity = vec3(-pos.y, pos.x, 0.0) * 0.4 * speed;

    pos += velocity * dt;

    fragColor = vec4(pos, 1.0);
}
```

### Buffer B (Trail Rendering)

```glsl
float smootherstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return x * x * x * (3.0 * x * (2.0 * x - 5.0) + 10.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;

    vec4 oldColor = texture(iChannel1, uv);
    float dissipation = 0.007;
    vec3 color = max(vec3(0.0), oldColor.rgb - dissipation);

    vec2 posScreen = (2.0 * fragCoord - iResolution.xy) / iResolution.y;
    float radius = 0.015 * .5;
    float radiusSq = radius * radius;
    vec3 accumColor = vec3(0.0);

    for(int i = 0; i < NUM_PARTICLES; i++) {
        int tx = i % DATA_WIDTH;
        int ty = i / DATA_WIDTH;

        vec4 p = texelFetch(iChannel0, ivec2(tx, ty), 0);
        vec2 particlePos = p.xy;

        vec2 d = particlePos - posScreen;
        float distSq = dot(d, d);

        if(distSq < radiusSq)
        {
            float dist = length(particlePos - posScreen);
            float intensity = smootherstep(radius, 0.0, dist);
            vec3 pColor = getParticleColor(i);
            accumColor += pColor * intensity;
        }
    }

    color += accumColor * 0.9;

    fragColor = vec4(color, 1.0);
}
```

### Image (Tonemap)

```glsl
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec4 col = texture(iChannel0, uv);
    col.rgb = col.rgb / (col.rgb + vec3(1.0));
    fragColor = col;
}
```

## Algorithm

The reference uses a 3-buffer architecture (simulation, rendering, tonemap). We collapse this into a single-pass ping-pong generator by computing star positions analytically instead of simulating them.

### Substitution Table

| Reference | Ours | Notes |
|-----------|------|-------|
| Buffer A particle simulation | Analytical orbit: `pos = radius * vec2(cos(angle), sin(angle))` | Circular orbits are analytically solvable. Hash seeds radius + initial angle per star. |
| `hash11()` | Keep verbatim | Same hash function for star seeding |
| `smootherstep()` | Keep verbatim | Same glow falloff function |
| `speed = 0.5 + sin(dist * 10.0) * 0.1` | `speed = orbitSpeed + sin(starRadius * speedWaviness) * speedVariation` | Expose all three terms as uniforms |
| `velocity = (-y, x) * 0.4 * speed` | Absorbed into analytical angle: `angle = initialAngle + speed * time` | No velocity integration needed |
| `getParticleColor(i)` | `texture(gradientLUT, vec2(t, 0.5)).rgb` where `t` = star's frequency position | Gradient LUT replaces hardcoded palette. `t` is the shared index for color AND FFT. |
| `dissipation = 0.007` (subtractive) | `decayFactor = exp(-0.693147 * deltaTime / halfLife)` (multiplicative, CPU-computed) | Matches Attractor Lines pattern. Frame-rate independent. |
| `texture(iChannel1, uv)` feedback | `texture(previousFrame, fragTexCoord)` | Ping-pong render texture pair |
| `NUM_PARTICLES 1000` | `uniform int starCount` (50-500) | Per-pixel loop is more expensive than per-texel simulation, so fewer stars |
| `radius = 0.015 * 0.5` (fixed) | `uniform float glowSize` | Configurable |
| No audio | Per-star FFT lookup: `freq = baseFreq * pow(maxFreq / baseFreq, t)` | Standard generator FFT pattern. `t` derived from freqMapMode. |
| Reinhard tonemap in Image pass | Reinhard in same shader after accumulation | Single-pass output |

### Frequency Mapping Mode

The `freqMapMode` uniform (int 0-2) determines how each star computes its frequency index `t`:

- **Mode 0 (Radius)**: `t = clamp(starRadius / spreadRadius, 0.0, 1.0)` - inner stars = bass, outer = treble
- **Mode 1 (Hash)**: `t = hash11(float(i) * 45.678)` - each star gets a pseudo-random frequency
- **Mode 2 (Angle)**: `t = fract(starAngle / TWO_PI)` - frequency sweeps around the circle

This same `t` drives both `gradientLUT` color sampling and FFT frequency lookup, per the generator pattern rule.

### Per-Star FFT Reactivity

Each star computes brightness from its FFT bin:
```
float freq = baseFreq * pow(maxFreq / baseFreq, t);
float bin = freq / (sampleRate * 0.5);
float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```

Brightness modulates both the star's glow intensity and optionally its glow size (brighter = slightly larger).

### Shader Structure (single pass)

```
1. Read previousFrame, multiply by decayFactor (trail fade)
2. Compute screen-centered coords: pos = fragTexCoord * resolution - resolution * 0.5
3. Loop over starCount stars:
   a. Hash star's radius, initial angle from star index
   b. Compute orbital speed from radius
   c. Compute current angle = initialAngle + speed * time
   d. Compute star position = radius * vec2(cos(angle), sin(angle))
   e. Distance check (squared) against glowSize
   f. If within range: compute t from freqMapMode, sample FFT for brightness, sample gradientLUT for color
   g. Accumulate: color += starColor * brightness * smootherstep falloff
4. Reinhard tonemap: output = accumulated / (accumulated + 1.0)
```

### C++ Infrastructure

Follows `REGISTER_GENERATOR_FULL` pattern (same as Attractor Lines):
- `StarTrailEffect` struct with `pingPong[2]`, `readIdx`, time accumulator
- Init allocates HDR ping-pong textures via `RenderUtilsInitTextureHDR()`
- Render swaps ping-pong indices each frame
- Setup computes `decayFactor` on CPU, binds all uniforms
- Speed accumulation: `time += orbitSpeed * deltaTime` on CPU, passed as uniform

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| starCount | int | 50-500 | 200 | Number of orbiting stars |
| freqMapMode | int | 0-2 | 0 | FFT mapping: 0=radius, 1=hash, 2=angle |
| orbitSpeed | float | 0.05-2.0 | 0.4 | Base angular velocity multiplier |
| speedWaviness | float | 0.0-20.0 | 10.0 | Radial speed ripple frequency |
| speedVariation | float | 0.0-0.5 | 0.1 | Radial speed ripple amplitude |
| spreadRadius | float | 0.2-2.0 | 1.0 | How far stars spread from center |
| glowSize | float | 0.005-0.05 | 0.008 | Star sprite radius in screen units |
| glowIntensity | float | 0.1-3.0 | 0.9 | Peak brightness of star sprites |
| decayHalfLife | float | 0.1-10.0 | 2.0 | Trail persistence in seconds |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency Hz |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency Hz |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Min brightness floor when silent |

## Modulation Candidates

- **orbitSpeed**: speeds up/slows down all orbits. Bass-mapped creates breathing orbital rhythm.
- **glowSize**: stars swell and shrink. Creates pulsing starbursts on beats.
- **glowIntensity**: overall brightness envelope. Quiet passages dim, loud passages flare.
- **decayHalfLife**: trail length shifts. Short decay = staccato dots, long decay = sweeping arcs.
- **spreadRadius**: stars expand outward or contract inward from center.
- **speedWaviness**: ripple frequency shifts, changing which radial bands orbit faster/slower.
- **speedVariation**: amplitude of speed ripples. Zero = uniform rotation, max = strong differential bands.

### Interaction Patterns

**Gain vs decayHalfLife (cascading threshold)**: Gain controls how bright new star contributions are relative to the decaying trail. With low gain, the decay catches up quickly -- arcs stay short and dim. Push gain past a threshold and arcs "break free" -- they persist long enough to overlap previous traces, building dense orbital rings. The threshold depends on decayHalfLife, creating a nonlinear unlock.

**orbitSpeed vs decayHalfLife (competing forces)**: Visible arc length = speed * persistence. Fast speed + short decay = sharp arcs of fixed angular extent. Slow speed + long decay = the same arc length but smoother, languid motion. Modulating orbitSpeed with percussive sources creates staccato bursts where arcs extend on hits then retract during gaps.

**spreadRadius vs freqMapMode=radius (resonance)**: When frequency maps to radius, contracting spreadRadius compresses the entire spectrum into a tighter ring. Bass and treble stars overlap spatially, and the gradient bands compress. Expanding it fans them out. The visual density and color separation shift together.

## Notes

- Per-pixel loop over starCount is the performance bottleneck. 500 stars at 1080p is the default. 1000 may need half-res flag on older GPUs.
- The `hash11` function from the reference is deterministic -- same star index always gets the same radius/angle seed. This means the star field is stable across frames (no flickering or re-randomization).
- orbitSpeed accumulation must happen on CPU (`time += orbitSpeed * deltaTime`), not in shader, per project conventions.
- FreqMapMode as a selectable enum is new to the generator system. Could become a reusable pattern for future generators.

## Implementation Notes

Deviations from the original reference and the initial research plan, documented during implementation.

### Removed: glowSize parameter

The research plan proposed a configurable `glowSize` (star sprite radius). This was removed. The original uses a fixed `radius = 0.015 * 0.5 = 0.0075` and so do we -- hardcoded as `const float SPRITE_RADIUS = 0.0075` in the shader. Exposing sprite size as a parameter coupled trail continuity to a UI knob: small values created gaps between consecutive frame dots, breaking the visual.

### Removed: Reinhard tonemap

The research plan ported the original's Image pass Reinhard (`col / (col + 1.0)`) into the generator shader. This washed out all colors. The pipeline handles HDR compositing downstream -- individual effects must output raw values. Never add tonemapping inside generator shaders.

### Removed: line segment sub-stepping

The initial implementation used `distToSegment()` between previous and current frame positions, with adaptive sub-stepping to fill gaps. This was unnecessary complexity. The original just draws dots and lets the feedback buffer accumulate them into trails -- same approach works here.

### Changed: radius distribution

The original initializes star positions as 2D random points (`r1, r2` uniform in [-1, 1]). This gives radial density proportional to radius -- more stars at larger radii, fewer near center. Our analytical approach seeds radius directly with `hash * spreadRadius`, which gives uniform radial density -- too many stars near center where orbits are tiny, causing center brightness blowout. Fixed with `sqrt(hash) * spreadRadius` to approximate the original's area-uniform distribution.

### Changed: star count defaults

Research plan proposed 200 default, 500 max. Increased to 500 default, 1000 max to get closer to the original's 1000-particle density.

### Known difference: center brightness

The center is still brighter than the reference. Three factors in the original prevent center blowout that we only partially replicate:
1. **2D random scatter** -- we approximate with sqrt(hash) but it is not identical
2. **Subtractive decay** (`color - 0.007`) -- every pixel loses the same flat amount per frame regardless of brightness. Our multiplicative decay (`color * factor`) takes longer to drain bright pixels, so center accumulation persists more.
3. **Reinhard tonemap** -- the original's Image pass compresses high values. We cannot use this (washes out colors in our pipeline).

Switching to subtractive decay would be the highest-impact fix for center brightness, but would diverge from the Attractor Lines / Muons pattern used by other generators in this codebase.

### Known difference: trail character

Multiplicative decay produces narrower trails than the original's subtractive decay. With subtractive, both bright centers and dim edges lose the same amount per frame, so trails maintain full width as they fade. With multiplicative, dim edges drop below visibility faster, producing thinner, sharper trails.
