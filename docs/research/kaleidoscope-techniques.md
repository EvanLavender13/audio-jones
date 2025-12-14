# Kaleidoscope Shader Techniques

Background research for alternative kaleidoscope implementations beyond polar radial mirroring.

## Current Implementation

AudioJones uses polar coordinate kaleidoscope (`shaders/kaleidoscope.fs:17-45`). The shader:

1. Centers UV at screen origin (`uv - 0.5`)
2. Converts to polar coordinates (`radius`, `angle`)
3. Divides 2π into N segments via `mod(angle, segmentAngle)`
4. Mirrors within segments via `min(angle, segmentAngle - angle)`
5. Converts back to Cartesian

This approach creates **radial/circular symmetry** centered on screen. On rectangular displays, corners lie further from center than edges, producing uneven visual density.

## Tessellation Theory

Physical kaleidoscopes use angled mirrors that reflect light. Digital implementations simulate this via coordinate transformation. According to mathematical analysis, exactly four mirror arrangements produce seamless tessellating patterns:

| Shape | Angles | Tessellation Pattern |
|-------|--------|---------------------|
| Equilateral triangle | 60°-60°-60° | Hexagonal grid |
| Rectangle | 90°-90°-90°-90° | Rectangular grid |
| Halved-square triangle | 90°-45°-45° | Square grid |
| Halved-equilateral triangle | 90°-60°-30° | Hexagonal grid |

Other polygons (pentagons, irregular hexagons) fail to tessellate - reflections create visible seams where edges misalign.

## Alternative Techniques

### Cartesian Mirror Tiling

Replaces polar coordinates with rectangular reflection. Divides screen into NxM grid, mirrors content at each tile boundary.

```glsl
vec2 tile = uv * vec2(tilesX, tilesY);
vec2 f = fract(tile);
vec2 i = floor(tile);

// Mirror on odd tiles (GL_MIRRORED_REPEAT behavior)
f = mix(f, 1.0 - f, mod(i, 2.0));
```

**Characteristics:**
- Fills entire rectangle uniformly
- Produces axial (not radial) symmetry
- Simpler math than polar conversion
- Visual result resembles wallpaper patterns

**Limitation:** No rotational symmetry. Pattern appears grid-aligned rather than mandala-like.

### Triangle-Based Tessellation

Uses 45-45-90 triangles to tile into square grid with diagonal reflections.

```glsl
vec2 st = uv * scale;
vec2 i = floor(st);
vec2 f = fract(st);

// Determine which triangle within square
bool upperTriangle = (f.x + f.y) > 1.0;

if (upperTriangle) {
    f = 1.0 - f;  // Reflect across diagonal
}

// Mirror within triangle
f = abs(f);
if (f.y > f.x) {
    f = f.yx;  // Reflect across y=x line
}
```

**Characteristics:**
- 8-fold symmetry per square cell
- Tiles seamlessly across rectangular screen
- Combines rotation and reflection
- More complex than pure Cartesian but fills space better

### Wallpaper Group Symmetries

The 17 wallpaper groups define all possible 2D repeating symmetries. Relevant groups for shader implementation:

| Group | Symmetry Operations | Visual Character |
|-------|---------------------|------------------|
| **pm** | 2 parallel reflections | Simple mirror strips |
| **pmm** | 4 perpendicular reflections | Rectangular grid with mirrors |
| **cmm** | 2 reflections + 180° rotation | Diagonal crosses |
| **p4m** | 4-fold rotation + reflections | Kaleidoscopic squares |
| **p6m** | 6-fold rotation + reflections | Hexagonal kaleidoscope |

Group **p4m** produces the most kaleidoscope-like effect on rectangular screens:

```glsl
vec2 st = uv * scale;
vec2 i = floor(st);
vec2 f = fract(st);

// Center at (0.5, 0.5)
f -= 0.5;

// 4-fold rotation: reduce to first quadrant
f = abs(f);

// Reflect across diagonal
if (f.y > f.x) {
    f = f.yx;
}

// Sample from transformed coordinates
f += 0.5;
```

### Hybrid: Tiled Polar Kaleidoscope

Applies polar kaleidoscope within each cell of a rectangular grid.

```glsl
// First: tile into rectangular cells
vec2 tile = uv * gridSize;
vec2 cellUV = fract(tile);

// Then: polar kaleidoscope within cell
vec2 centered = cellUV - 0.5;
float radius = length(centered);
float angle = atan(centered.y, centered.x);

angle = mod(angle + rotation, segmentAngle);
angle = min(angle, segmentAngle - angle);

vec2 result = vec2(cos(angle), sin(angle)) * radius + 0.5;
```

**Characteristics:**
- Retains mandala aesthetic
- Multiple kaleidoscope copies fill screen
- Each cell independent - no cross-cell continuity
- Higher visual complexity

## Comparison Matrix

| Technique | Fills Rectangle | Symmetry Type | Continuity | Complexity |
|-----------|-----------------|---------------|------------|------------|
| Polar (current) | Circular only | Radial N-fold | Continuous | Low |
| Cartesian mirror | Full | Axial 2x2 | Continuous | Low |
| Triangle 45-45-90 | Full | Dihedral 8-fold | Continuous | Medium |
| Wallpaper p4m | Full | Dihedral 8-fold | Continuous | Medium |
| Hybrid tiled | Full (tiled) | Radial N-fold | Per-cell | Medium |

