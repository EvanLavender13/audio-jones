# Cymatics Boundary Reflections

Mirror source technique to simulate wave reflections at screen edges, creating standing wave interference patterns.

## Classification

- **Type**: Enhancement to existing Simulation (Cymatics)
- **Compute Model**: Same as current—compute shader, no architectural change

## References

- [Method of Images (Wikipedia concept)](https://en.wikipedia.org/wiki/Method_of_images) - Classical technique for solving boundary value problems by placing virtual sources at reflected positions
- Existing implementation: `shaders/cymatics.glsl:50-59`

## Algorithm

### Current Behavior

Each pixel sums wave contributions from N real sources:
```glsl
for (int i = 0; i < sourceCount; i++) {
    float dist = length(uv - sources[i]);
    float delay = dist * waveScale;
    totalWave += fetchWaveform(delay) * attenuation;
}
```

Waves radiate outward and fade. No reflection.

### Mirror Source Method

For boundaries at `x = ±aspectRatio` and `y = ±1`, reflect each source across all four edges:

```glsl
// For source at (sx, sy), mirrors are:
vec2 mirrorLeft  = vec2(-2.0 * aspect - sx, sy);
vec2 mirrorRight = vec2( 2.0 * aspect - sx, sy);
vec2 mirrorBottom = vec2(sx, -2.0 - sy);
vec2 mirrorTop    = vec2(sx,  2.0 - sy);
```

Sample from both real and mirror sources. Mirror waves enter the visible area as if reflected off the boundary.

### Reflection Generations

- **1st order**: 4 mirrors per source (one per edge). Cost: 5× samples.
- **2nd order**: Add corner reflections (4 more) + edge-to-edge (4 more). Cost: 13× samples.
- **Recommendation**: 1st order only. Diminishing visual returns beyond that.

### Attenuation Consideration

Mirror sources are farther away, so Gaussian falloff naturally reduces their contribution. May need to reduce `falloff` parameter when boundaries enabled to let reflections remain visible.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| boundaries | bool | — | false | Enable/disable edge reflections |
| reflectionGain | float | 0.0–1.0 | 1.0 | Attenuate mirror contributions (optional, for tuning) |

## Modulation Candidates

- **reflectionGain**: Fade reflections in/out for evolving patterns

## Implementation Notes

### Shader Changes (`cymatics.glsl`)

Add uniform and mirror sampling loop:
```glsl
uniform bool boundaries;

// Inside main loop, after sampling real source:
if (boundaries) {
    // Sample 4 mirror positions with same delay/attenuation logic
}
```

### CPU Changes (`cymatics.cpp`)

- Add `boundaries` field to `CymaticsConfig`
- Pass uniform to shader

### Performance

With 8 sources × 5 samples = 40 texture fetches per pixel. Current is 8. Still well within budget for a compute shader at 1080p.

## Visual Result

- **Off**: Current behavior—outward radiating waves, interference only between sources
- **On**: Waves appear to bounce off screen edges, creating denser interference patterns and pseudo-standing-waves near boundaries

Not true resonance modes, but visually similar to bounded cymatics plates.
