# Slit Scan Corridor

A self-contained ping-pong effect that simulates the 2001: A Space Odyssey stargate sequence without a history buffer. Each frame, a 2-pixel-wide vertical slit is sampled from the input scene and stamped at the center of an accumulation texture. Old slits push outward with perspective-accelerated motion — slow at center (far away), fast at edges (rushing past the viewer). The left pixel of the slit becomes the left wall, the right pixel becomes the right wall. The result is a corridor of light streaking outward from a vanishing point.

## Classification

- **Category**: TRANSFORMS > Motion
- **Pipeline Position**: Transform chain (user-ordered), composited via blend compositor
- **Compute Model**: Ping-pong render textures (same pattern as Attractor Lines)

## References

- `src/effects/attractor_lines.cpp` — Ping-pong + blend compositor pattern (this codebase)
- `src/effects/fireworks.cpp` — Another ping-pong generator for reference
- User-designed algorithm — no external reference; technique combines standard UV remapping with temporal accumulation

## Algorithm

### Per-Frame Shader (ping-pong write pass)

The shader reads two textures: the previous accumulation (`previousFrame`) and the current scene (`sceneTexture`). For each output pixel:

1. **Horizontal distance from center**: `d = abs(u - 0.5)` where `u` is the horizontal UV

2. **Perspective-accelerated inward sample**: To simulate outward motion, sample the old frame at a UV shifted toward center. The shift uses perspective foreshortening so near-center pixels barely move while edge pixels rush past:
   - `sampleD = d / (1.0 + speed * deltaTime * d * perspective)`
   - `sampleU = 0.5 + sign(u - 0.5) * sampleD`
   - Sample `previousFrame` at `(sampleU, v)` and multiply by decay factor

3. **Stamp fresh slit at center**: When `d` is very small (within slit width), replace with a fresh sample from the scene:
   - Left of center (`u < 0.5`): sample `sceneTexture` at `(slitPosition - halfPixel, v)` — left pixel of the slit
   - Right of center (`u >= 0.5`): sample `sceneTexture` at `(slitPosition + halfPixel, v)` — right pixel of the slit
   - Blend via `smoothstep(slitWidth, 0.0, d)` so the slit feathers into the accumulation

4. **Output rotation**: Applied when the accumulation texture is composited to screen (not inside the ping-pong loop, to avoid rotational drift). Rotation angle accumulates from `rotationSpeed * deltaTime` plus static `rotationAngle`. Applied as standard 2D rotation around center in the blend compositor setup or a final display pass.

### Decay

Half-life-based decay identical to Attractor Lines: `decayFactor = exp(-0.693147 * deltaTime / halfLife)`. Old slits naturally dim as they approach the edges, creating depth cue.

### Wall Appearance

Top and bottom of the corridor fade to black — there is no floor or ceiling. The vertical dimension simply shows the slit's content at each Y position. A vignette-style vertical fade at top/bottom edges reinforces the corridor framing.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| slitPosition | float | 0.0-1.0 | 0.5 | Horizontal position to sample the 2-pixel slit from the input scene |
| speed | float | 0.1-10.0 | 2.0 | How fast the corridor advances (world units/sec) |
| perspective | float | 0.5-8.0 | 3.0 | Foreshortening strength — higher values create deeper, more dramatic tunnel |
| slitWidth | float | 0.001-0.05 | 0.005 | Feathering radius for the center slit stamp |
| decayHalfLife | float | 0.1-10.0 | 3.0 | Trail brightness half-life in seconds |
| brightness | float | 0.1-3.0 | 1.0 | Fresh slit brightness multiplier |
| rotationAngle | float | -PI..PI | 0.0 | Static rotation of the output corridor |
| rotationSpeed | float | -PI..PI | 0.0 | Rotation rate (rad/s) |

## Modulation Candidates

- **slitPosition**: Scanning the slit across the screen creates changing wall content — slow LFO gives gradual scene survey, audio-reactive makes walls flash to different parts of the input on beats
- **speed**: Faster = rushing through the tunnel, slower = drifting. Maps naturally to energy/tempo
- **perspective**: Higher = tighter vanishing point, more dramatic foreshortening. Lower = flatter walls
- **decayHalfLife**: Short = only nearby slits visible (short tunnel), long = deep corridor stretching into the distance
- **brightness**: Intensity of the fresh slit stamp — gates how vivid new content appears
- **rotationSpeed**: Spinning the entire corridor view

### Interaction Patterns

- **speed x decayHalfLife (competing forces)**: Speed determines how fast new slits are introduced; decay determines how long old slits survive. Fast speed + short decay = frantic short tunnel. Slow speed + long decay = deep serene corridor. The apparent corridor "length" emerges from their balance — modulating both with different audio bands creates a corridor that breathes in depth.
- **slitPosition x speed (cascading threshold)**: When slitPosition changes rapidly, new wall content only becomes visible if speed is high enough to push old content out of the way. Slow speed with rapid slit movement creates a smeared, blended wall. Fast speed lets each new slit position appear as a distinct stripe.
- **perspective x speed (resonance)**: At high perspective, edge pixels rush past extremely fast. Combined with high speed, the outer edges become pure blur while the center stays sharp. When both peak together, the tunnel focus snaps inward dramatically.

## Notes

- The ping-pong runs every frame regardless of whether the effect's blend is visible — consider gating the render pass on `enabled`
- Rotation must NOT be applied inside the ping-pong loop (each frame would compound the rotation, causing drift). Rotation applies only at composite time.
- The scene texture must be bound as a second sampler in the ping-pong shader — check how Attractor Lines handles `previousFrame` + `gradientLUT` dual-texture binding for the pattern
- Vertical fade (top/bottom darkening) can be a simple `smoothstep` in the shader based on `abs(v - 0.5)` — not a separate parameter unless the user wants control
- `halfPixel = 0.5 / screenWidth` for the slit offset — ensures left and right pixels are adjacent in the source
