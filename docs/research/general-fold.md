# General Fold

A composable UV folding system built from stackable primitives. Users construct custom fold behaviors by combining, ordering, and parameterizing atomic fold operations. Replaces KIFS, Mandelbox, and Triangle Fold with a single configurable effect.

## Classification

- **Category**: TRANSFORMS → Symmetry
- **Core Operation**: Iterative UV folding via user-defined primitive stack
- **Pipeline Position**: Transform stage (user-ordered with other transforms)

## Design Philosophy

**Build, don't select.** Instead of choosing from preset fold types, users stack fold primitives to create custom behaviors. Existing effects become presets, not hardcoded modes.

**Orthogonal to Warp and Depth.** Fold handles symmetry/reflection. Warp handles displacement. Depth handles layering. Chain them freely.

## Fold Primitives

Atomic operations that can be stacked in any order. Each primitive has optional parameters.

### Cartesian (abs)
Reflects coordinates into positive quadrant.

```glsl
// Axis: Both
p = abs(p);

// Axis: X only
p.x = abs(p.x);

// Axis: Y only
p.y = abs(p.y);
```

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `axis` | enum | Both / X / Y | Both |

---

### Octant Swap
Folds into single octant by swapping coordinates.

```glsl
if (p.x < p.y) p.xy = p.yx;
```

No parameters.

---

### Box Fold
Conditional reflection at boundaries. Creates the "box" in Mandelbox.

```glsl
p = clamp(p, -limit, limit) * 2.0 - p;
```

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `limit` | float | 0.5-2.0 | 1.0 |

---

### Sphere Fold
Inversion through nested spheres. Creates the "ball" in Mandelbox.

```glsl
float r2 = dot(p, p);
if (r2 < minR * minR) {
    p *= (maxR * maxR) / (minR * minR);
} else if (r2 < maxR * maxR) {
    p *= (maxR * maxR) / r2;
}
```

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `minRadius` | float | 0.1-0.5 | 0.25 |
| `maxRadius` | float | 0.5-2.0 | 1.0 |

---

### Axis Reflect
Reflects across a line through origin at specified angle.

```glsl
vec2 n = vec2(cos(angle), sin(angle));
float d = dot(p, n);
if (d < 0.0) p -= 2.0 * d * n;
```

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `angle` | float | 0 - π | 0.0 |
| `conditional` | bool | - | true |

When `conditional=false`, always reflects (no `if` check).

---

### Modulo Fold
Tiles space and folds into cell. Creates repeating patterns.

```glsl
p = mod(p + size * 0.5, size) - size * 0.5;
p = abs(p);  // fold within cell
```

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `sizeX` | float | 0.1-2.0 | 1.0 |
| `sizeY` | float | 0.1-2.0 | 1.0 |

---

## Pre-Fold Operations

Applied once before the iteration loop begins.

### Polar Fold
Radial wedge symmetry. Folds angle into segment, mirrors alternating segments.

```glsl
float angle = atan(p.y, p.x);
float segmentAngle = TWO_PI / float(segments);
float halfSeg = PI / float(segments);
float c = floor((angle + halfSeg) / segmentAngle);
angle = mod(angle + halfSeg, segmentAngle) - halfSeg;
angle *= mod(c, 2.0) * 2.0 - 1.0;
p = vec2(cos(angle), sin(angle)) * length(p);
```

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `segments` | int | 2-12 | 6 |

---

## Per-Iteration Transform

Applied after the fold stack, before next iteration.

```glsl
p = p * scale - offset;
p = rotate2d(p, twist);
```

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `scale` | float | 1.0-3.0 | 2.0 |
| `offsetX` | float | -2.0 to 2.0 | 1.0 |
| `offsetY` | float | -2.0 to 2.0 | 1.0 |
| `twist` | float | -π to π | 0.0 |

---

## Global Parameters

| Param | Type | Range | Default |
|-------|------|-------|---------|
| `iterations` | int | 1-8 | 4 |
| `rotation` | float | -π to π | 0.0 |
| `boundsMode` | enum | Mirror / Clamp / Wrap | Mirror |

---

## Algorithm

```glsl
void main() {
    vec2 p = (fragTexCoord - 0.5) * 2.0;

    // Global rotation
    p = rotate2d(p, rotation);

    // Pre-fold
    if (polarFoldEnabled) {
        p = polarFold(p, polarSegments);
    }

    // Iteration loop
    for (int i = 0; i < iterations; i++) {
        // Execute fold stack in order
        for (int f = 0; f < foldStackSize; f++) {
            p = applyFold(p, foldStack[f]);
        }

        // Per-iteration transform
        p = p * scale - offset;
        p = rotate2d(p, twist);
    }

    // Bounds handling
    vec2 uv = handleBounds(p * 0.5 + 0.5, boundsMode);

    finalColor = texture(texture0, uv);
}
```

---

## Preset Recipes

