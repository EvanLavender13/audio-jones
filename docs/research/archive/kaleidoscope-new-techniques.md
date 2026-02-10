# Kaleidoscope New Techniques

Research for two new techniques identified during kaleidoscope reorganization.

## Classification

- **Category**: TRANSFORMS → Symmetry (smooth kaleidoscope), TRANSFORMS → Cellular (lattice fold)
- **Core Operation**: UV domain folding into fundamental regions
- **Pipeline Position**: Transform stage (user-ordered)

## References

- [Daniel Ilett - Crazy Kaleidoscopes](https://danielilett.com/2020-02-19-tut3-8-crazy-kaleidoscopes/) - Polar kaleidoscope tutorial
- [Mercury hg_sdf](https://mercury.sexy/hg_sdf/) - `modMirror1` and smooth folding functions
- [Geometry in Color - Kaleidoscopes with Periodic Images](https://geometricolor.wordpress.com/2014/02/01/geometry-of-kaleidoscopes-with-periodic-images/) - Wallpaper group symmetries
- User-provided Shadertoy references (inline code samples below)

---

## Technique 1: Smooth Polar Kaleidoscope

### Problem

Standard polar kaleidoscope creates hard seams at wedge boundaries:

```glsl
// Hard fold - discontinuity at segAngle boundary
float a = mod(angle, segAngle);
a = min(a, segAngle - a);
```

### Solution: Polynomial Smooth Absolute

Replace hard `min(a, segAngle - a)` with smooth polynomial blend using `pabs`:

```glsl
// From Mercury hg_sdf / Mårten Rånge
float pmin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

float pabs(float a, float k) {
    return -pmin(a, -a, k);
}

// Smooth kaleidoscope fold
float smoothKaleidoscope(inout vec2 p, float smoothing, float segments) {
    vec2 polar = vec2(length(p), atan(p.y, p.x));

    // Mirror into segment
    float segAngle = TAU / segments;
    float halfSeg = PI / segments;
    float c = floor((polar.y + halfSeg) / segAngle);
    polar.y = mod(polar.y + halfSeg, segAngle) - halfSeg;
    polar.y *= mod(c, 2.0) * 2.0 - 1.0;  // flip alternating segments

    // Smooth fold at boundary (smoothing=0 gives hard edge)
    float sa = halfSeg - pabs(halfSeg - abs(polar.y), smoothing);
    polar.y = sign(polar.y) * sa;

    // Back to Cartesian
    p = vec2(cos(polar.y), sin(polar.y)) * polar.x;
    return c;
}
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| smoothing | float | 0.0 - 0.5 | Blend width at wedge seams. 0 = hard edge (current behavior) |

### Visual Effect

- `smoothing = 0`: Identical to current polar kaleidoscope
- `smoothing = 0.1`: Slight softening at wedge boundaries
- `smoothing = 0.3+`: Rounded, organic-looking symmetry

---

## Technique 2: Lattice Fold

Fold UV coordinates into repeating cell grids with internal symmetry. Supports triangular, square, and hexagonal tilings.

### Core Concept

1. Transform screen coords to lattice coords (skewed for tri/hex)
2. Identify which cell the point falls in
3. Get position within cell
4. Apply internal cell symmetry (3/4/6-fold)
5. Map back to UV for texture sampling

### Triangular Lattice

From user reference - uses skewed coordinate matrix:

```glsl
// Screen to triangular coordinates
const mat2x3 SCREEN_TO_TRI = mat2x3(
    1.0,  0.5,       -0.5,
    0.0,  sqrt(0.75), sqrt(0.75)
) / scale;

vec3 tri = SCREEN_TO_TRI * fragCoord;  // tri.z = tri.y - tri.x
vec3 cell = ceil(tri);

// Hex coords from triangular (every 3 triangles = 1 hex)
vec2 hex = floor((cell.y + cell.xz) / 3.0);
```

### Hexagonal Lattice

Current implementation in `kaleidoscope.fs`:

```glsl
const vec2 HEX_S = vec2(1.0, 1.7320508);  // 1, sqrt(3)

vec4 getHex(vec2 p) {
    vec4 hC = floor(vec4(p, p - vec2(0.5, 1.0)) / HEX_S.xyxy) + 0.5;
    vec4 h = vec4(p - hC.xy * HEX_S, p - (hC.zw + 0.5) * HEX_S);
    return dot(h.xy, h.xy) < dot(h.zw, h.zw)
        ? vec4(h.xy, hC.xy)
        : vec4(h.zw, hC.zw + 0.5);
}
```

### Alternative Hex Fold (from user reference)

Iterative folding approach:

```glsl
vec2 fold(vec2 p, float scale) {
    vec4 m = vec4(2.0, -1.0, 0.0, sqrt(3.0));
    p.y += m.w / 3.0;  // center at vertex
    vec2 t = mat2(m) * p;  // triangular coordinates
    return p - 0.5 * mat2(m.xzyw) * round((floor(t) + ceil(t.x + t.y)) / 3.0);
}

// Iterative application with rotation/scale
for (int i = 0; i < iterations; i++) {
    p = fold(rotationMatrix * p * scale);
}
```

### Square Lattice

Simplest case - axis-aligned:

```glsl
vec2 squareFold(vec2 p, float scale) {
    vec2 cell = floor(p * scale);
    vec2 local = fract(p * scale) - 0.5;

    // 4-fold symmetry within cell
    local = abs(local);

    return local / scale;
}
```

### Unified Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| cellType | int | 3, 4, 6 | Triangle, Square, Hexagon |
| cellScale | float | 1.0 - 20.0 | Cell density (higher = more cells) |
| internalSymmetry | bool | true/false | Apply N-fold symmetry within each cell |
| rotation | float | 0 - 2π | Rotate entire lattice |

### Visual Differences

- **Triangle (3)**: Densest packing, 60° angles, complex interference patterns
- **Square (4)**: Familiar grid, 90° angles, clean geometric look
- **Hexagon (6)**: Honeycomb pattern, 120° angles, organic feel

---

## Audio Mapping Ideas

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| smoothing | Bass energy | Harder edges on beats, soft between |
| cellScale | Mid-range | Cells pulse larger/smaller |
| rotation | Low frequency | Slow rotation drift |

---

## Notes

- Smooth kaleidoscope is a parameter addition to existing Polar technique
- Lattice Fold replaces current Hex Fold with generalized cell types
- Both techniques sample input texture (UV transforms, not procedural)
- Square lattice `fract` differs from fractal zoom `fract` - here we fold back, not tile infinitely
