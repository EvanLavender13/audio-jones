# Poincaré Disk Hyperbolic Tiling

Warps texture through hyperbolic space using the Poincaré disk model. Combines Möbius translation (smooth camera movement) with hyperbolic folding (Escher-like repeating tiles). The effect confines all content within a circular boundary where the edge represents "infinity."

## Classification

- **Category**: TRANSFORMS → Symmetry
- **Core Operation**: Apply Möbius translation, fold into fundamental domain, sample texture at transformed UVs
- **Pipeline Position**: Reorderable (executes in user-defined order within Transforms stage)

## References

- Shadertoy "Tiling the Poincaré Disc" (user-provided) - Folding algorithm with (P,Q,R) triangle parameters
- Shadertoy "Poincaré Disk Isometries" (user-provided) - Möbius structures and disk-to-UHP mapping
- Shadertoy "Poincaré Tiling" by Skye Adaire (user-provided) - Clean Möbius matrix implementation
- Shadertoy texture sampling example (user-provided) - Confirms `texture(input, transformedUV)` approach
- "Compass and Straightedge in the Poincaré Disk" by Chaim Goodman-Strauss - Mathematical foundations

## Algorithm

### 1. Möbius Translation (Hyperbolic Camera Movement)

Translates the viewpoint through hyperbolic space. Points near the disk edge compress; center expands.

```glsl
// Hyperbolic translation: move origin toward point v
vec2 mobiusTranslate(vec2 v, vec2 z) {
    float vv = dot(v, v);
    float zv = dot(z, v);
    float zz = dot(z, z);
    return ((1.0 + 2.0 * zv + zz) * v + (1.0 - vv) * z) /
           (1.0 + 2.0 * zv + vv * zz);
}
```

Alternative matrix form (more composable):
```glsl
// SU(1,1) matrix for translation toward point a
void mobiusMatrix(vec2 a, out vec2 alpha, out vec2 beta) {
    float s = inversesqrt(max(1.0 - dot(a, a), 1e-8));
    alpha = vec2(s, 0.0);
    beta = -a * s;
}

// Apply Möbius matrix to point z
vec2 applyMobius(vec2 alpha, vec2 beta, vec2 z) {
    vec2 num = cmul(alpha, z) + beta;
    vec2 den = cmul(conj(beta), z) + conj(alpha);
    return cdiv(num, den);
}
```

### 2. Hyperbolic Folding (Tiling)

Reflects point into fundamental triangular domain. The triangle has angles π/P, π/Q, π/R at vertices. Valid tilings require `1/P + 1/Q + 1/R < 1`.

```glsl
// Fundamental domain defined by three mirrors
// A, B: straight lines through origin (vec2 normals)
// C: circular arc orthogonal to unit circle (vec3: center.xy, radius²)

vec2 hyperbolicFold(vec2 z, vec2 A, vec2 B, vec3 C, out int foldCount) {
    foldCount = 0;
    for (int i = 0; i < 50; i++) {
        int folds = 0;

        // Reflect across line A (normal A)
        float tA = dot(z, A);
        if (tA < 0.0) {
            z -= 2.0 * tA * A;
            folds++;
        }

        // Reflect across line B (normal B)
        float tB = dot(z, B);
        if (tB < 0.0) {
            z -= 2.0 * tB * B;
            folds++;
        }

        // Invert through circle C (center C.xy, radius² C.z)
        vec2 zc = z - C.xy;
        float d2 = dot(zc, zc);
        if (d2 < C.z) {
            z = C.xy + zc * C.z / d2;
            folds++;
        }

        if (folds == 0) break;
        foldCount += folds;
    }
    return z;
}
```

### 3. Constructing the Fundamental Triangle

Given angles π/P, π/Q, π/R, compute mirrors A, B and circle C:

```glsl
void solveFundamentalDomain(int P, int Q, int R,
                            out vec2 A, out vec2 B, out vec3 C) {
    // First two mirrors at angle π/P
    A = vec2(1.0, 0.0);
    B = vec2(-cos(PI / float(P)), sin(PI / float(P)));

    // Circle C orthogonal to unit circle, making angles π/Q and π/R
    // with mirrors A and B respectively
    mat2 m = inverse(mat2(A, B));
    vec2 cDir = vec2(cos(PI / float(R)), cos(PI / float(Q))) * m;

    float S2 = 1.0 / (dot(cDir, cDir) - 1.0);
    float S = sqrt(S2);
    C = vec3(cDir * S, S2);
}
```

### 4. Complex Number Helpers

```glsl
vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

vec2 cdiv(vec2 a, vec2 b) {
    return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / dot(b, b);
}

vec2 conj(vec2 z) { return vec2(z.x, -z.y); }
```

### 5. Full UV Transform

```glsl
vec2 poincareUV(vec2 uv, vec2 translation, int P, int Q, int R,
                float diskScale, out int foldCount) {
    // Map to disk coordinates
    vec2 z = (uv - 0.5) * 2.0 * diskScale;

    // Clip to unit disk
    if (dot(z, z) > 1.0) {
        foldCount = -1;
        return vec2(-1.0); // Outside disk
    }

    // Apply translation
    z = mobiusTranslate(translation, z);

    // Construct fundamental domain
    vec2 A, B; vec3 C;
    solveFundamentalDomain(P, Q, R, A, B, C);

    // Fold into fundamental domain
    z = hyperbolicFold(z, A, B, C, foldCount);

    // Map back to texture coords
    return z * 0.5 + 0.5;
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `tileP` | int | 2-12 | Angle at origin vertex (π/P) |
| `tileQ` | int | 2-12 (or 0=∞) | Angle at second vertex |
| `tileR` | int | 2-12 (or 0=∞) | Angle at third vertex |
| `translationX` | float | -0.9 to 0.9 | Möbius translation X |
| `translationY` | float | -0.9 to 0.9 | Möbius translation Y |
| `translationSpeed` | float | 0.0-2.0 | Auto-animation speed (radians/sec) |
| `diskScale` | float | 0.5-2.0 | Disk size relative to screen |
| `rotationSpeed` | float | 0.0-1.0 | Euclidean rotation (also an isometry) |

Classic tilings:
- (2,3,7) - Order-7 triangular
- (3,3,4) - Alternated octagonal
- (4,4,4) - Square tiling
- (5,4,2) - Order-4 pentagonal
- (7,3,2) - Heptagonal (from references)

## Audio Mapping Ideas

| Audio Feature | Parameter | Effect |
|---------------|-----------|--------|
| Bass energy | `translationSpeed` | Faster hyperbolic panning on bass hits |
| Mid energy | `rotationSpeed` | Spin the disk |
| High energy | Tile color modulation | Shimmer on highs (use foldCount for color) |
| Beat detection | Translation direction | Jump to new random direction on beat |
| Sustained bass | `diskScale` | Breathing/zoom effect |

## Notes

- **Fold count parity**: `foldCount % 2` distinguishes adjacent tiles (checkerboard coloring)
- **Edge behavior**: Content compresses infinitely toward disk edge; may want to fade alpha near boundary
- **Performance**: 50 fold iterations handles most cases; reduce for performance if needed
- **Degenerate cases**: P, Q, R must satisfy `1/P + 1/Q + 1/R < 1` for hyperbolic geometry
- **Ideal vertices**: Setting Q or R to 0 creates "ideal" triangles with vertices at infinity (on disk boundary)
- **Stacking**: Can run before or after other UV effects; translation is composable via matrix multiplication