### KIFS
```
Fold Stack: [Cartesian(Both)]
Pre-fold: Polar(6) [optional]
Iterations: 4, Scale: 2.0, Offset: (1.0, 1.0), Twist: 0.1
```

### KIFS + Octant
```
Fold Stack: [Cartesian(Both), Octant]
Iterations: 4, Scale: 2.0, Offset: (1.0, 1.0), Twist: 0.1
```

### Mandelbox
```
Fold Stack: [Box(limit=1.0), Sphere(min=0.25, max=1.0)]
Pre-fold: Polar(6) [optional]
Iterations: 4, Scale: 2.0, Offset: (1.0, 1.0), Twist: 0.0
```

### Triangle (Sierpinski)
```
Fold Stack: [Cartesian(X), Axis(angle=60°)]
Iterations: 4, Scale: 2.0, Offset: (1.0, 0.5), Twist: 0.0
```

### Hexagonal
```
Fold Stack: [Cartesian(X), Axis(60°), Axis(120°)]
Iterations: 3, Scale: 2.0, Offset: (0.5, 0.866)
```

### Custom: Box + Axis
```
Fold Stack: [Box(limit=0.8), Axis(45°)]
Iterations: 5, Scale: 1.8, Offset: (0.5, 0.5), Twist: 0.2
```

---

## Modulation Candidates

- **scale** — Bass for breathing/pulsing fractal depth
- **twist** — Mid for rotational motion
- **offsetX/Y** — Spectral centroid for pattern drift
- **iterations** — Beat for complexity spikes (use sparingly)
- **Sphere.minRadius** — Creates "opening/closing" organic motion
- **Box.limit** — Subtle pattern morphing

---

## Implementation Notes

### Fold Stack Representation

Option A: Fixed-size array with enable flags
```cpp
struct FoldOp {
    FoldType type;      // enum
    bool enabled;
    float params[4];    // type-specific
};
FoldOp foldStack[MAX_FOLDS];  // MAX_FOLDS = 6
```

Option B: Dynamic vector (more flexible, harder to serialize)
```cpp
std::vector<FoldOp> foldStack;
```

Recommend Option A for simplicity. 6 slots covers all reasonable use cases.

### Shader Compilation

With a fixed stack size, use a switch inside the fold loop:
```glsl
for (int f = 0; f < MAX_FOLDS; f++) {
    if (!foldEnabled[f]) continue;
    switch (foldType[f]) {
        case FOLD_CARTESIAN: p = cartesianFold(p, foldParams[f]); break;
        case FOLD_BOX:       p = boxFold(p, foldParams[f]); break;
        // ...
    }
}
```

### UI Design

```
── Fold Stack ─────────────────────────────
   [+ Add Fold]

   ┌─ 1. Cartesian ──────────────── [×] ─┐
   │  Axis: (•) Both  ( ) X  ( ) Y       │
   └────────────────────────────── ↑↓ ───┘

   ┌─ 2. Sphere ─────────────────── [×] ─┐
   │  Min Radius ────○─────────          │
   │  Max Radius ──────────○───          │
   └────────────────────────────── ↑↓ ───┘

── Pre-Fold ───────────────────────────────
   [ ] Polar Fold     Segments: [6]

── Iteration ──────────────────────────────
   Iterations: [4]
   Scale ────────────○─────────
   Offset X ─────────○─────────
   Offset Y ─────────○─────────
   Twist ────────────○─────────

── Global ─────────────────────────────────
   Rotation ─────────○─────────
   Bounds: [Mirror ▾]

── Presets ────────────────────────────────
   [KIFS] [Mandelbox] [Triangle] [Save...]
```

Drag handles (↑↓) for reordering. [×] for removal. Collapsed by default, expand on click.

### Angular Parameters

- `rotation`, `twist`: Follow `*Angle` convention (radians, ±ROTATION_OFFSET_MAX)
- `Axis.angle`: Same convention
- Display as degrees in UI via `ModulatableSliderAngleDeg`

---

## Effects to Deprecate

Once General Fold ships:
- **KIFS** → Preset
- **Mandelbox** → Preset
- **Triangle Fold** → Preset

Keep separate:
- **Kaleidoscope** — Single-pass polar, different paradigm (smooth edges, radial twist, no iteration)
- **Lattice Fold** — Cell-based tiling, not iterative folding
- **Poincare Disk** — Hyperbolic geometry, fundamentally different math

---

## Open Questions

1. **Max stack size?** 6 feels reasonable. More than that is probably over-engineering.

2. **Per-primitive modulation?** Should individual fold params (like Box.limit) be modulatable, or just global params? Recommend: yes, all floats modulatable.

3. **Intensity/blend per primitive?** Mandelbox has `boxIntensity` and `sphereIntensity` to blend between folded and unfolded. Add this to all primitives, or keep it simple?

4. **Primitive ordering UI:** Drag-and-drop reordering, or up/down buttons? Drag is nicer but harder to implement.
