# Folding Techniques

Survey of UV folding operations for fractal and symmetry transforms. These techniques reflect, tile, or invert coordinates to create self-similar or kaleidoscopic patterns from input textures.

## Existing Effects (for reference)

| Effect | Fold Type | Operation |
|--------|-----------|-----------|
| KIFS | Cartesian reflection | `abs(p)` + scale + translate |
| Kaleidoscope | Polar wedge | `mod(angle, 2π/segments)` |
| Lattice Fold | Grid tiling | `fract(p)` with hex/square cells |
| Poincare Disk | Hyperbolic | Möbius transforms in unit disk |

## New Techniques

### 1. Box Fold (Mandelbox-style)

Conditional reflection that "folds" coordinates back when they exceed a boundary. Unlike `abs()` which always reflects to positive, box fold only reflects when outside the box.

```glsl
// Box fold: reflect if outside [-limit, limit]
vec2 boxFold(vec2 p, float limit) {
    return clamp(p, -limit, limit) * 2.0 - p;
}
```

**How it works:**
- If `p.x` is within `[-limit, limit]`: `clamp(p.x) * 2 - p.x = p.x * 2 - p.x = p.x` (unchanged)
- If `p.x > limit`: `clamp(p.x) * 2 - p.x = limit * 2 - p.x` (reflected across `+limit`)
- If `p.x < -limit`: `clamp(p.x) * 2 - p.x = -limit * 2 - p.x` (reflected across `-limit`)

**Iterated box fold:**
```glsl
for (int i = 0; i < iterations; i++) {
    p = boxFold(p, foldLimit);
    p = p * scale - offset;
    p = rotate2d(p, twist);
}
```

**Visual character:** Creates hard-edged crystalline patterns. The fold boundary creates visible seams where reflections occur.

**Parameters:**
| Parameter | Range | Effect |
|-----------|-------|--------|
| foldLimit | 0.5 - 2.0 | Box size for folding |
| scale | 1.5 - 3.0 | Expansion per iteration |
| offset | vec2 | Translation after fold |
| iterations | 1 - 12 | Recursion depth |

---

### 2. Sphere Fold (Inversion)

Inverts points through a sphere — points inside get pushed out, points outside get pulled in. Creates bulbous, organic distortions.

```glsl
// Sphere fold: invert through sphere of radius R
vec2 sphereFold(vec2 p, float minRadius, float maxRadius) {
    float r2 = dot(p, p);
    if (r2 < minRadius * minRadius) {
        // Inside inner sphere: scale up strongly
        p *= (maxRadius * maxRadius) / (minRadius * minRadius);
    } else if (r2 < maxRadius * maxRadius) {
        // Between spheres: scale inversely with distance
        p *= (maxRadius * maxRadius) / r2;
    }
    // Outside: unchanged
    return p;
}
```

**Simplified version (single radius):**
```glsl
vec2 sphereFoldSimple(vec2 p, float radius) {
    float r2 = dot(p, p);
    float rad2 = radius * radius;
    if (r2 < rad2) {
        p *= rad2 / max(r2, 0.0001);  // Invert
    }
    return p;
}
```

**Visual character:** Soft, bulging distortions. Points near center explode outward, creating bubble-like structures.

**Parameters:**
| Parameter | Range | Effect |
|-----------|-------|--------|
| minRadius | 0.1 - 0.5 | Inner sphere (strong inversion zone) |
| maxRadius | 0.5 - 2.0 | Outer sphere (inversion boundary) |

---

### 3. Combined Box + Sphere Fold (Mandelbox 2D)

The Mandelbox formula combines box fold and sphere fold for rich fractal patterns:

```glsl
vec2 mandelboxFold(vec2 p, float boxLimit, float sphereMin, float sphereMax, float scale) {
    // Box fold first
    p = clamp(p, -boxLimit, boxLimit) * 2.0 - p;

    // Then sphere fold
    float r2 = dot(p, p);
    if (r2 < sphereMin * sphereMin) {
        p *= (sphereMax * sphereMax) / (sphereMin * sphereMin);
    } else if (r2 < sphereMax * sphereMax) {
        p *= (sphereMax * sphereMax) / r2;
    }

    // Scale
    p *= scale;

    return p;
}

// Full iteration
for (int i = 0; i < iterations; i++) {
    p = mandelboxFold(p, 1.0, 0.5, 1.0, scale);
    p -= offset;
}
```

**Visual character:** Combines hard box edges with soft sphere bulges. Creates the distinctive Mandelbox aesthetic — boxy but organic.

---

### 4. Triangle/Sierpinski Fold

Folds space into equilateral triangles, creating Sierpinski-like patterns:

