# Fracture Grid V2

Enhancements to the existing Fracture Grid transform: per-tile flip/mirror and skew transforms, plus three wave propagation modes (directional sweep, radial ripple, Manhattan cascade) that give large-scale spatial structure to the tile animations.

## Classification

- **Category**: TRANSFORMS > Cellular
- **Pipeline Position**: Existing effect enhancement (Fracture Grid)

## References

- [The Book of Shaders: 2D Matrices](https://thebookofshaders.com/08/) - Shear/rotation matrix construction
- [The Book of Shaders: Patterns](https://thebookofshaders.com/09/) - Per-tile transforms in fragment shaders

## Reference Code

Existing `computeTileWarp` from `shaders/fracture_grid.fs` - the function that applies per-tile transforms:

```glsl
vec2 computeTileWarp(vec2 tileId, vec2 tileCellUV, vec2 tileCellCenter, float sub) {
    vec3 h = mix(hash3(tileId), computeSpatial(tileCellCenter, waveTime), spatialBias);
    float phase = h.x * 6.283;
    vec2 offset = (h.xy - 0.5) * stagger * offsetScale
                * shapedWave(waveTime + phase, waveShape);
    float angle = (h.z - 0.5) * stagger * rotationScale
                * shapedWave(waveTime * 1.3 + phase, waveShape);
    float zoom = max(0.2, 1.0 + (h.y - 0.5) * stagger * zoomScale
                * shapedWave(waveTime * 0.7 + phase, waveShape));
    return tileCellCenter + rot2(angle) * (tileCellUV / (sub * zoom)) + offset;
}
```

Standard 2D shear matrix:

```glsl
mat2 shear2(float sx, float sy) {
    return mat2(1.0, sy, sx, 1.0);
}
```

## Algorithm

All changes are additive to the existing shader. No existing behavior changes at default values.

### A. Flip/Mirror

Add a `flipChance` uniform (0-1). In `computeTileWarp`, use two extra hash channels to decide per-tile horizontal and vertical flip:

```glsl
// Need a second hash call for independent flip channels
vec3 h2 = hash3(tileId + vec2(127.1, 311.7));
vec2 flip = vec2(1.0);
if (h2.x < flipChance) flip.x = -1.0;
if (h2.y < flipChance) flip.y = -1.0;
```

Apply flip to `tileCellUV` before other transforms. The mirror-wrap at the end of `main()` already handles UVs outside [0,1], so flipped tiles sample correctly.

### B. Skew/Shear

Add a `skewScale` uniform (0-1). In `computeTileWarp`, derive per-tile shear amounts from hash and apply as a matrix multiply alongside rotation:

```glsl
float shearAmt = (h2.z - 0.5) * stagger * skewScale
               * shapedWave(waveTime * 0.9 + phase, waveShape);
```

Compose with existing rotation: `rot2(angle) * shear2(shearAmt, 0.0)` applied to `tileCellUV`. Single-axis shear (x-axis) is sufficient - rotation already provides directional variety, so shearing both axes would be redundant.

### C. Wave Propagation

Add three uniforms:
- `propagationMode` (int): 0=none (current behavior), 1=directional, 2=radial, 3=cascade
- `propagationSpeed` (float): scales the phase offset from distance
- `propagationAngle` (float, radians): sweep direction for mode 1

Compute a distance-based phase offset added to each tile's random phase:

```glsl
float propPhase = 0.0;
if (propagationMode == 1) {
    // Directional sweep along configurable angle
    vec2 dir = vec2(cos(propagationAngle), sin(propagationAngle));
    propPhase = dot(tileCellCenter, dir) * propagationSpeed;
} else if (propagationMode == 2) {
    // Radial ripple from center
    propPhase = length(tileCellCenter) * propagationSpeed;
} else if (propagationMode == 3) {
    // Manhattan cascade - diamond/staircase wavefront
    propPhase = (abs(tileCellCenter.x) + abs(tileCellCenter.y)) * propagationSpeed;
}
```

Replace `phase` usage: `shapedWave(waveTime + phase, ...)` becomes `shapedWave(waveTime + phase + propPhase, ...)`. The random per-tile phase is preserved so tiles still have individual character within the propagation pattern.

### Composition

The full transform chain in `computeTileWarp` becomes:

```
tileCellUV *= flip                          (A: mirror)
warped = rot2(angle) * shear2(shear, 0)     (B: skew composed with existing rotation)
         * (tileCellUV / (sub * zoom))       (existing zoom)
result = tileCellCenter + warped + offset    (existing offset)
```

All wave-driven values (offset, angle, shear, zoom) use `waveTime + phase + propPhase` instead of `waveTime + phase`.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| flipChance | float | 0.0-1.0 | 0.0 | Probability of per-tile horizontal/vertical flip |
| skewScale | float | 0.0-1.0 | 0.0 | Max shear intensity per tile |
| propagationMode | int | 0-3 | 0 | 0=none, 1=directional, 2=radial, 3=cascade |
| propagationSpeed | float | 0.0-20.0 | 5.0 | Phase offset scale from distance |
| propagationAngle | float | -PI..PI | 0.0 | Sweep direction (directional mode only, radians) |

## Modulation Candidates

- **flipChance**: low values = occasional flipped tile accents; high values = full kaleidoscope chaos
- **skewScale**: tiles lean and relax with the music, gives organic breathing quality
- **propagationSpeed**: faster = tighter wavefronts with more visible bands; slower = broad gentle sweeps
- **propagationAngle**: rotating the sweep direction creates a spinning wave pattern across the grid

### Interaction Patterns

- **flipChance x stagger** (cascading threshold): at low stagger, flips are the only visible per-tile variation - tiles snap between original and mirrored. As stagger rises, the flips get buried in the offset/rotation chaos. Low stagger + modulated flipChance creates a clean binary flicker; high stagger makes it invisible.
- **propagationSpeed x spatialBias** (competing forces): propagation imposes geometric order (wavefronts). Spatial bias imposes organic order (noise clusters). At high values of both, they fight - the wavefront tries to sweep linearly but the spatial noise pulls clusters out of sync. The tension between geometric and organic phase creates unpredictable pockets of alignment.
- **skewScale x rotationScale** (resonance): when both peak simultaneously, tiles undergo extreme angular distortion - the shear amplifies the rotation into a stretched spiral. When only one peaks, the distortion stays controlled. Modulating both from the same source creates synchronized intensity; different sources create moments of rare combined extremes.

## Notes

- All new params default to 0/off, so existing presets are unaffected
- `propagationAngle` only matters when `propagationMode == 1`; UI should show/hide conditionally
- The second `hash3()` call for flip channels uses an offset seed (`tileId + vec2(127.1, 311.7)`) to avoid correlation with the existing hash
- `propagationSpeed` range is wide (0-20) because the phase offset scales with cell center distance, which varies by subdivision count. Higher subdivisions need higher speed for the same visual wavefront density.
