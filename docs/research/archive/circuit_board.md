# Circuit Board (Squarenoi)

Warps input imagery along SDF-based square Voronoi cell boundaries. Each grid cell randomly picks a shape (box, circle, or diamond), and the "second-closest distance" trick merges neighboring cells into flowing trace-like pathways — creating PCB/circuit board aesthetics from a fundamentally different algorithm than the old triangle-wave fold approach. Cell sizes breathe with time; an optional dual-layer mode overlaps two grids for denser trace networks.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Transform stage (user-ordered with other transforms)
- **Replaces**: Existing Circuit Board warp (triangle wave folding algorithm)
- **Chosen Approach**: Balanced with dual-layer toggle — faithful reproduction of the reference squarenoi algorithm with per-cell shape mix and configurable dual-grid interference. No chromatic mode. No neon glow coloring.

## References

- User-provided Shadertoy "squarenoi" shader — source algorithm for SDF square Voronoi with second-closest merging, murmur hash, and dual-layer evaluation
- [Voronoi Edges](https://iquilezles.org/articles/voronoilines/) — Inigo Quilez: F1/F2 distance computation, second-closest cell distance theory, and perpendicular bisector edge detection
- [Understanding Cellular Noise Variations](https://sangillee.com/_posts/2025-04-18-Cellular-noises/) — F1/F2 distance metrics and Voronoi pattern variations

## Algorithm

### Hash Function

Murmur hash producing 3 outputs from 3 inputs. Operates on uint bit representation for uniform distribution:

```glsl
uvec3 murmurHash33(uvec3 src) {
    const uint M = 0x5bd1e995u;
    uvec3 h = uvec3(1190494759u, 2147483647u, 3559788179u);
    src *= M; src ^= src >> 24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z;
    h ^= h >> 13u; h *= M; h ^= h >> 15u;
    return h;
}

vec3 hash(vec3 src) {
    uvec3 h = murmurHash33(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}
```

Each grid cell receives 3 random values in [0, 1): `rng.xy` positions the cell's seed point, `rng.z` selects the cell's shape and seeds animation phase.

### Cell Point Lookup

```glsl
vec3 get_point(vec2 pos) {
    ivec2 p = (ivec2(pos) - 1) * 4;
    vec3 rng = hash(vec3(float(p.x), float(p.y), float(p.x + p.y)));
    return rng.xyz;
}
```

The `(ivec2 - 1) * 4` mapping spaces hash inputs to avoid correlation between adjacent cells.

### Per-Cell Shape Selection

The reference uses `sdBox` for all cells. The extension: use `rng.z` to select one of three SDF primitives per cell:

- **Box** (`rng.z < 0.5`): `max(abs(p.x) - rad.x, abs(p.y) - rad.y)` — rectangular cells (IC chips, pads)
- **Circle** (`0.5 <= rng.z < 0.8`): `length(p) - rad.x` — round cells (capacitors, vias)
- **Diamond** (`rng.z >= 0.8`): `abs(p.x) + abs(p.y) - rad.x` — rotated square cells (small components)

The shape thresholds bias toward boxes (~50%), then circles (~30%), then diamonds (~20%) — favoring the rectangular PCB aesthetic while adding variety.

### Cell Size Animation

Cell SDF radii oscillate per-cell using the random phase from `rng.z`:

```
rad = vec2(baseSize + cos(time * speed + rng.z * TAU) * breathe,
           baseSize + sin(time * speed + rng.z * TAU) * breathe)
```

`baseSize` controls the static cell radius. `breathe` controls oscillation amplitude. `speed` controls rate. Each cell's phase offset from `rng.z` prevents synchronization — cells breathe independently.

### 24-Neighbor Search with Second-Closest Trick

For each pixel, search all 24 neighbors (5x5 grid minus center cell). Track both the closest and second-closest SDF distances:

1. Compute SDF distance to current cell's shape
2. For each of 24 neighbor offsets: compute SDF to that neighbor's shape
3. Maintain sorted pair: `dists.x` = closest, `dists.y` = second-closest
4. **Output the second-closest distance** — this creates the merged boundary pattern

The second-closest distance produces pathways where cells merge rather than hard Voronoi borders. Regions where two cells compete for "closest" show flowing trace-like seams.

### Displacement Generation

The signed distance from the squarenoi evaluation maps to UV displacement:

```
displacement = sdVoronoi(scaledUV)
finalUV = uv + strength * displacement * warpDirection
```

Where `warpDirection` derives from the local SDF gradient (central differences on the squarenoi field) to push pixels perpendicular to traces. Pixels near trace edges displace outward; pixels in cell interiors stay put.

### Dual-Layer Mode

When enabled, evaluate the squarenoi twice at different grid offsets and sum the results:

```
d = sdVoronoi(uv) + sdVoronoi(uv + layerOffset)
```

`layerOffset` (default 40.0 as in reference) controls how different the two grids are. Large offsets produce uncorrelated grids with complex crossing patterns. Small offsets produce subtle interference. This doubles shader cost but creates significantly denser trace networks.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| tileScale | float | 2.0-16.0 | 8.0 | Grid density — higher = more, smaller cells |
| strength | float | 0.0-1.0 | 0.3 | Warp displacement intensity |
| baseSize | float | 0.3-0.9 | 0.7 | Static cell radius before animation |
| breathe | float | 0.0-0.4 | 0.2 | Cell size oscillation amplitude |
| breatheSpeed | float | 0.1-4.0 | 1.0 | Cell size oscillation rate |
| dualLayer | bool | — | false | Enable second overlapping grid evaluation |
| layerOffset | float | 5.0-80.0 | 40.0 | Spatial offset between the two grids (only when dualLayer on) |
| shapeMix | float | 0.0-1.0 | 0.5 | Bias toward boxes (0.0 = all boxes) vs mixed shapes (1.0 = more circles/diamonds) |

## Modulation Candidates

- **strength**: Warp intensity pulses — traces push harder into the image during peaks
- **breathe**: Cell oscillation amplitude — quiet = steady cells, loud = jittery pulsing
- **tileScale**: Grid density shifts — zooms in/out on the trace pattern
- **baseSize**: Cell radius change — traces thicken or thin
- **shapeMix**: Bias between uniform rectangles and mixed components
- **layerOffset**: Drift between the two grids when dual-layer is on

### Interaction Patterns

- **Strength x Breathe (Resonance)**: When both spike simultaneously, cells are at their most distorted *and* displacement is highest — producing rare moments of extreme warping that quickly relax. Either value alone produces mild effect; together they amplify into dramatic bursts.

- **BaseSize x TileScale (Competing Forces)**: Large baseSize with high tileScale packs fat cells into a dense grid — traces compress into thin seams. Small baseSize with low tileScale creates sparse islands of small cells with wide empty gaps. The visual character (dense traces vs floating islands) depends on their ratio, not either value alone.

## Notes

- Replaces the existing Circuit Board warp. The old triangle-wave folding files (circuit_board.cpp/h, circuit_board.fs, shader setup, UI entries, config struct, param registration) are overwritten — same file names, completely new algorithm.
- The 24-neighbor search (5x5 grid minus center) is the main performance cost. At tileScale=8 this evaluates 24 SDF calls per pixel (48 with dual layer). Keep iterations implicit in the neighbor count — no user-facing iteration parameter.
- The murmur hash produces excellent distribution without visible grid artifacts. No fallback hash needed.
- UV clamping on final displaced coordinates prevents sampling outside [0,1].
- The reference's `+.001` UV offset to hide tiling artifacts at grid edges may or may not be needed — test during implementation.
