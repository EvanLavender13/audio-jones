# Constellation Enhancements: Triangle Fill + Directional Wave

Two enhancements to the existing constellation generator: filled triangles between close points, and a movable wave center replacing the fixed radial origin.

## Classification

- **Category**: GENERATORS (enhancement to existing constellation effect)
- **Pipeline Position**: Unchanged — after feedback, before transforms
- **Chosen Approach**: Minimal — both features modify the existing shader and config; no new files

## References

- [Inigo Quilez :: 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) — triangle SDF formula (sdTriangle)
- [BigWings Plexus tutorial](https://www.youtube.com/watch?v=3CycKKJiwis) — user-provided reference shaders showing perimeter-gated triangle fill
- [Godot Plexus Particles](https://godotshaders.com/shader/plexus-particles/) — plexus connection patterns

## Algorithm

### Triangle Fill

After computing the 9 grid-cell positions, iterate all unique triples (i, j, k) where i < j < k. For each triple:

1. Compute perimeter: `len(p[i]-p[j]) + len(p[j]-p[k]) + len(p[i]-p[k])`
2. Skip if perimeter exceeds threshold (tight triangles only)
3. Evaluate iq's `sdTriangle(gv, p[i], p[j], p[k])` — negative inside, positive outside
4. Apply smoothstep for soft edge: `smoothstep(0.0, -blur, dist)` where blur ~ lineThickness
5. Color by interpolating the 3 vertex gradient-LUT colors at the centroid using barycentric weights

The triangle SDF from iq (exact, no approximation):

```
sdTriangle(p, p0, p1, p2):
  e0=p1-p0, e1=p2-p1, e2=p0-p2
  v0=p-p0, v1=p-p1, v2=p-p2
  pq0 = v0 - e0*clamp(dot(v0,e0)/dot(e0,e0), 0, 1)
  pq1 = v1 - e1*clamp(dot(v1,e1)/dot(e1,e1), 0, 1)
  pq2 = v2 - e2*clamp(dot(v2,e2)/dot(e2,e2), 0, 1)
  s = sign(e0.x*e2.y - e0.y*e2.x)
  d = min(min(vec2(dot(pq0,pq0), s*(v0.x*e0.y-v0.y*e0.x)),
              vec2(dot(pq1,pq1), s*(v1.x*e1.y-v1.y*e1.x))),
          vec2(dot(pq2,pq2), s*(v2.x*e2.y-v2.y*e2.x)))
  return -sqrt(d.x)*sign(d.y)
```

Triangle iteration count: C(9,3) = 84 triples. With early perimeter rejection, most skip the SDF entirely. The references use a perimeter threshold of ~2.8 in grid-cell units; our equivalent depends on `wanderAmp` and `spiralSpacing`. A reasonable default is 2.5 (grid-cell-space units).

### Barycentric Color Interpolation

For a pixel inside triangle (a, b, c), compute barycentric weights:

```
w0 = area(p, b, c) / area(a, b, c)
w1 = area(a, p, c) / area(a, b, c)
w2 = 1.0 - w0 - w1
color = w0*colorA + w1*colorB + w2*colorC
```

Where `area(x,y,z) = cross(y-x, z-x)` (signed area, use abs for weights). Vertex colors come from the existing point gradient LUT sampled at `N21(cellID)` per vertex.

### Movable Wave Center

Replace the current radial wave system:

**Current**: `distance = length(cellID + cellOffset)` — always radiates from grid origin (screen center).

**New**: `distance = length(cellID + cellOffset - waveCenter * gridScale)` — radiates from `waveCenter` (in UV space, 0-1).

- Setting waveCenter to (0.5, 0.5) reproduces the current radial behavior
- Dragging waveCenter off-screen (e.g., (-1, 0.5)) creates a directional wave sweeping left-to-right
- The farther off-screen, the more planar (less curved) the wavefronts become

The existing `radialFreq`, `radialAmp`, `radialSpeed` params remain unchanged — they control the wave's spatial frequency, displacement strength, and propagation speed respectively. Only the origin point changes.

## Parameters

### New Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| fillEnabled | bool | - | false | Toggle triangle fills on/off |
| fillOpacity | float | 0.0 - 1.0 | 0.3 | Triangle fill brightness |
| fillThreshold | float | 1.0 - 4.0 | 2.5 | Max perimeter for visible triangles (grid-cell units) |
| waveCenterX | float | -2.0 - 3.0 | 0.5 | Wave origin X in UV space |
| waveCenterY | float | -2.0 - 3.0 | 0.5 | Wave origin Y in UV space |

### Removed Parameters

| Parameter | Replaced By |
|-----------|-------------|
| (none removed) | waveCenterX/Y extend the existing radial system |

### Renamed Parameters (for clarity)

| Old Name | New Name | Reason |
|----------|----------|--------|
| radialFreq | waveFreq | No longer exclusively radial |
| radialAmp | waveAmp | No longer exclusively radial |
| radialSpeed | waveSpeed | No longer exclusively radial |

## Modulation Candidates

- **fillOpacity**: triangle fills pulse with energy
- **waveCenterX / waveCenterY**: wave origin drifts across screen
- **waveAmp**: wave displacement strength reactive to transients

## Notes

- 84 triangle candidates per pixel sounds heavy, but the perimeter check is 3 `length()` calls + 1 comparison. Only triangles passing the threshold evaluate the SDF (6 dot products + 3 clamp + 1 sqrt). With tight threshold ~2.5, expect 5-15 triangles to survive per pixel.
- Triangle fills accumulate additively. Combined with existing lines and points, total brightness may exceed 1.0. The existing blend compositing (screen mode) handles this, and bloom further softens hot spots.
- The wave center range extends beyond [0,1] to allow off-screen positioning. At waveCenterX = -2.0, wavefronts are nearly parallel vertical lines sweeping right — effectively a directional wave.
- Renaming radial* to wave* changes preset serialization keys. Migration: deserialize both old and new names in preset.cpp for backward compatibility.
- The `fillThreshold` operates in grid-cell-space units (after UV * gridScale). Changing `gridScale` does not require adjusting the threshold.
