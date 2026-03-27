# Lotus Warp

Infinitely spiraling diamond cells created by a conformal log-polar coordinate warp. The input texture tiles inward along logarithmic spirals, each cell showing a warped copy of the underlying image. Shares all 9 CellMode sub-effects with Voronoi and Phyllotaxis. Independent zoom and spin speeds create bidirectional spiral flow.

## Classification

- **Category**: TRANSFORMS > Cellular
- **Pipeline Position**: Transform stage (section 2, alongside Voronoi/Phyllotaxis)
- **Flags**: `EFFECT_FLAG_NONE`

## Attribution

- **Based on**: "logpolar mapping - 2" by FabriceNeyret2
- **Source**: https://www.shadertoy.com/view/3sSSDR
- **License**: CC BY-NC-SA 3.0

## References

- [Log-spherical Mapping in SDF Raymarching](https://www.osar.fr/notes/logspherical/) - Log-polar tiling math, fract-based cell extraction, seamless tiling constraints

## Reference Code

FabriceNeyret2's logpolar mapping - 2. Fork of https://www.shadertoy.com/view/3dSSDR

```glsl
void mainImage( out vec4 O, vec2 u )
{
    vec2 R = iResolution.xy,
         U = u+u-floor(R/4.)*4., // trick to avoid angle derivative discontinuity

         p = vec2(log(length(U)/R.y)-.3*iTime, atan(U.y,U.x) );

 // O = texture(iChannel0, .637*p );           // .637 = 2./pi

    O = texture(iChannel0, .637*p* mat2(1,-1,1,1) ) *4.5-2.;

 // O = texture(iChannel0, p* mat2(cos(iTime+ vec4(0,33,11,0))) ); // https://www.shadertoy.com/view/XlsyWX
}
```

## Algorithm

### Substitution table

| Reference | Ours | Notes |
|-----------|------|-------|
| `u+u-floor(R/4.)*4.` | `fragTexCoord * resolution - resolution * 0.5` | Standard center-relative coords per conventions |
| `log(length(U)/R.y)` | `log(length(pos) / resolution.y)` | Log-radial component of log-polar transform |
| `-.3*iTime` (in log component) | `- zoomPhase` uniform | CPU-accumulated: `zoomPhase += zoomSpeed * deltaTime` |
| `atan(U.y,U.x)` | `atan(pos.y, pos.x) - spinPhase` | CPU-accumulated: `spinPhase += spinSpeed * deltaTime` |
| `.637` (2/PI normalization) | `TWO_OVER_PI` constant (0.6366) | Normalizes angular period to 4 units for clean tiling |
| `mat2(1,-1,1,1)` | Keep verbatim | Conformal 45-degree rotation: `(p.x - p.y, p.x + p.y)` |
| `texture(iChannel0, transformed)` | Cell geometry computation + CellMode sub-effects | Replace direct texture sample with cellular warp pipeline |
| `*4.5-2.` (contrast boost) | Remove | Not needed - intensity and sub-effects handle contrast |

### Forward transform chain

```
1. pos = fragTexCoord * resolution - resolution * 0.5
2. lp = vec2(log(length(pos) / resolution.y) - zoomPhase,
             atan(pos.y, pos.x) - spinPhase)
3. conformal = TWO_OVER_PI * lp * mat2(1,-1,1,1)
4. cellCoord = conformal * scale
5. ip = floor(cellCoord)       // cell ID
6. fp = fract(cellCoord)       // position within cell [0,1)
```

### Cell geometry from fract coordinates

After fract(), each cell is a unit square. Cell geometry for sub-effects:

```
toCenter = vec2(0.5) - fp          // vector from pixel to cell center (matches Voronoi mr)
centerDist = length(toCenter)

edgeDistX = min(fp.x, 1.0 - fp.x)
edgeDistY = min(fp.y, 1.0 - fp.y)
edgeDist = min(edgeDistX, edgeDistY)

borderVec: direction toward nearest cell edge
  - if edgeDistX < edgeDistY: vec2(sign(0.5 - fp.x) * edgeDistX, 0)
  - else:                     vec2(0, sign(0.5 - fp.y) * edgeDistY)
```

### Cell center UV (inverse transform)

To sample the input texture at each cell's center (for cellColor in sub-effects), inverse-map the cell center back to screen UV:

```
cellCenterLP = (ip + 0.5) / scale
// Undo conformal: inverse of mat2(1,-1,1,1) is mat2(1,1,-1,1) * 0.5
cellCenterLP = (cellCenterLP * mat2(1,1,-1,1)) * 0.5 / TWO_OVER_PI
// Add back animation phases
rho = cellCenterLP.x + zoomPhase
theta = cellCenterLP.y + spinPhase
// Inverse log-polar
r = exp(rho) * resolution.y
cartesian = r * vec2(cos(theta), sin(theta))
// To UV
cellCenterUV = (cartesian + resolution * 0.5) / resolution
```

### Smooth mode adaptation

Voronoi/Phyllotaxis use inverse-distance neighbor weighting for smooth mode. For grid cells, smooth mode instead softens cell boundaries: apply smoothstep to edge distances to round cell corners, and blend toCenter toward a circular falloff rather than a square cell boundary.

### Sub-effect dispatch

All 9 CellMode sub-effects (Distort, Organic Flow, Edge Iso, Center Iso, Flat Fill, Edge Glow, Ratio, Determinant, Edge Detect) apply identically to Voronoi's implementation, using the grid-derived `toCenter`, `centerDist`, `edgeDist`, and `borderVec` values. The UV displacement modes (0, 1) need to inverse-transform the displacement back to screen space, which requires dividing by the local Jacobian scale (approximately `length(pos) * TWO_OVER_PI * scale`).

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| scale | float | 0.5-10.0 | 3.0 | Cell density (cells across screen) |
| zoomSpeed | float | -PI_F to PI_F | 0.3 | Radial zoom scroll rate (rad/s), accumulated on CPU |
| spinSpeed | float | -PI_F to PI_F | 0.0 | Angular rotation rate (rad/s), accumulated on CPU |
| edgeFalloff | float | 0.01-1.0 | 0.3 | Edge softness for cell sub-effects |
| isoFrequency | float | 1.0-30.0 | 10.0 | Ring density for iso effects |
| mode | int | 0-8 | 0 | CellMode sub-effect index (shared enum) |
| intensity | float | 0.0-1.0 | 0.5 | Sub-effect strength |
| smoothMode | bool | - | false | Soften cell boundaries with rounded edges |

## Modulation Candidates

- **scale**: Pulse cell density - large cells on bass hits, dense cells in quiet sections
- **zoomSpeed**: Accelerate or reverse the infinite zoom
- **spinSpeed**: Speed or reverse angular rotation
- **edgeFalloff**: Modulate edge sharpness for breathing cell borders
- **intensity**: Overall effect strength

### Interaction Patterns

**zoomSpeed vs spinSpeed (competing forces)**: When modulated by different audio bands, these create complex spiral trajectories. Bass-driven zoom with treble-driven spin produces sections where cells flow inward during heavy bass and orbit during high-frequency passages. When both peak simultaneously, the spiral motion intensifies - rare, dramatic moments. When both are near zero, the pattern freezes into a static stained-glass window.

**scale vs intensity (cascading threshold)**: At low scale (few large cells), the cell pattern is coarse and each sub-effect covers a wide area. At high scale (many small cells), the same intensity creates fine, dense patterning. Modulating scale with one source while intensity tracks another means the visual density of the effect shifts with the music - chorus sections can reveal intricate small-cell detail while verses show broad, simple warps.

## Notes

- The `atan()` discontinuity at angle +/-PI (left edge of screen) can produce a visible seam if `4 * scale` is not an integer. Integer and half-integer scale values guarantee seamless angular tiling (the 2/PI normalization maps the angular period to exactly 4 units).
- The log-polar transform has a singularity at the center (log(0) = -inf). Clamp `length(pos)` to a small minimum (e.g., 1.0) to avoid NaN.
- Cells near the center are visually smaller (logarithmic compression). This is inherent to the mapping and is the defining visual quality of the effect.
- The conformal mat2(1,-1,1,1) creates diamond-shaped cells tilted at 45 degrees. This is a fixed geometric property, not a tunable parameter.
- UV displacement modes (0, 1) need care: the displacement vector in cell space must be scaled by the inverse Jacobian to produce correct screen-space UV offsets. The Jacobian scale varies with radius (larger near center, smaller at edges).
- Smooth mode for grid cells is simpler than Voronoi's neighbor-weighting approach since there is no neighbor search loop to modify - it only changes how edge/center distances are computed.