```glsl
// Fold into equilateral triangle
vec2 triangleFold(vec2 p) {
    const float SQRT3 = 1.7320508;

    // Fold across Y axis
    p.x = abs(p.x);

    // Fold across line at 60° (y = sqrt(3) * x)
    if (p.y > SQRT3 * p.x) {
        // Reflect across the 60° line
        vec2 n = vec2(SQRT3, -1.0) * 0.5;  // Normal to 60° line
        p -= 2.0 * dot(p, n) * n;
    }

    return p;
}

// Sierpinski iteration
for (int i = 0; i < iterations; i++) {
    p = triangleFold(p);
    p = p * scale - offset;
}
```

**Visual character:** 6-fold or 3-fold symmetry with triangular tiling. Creates gasket-like fractal holes.

---

### 5. Arbitrary Plane Fold

Fold across any line defined by angle:

```glsl
// Fold across line at angle theta (measured from X axis)
vec2 planeFold(vec2 p, float theta) {
    // Normal to the fold line
    vec2 n = vec2(sin(theta), -cos(theta));

    // Reflect if on negative side of line
    float d = dot(p, n);
    if (d < 0.0) {
        p -= 2.0 * d * n;
    }
    return p;
}

// Multi-plane fold (creates polygon symmetry)
vec2 polygonFold(vec2 p, int sides) {
    float angle = 3.14159 / float(sides);
    for (int i = 0; i < sides / 2; i++) {
        p = planeFold(p, angle * float(i));
    }
    return p;
}
```

**Visual character:** Generalizes kaleidoscope to arbitrary angles. Can create any regular polygon symmetry.

---

### 6. Modular Domain Repetition

Infinite tiling via modulo — different from Lattice Fold's fract() because it preserves distance relationships:

```glsl
// Infinite tiling with cell ID
vec2 domainRepeat(vec2 p, float cellSize, out vec2 cellID) {
    cellID = floor(p / cellSize);
    return mod(p, cellSize) - cellSize * 0.5;  // Center in cell
}

// With mirroring for seamless edges
vec2 domainRepeatMirror(vec2 p, float cellSize) {
    vec2 cell = floor(p / cellSize);
    vec2 local = mod(p, cellSize) / cellSize;

    // Mirror alternate cells
    if (mod(cell.x, 2.0) >= 1.0) local.x = 1.0 - local.x;
    if (mod(cell.y, 2.0) >= 1.0) local.y = 1.0 - local.y;

    return local;
}
```

**Visual character:** Clean infinite tiling. Mirror variant prevents seams at cell boundaries.

---

## Compound Effects

### KIFS + Box Fold

Combines abs() folding with conditional box fold:

```glsl
for (int i = 0; i < iterations; i++) {
    p = abs(p);                           // Standard KIFS fold
    p = clamp(p, -limit, limit) * 2.0 - p; // Additional box fold
    p = p * scale - offset;
    p = rotate2d(p, twist);
}
```

### Nested Symmetries

Apply different fold types at different iteration depths:

```glsl
for (int i = 0; i < iterations; i++) {
    if (i < 3) {
        p = abs(p);  // Cartesian fold early
    } else {
        p = triangleFold(p);  // Triangle fold later
    }
    p = p * scale - offset;
}
```

---

## Classification Summary

| Technique | Category | Complexity | Visual Style |
|-----------|----------|------------|--------------|
| Box Fold | Symmetry | Low | Hard-edged, crystalline |
| Sphere Fold | Warp | Low | Bulbous, organic |
| Mandelbox 2D | Symmetry | Medium | Boxy + organic hybrid |
| Triangle Fold | Symmetry | Low | Sierpinski, gasket |
| Plane Fold | Symmetry | Low | Arbitrary polygon |
| Domain Repeat | Cellular | Low | Clean infinite tiling |

---

## Implementation Priority

**Recommended first:** Box Fold + Sphere Fold combined (Mandelbox 2D)
- Distinct visual from existing KIFS
- Two new fold primitives in one effect
- Rich parameter space for audio reactivity

**Second:** Triangle Fold
- Adds non-90° symmetry
- Complements existing square/hex Lattice Fold

---

## References

- [Mandelbox on Syntopia](http://blog.hvidtfeldts.net/index.php/2011/11/distance-estimated-3d-fractals-vi-the-mandelbox/) - Box/sphere fold origin
- [Shadertoy shader-fractals](https://github.com/pedrotrschneider/shader-fractals) - GLSL implementations
- [Folding the Koch Snowflake](http://roy.red/posts/folding-the-koch-snowflake/) - Fold-based fractal tutorial
- [Domain Repetition tutorial](https://fabricecastel.github.io/blog/2016-08-17/main.html) - Mod-based tiling