## Implementation Considerations

### UV Coordinate Handling

All techniques require careful boundary handling. `fract()` produces discontinuity at integer boundaries if derivatives not computed before transformation:

```glsl
// Compute derivatives BEFORE fract/mod
vec2 dx = dFdx(uv * scale);
vec2 dy = dFdy(uv * scale);
vec2 f = fract(uv * scale);

// Use textureGrad for correct filtering
vec4 color = textureGrad(tex, f, dx, dy);
```

### Aspect Ratio Correction

Rectangular screens require aspect compensation for circular symmetry:

```glsl
vec2 aspect = vec2(resolution.x / resolution.y, 1.0);
vec2 centered = (uv - 0.5) * aspect;
```

Without correction, circles appear as ellipses.

### Performance

All techniques run single-pass with O(1) per-pixel complexity. No technique significantly outperforms others:

| Technique | Operations/pixel | Notes |
|-----------|------------------|-------|
| Polar | 2 trig + 4 arith | atan, sin, cos |
| Cartesian | 4 arith | floor, fract, mix, mod |
| Triangle | 6 arith + 2 branch | Conditionals may diverge |
| Wallpaper p4m | 5 arith | abs, comparison |

## Recommendations

For AudioJones specifically:

**Add Cartesian mirror or p4m wallpaper** as alternative mode:
- Fills entire rectangular screen uniformly
- Complements linear waveforms better than polar radial symmetry
- Linear waveforms reflected across axes maintain their horizontal character

**Keep polar kaleidoscope** as secondary option:
- Produces mandala aesthetic for users who prefer radial patterns
- Works well with circular waveform mode when enabled

**Consider triangle-based (45-45-90)** for hybrid aesthetic:
- 8-fold symmetry provides kaleidoscope feel
- Tiles into square grid that fills rectangular screens
- Balances radial appearance with full-screen coverage

## Uniform Requirements

Current implementation (`shaders/kaleidoscope.fs:10-11`) uses:
- `segments` (int) - number of radial mirror segments
- `rotation` (float) - pattern rotation in radians

### Per-Technique Uniforms

| Technique | Parameters | Notes |
|-----------|------------|-------|
| Polar (current) | `segments` (int), `rotation` | Segments divides 2π into discrete wedges |
| Cartesian mirror | `tilesX`, `tilesY` (int or float), `rotation` | Per-axis tile count |
| Wallpaper p4m | `scale` (float), `rotation` | Continuous scale factor |
| Triangle 45-45-90 | `scale` (float), `rotation` | Continuous scale factor |
| Hybrid tiled polar | `gridSize` (vec2), `segments`, `rotation` | Combines grid + radial |

Key difference: polar uses discrete `segments` (dividing 2π), rectangular techniques use continuous `scale` or per-axis `tiles`.

### Implementation Options

**Option A: Mode uniform**
Add `int mode` to switch algorithm within single shader. Reinterpret `segments` per mode.
- Pro: Single shader file, single set of uniform locations
- Con: Shader contains dead code paths, `segments` meaning varies by mode

**Option B: Separate shaders**
Create `kaleidoscope_polar.fs`, `kaleidoscope_rect.fs` with appropriate uniforms.
- Pro: Clean separation, each shader optimized for its technique
- Con: Multiple shader loads, duplicated uniform setup in C++ code

**Option C: Generalized uniforms**
Use `vec2 tiles` that polar interprets as `(segments, unused)` and rectangular as `(tilesX, tilesY)`.
- Pro: Unified interface, smooth parameter interpolation possible
- Con: Type mismatch (polar segments are discrete integers)

**Recommendation:** Option B (separate shaders). Matches existing pattern where each post-effect has dedicated shader (`blur_h.fs`, `blur_v.fs`, `chromatic.fs`, `feedback.fs`, `voronoi.fs`). Avoids runtime branching and keeps uniform semantics clear.

## Sources

- [Daniel Ilett - Crazy Kaleidoscopes](https://danielilett.com/2020-02-19-tut3-8-crazy-kaleidoscopes/) - Polar coordinate implementation
- [Leif Gehrmann - Digital Kaleidoscopes](https://leifgehrmann.com/kaleidoscopes/) - Tessellation mathematics
- [LYGIA Shader Library](https://lygia.xyz/space/kaleidoscope) - Production GLSL reference
- [Wolfram MathWorld - Wallpaper Groups](https://mathworld.wolfram.com/WallpaperGroups.html) - 17 symmetry groups
- [The Book of Shaders - Patterns](https://thebookofshaders.com/09/) - Tiling fundamentals
- [Shader Design Patterns 2024](https://sangillee.com/2024-05-25-shader-design-patterns/) - fract/floor patterns
- [Inigo Quilez - Texture Repetition](https://iquilezles.org/articles/texturerepetition/) - Anti-tiling techniques

## Open Questions

- **TBD:** Does wallpaper p4m interact well with existing feedback/zoom effects?
- **Needs investigation:** Performance impact of derivative-correct texture sampling on target hardware
- **TBD:** User preference for radial vs rectangular symmetry in audio visualization context
