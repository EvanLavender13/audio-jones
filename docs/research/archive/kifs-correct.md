# KIFS (Kaleidoscopic Iterated Function System) - Correct Implementation

KIFS creates fractal self-similarity through iterated Cartesian reflections (folds), scaling, and translation. Each iteration reflects the UV space across axis-aligned planes using `abs()`, then contracts toward an offset point.

## Classification

- **Category**: TRANSFORMS → Symmetry
- **Core Operation**: Iterated `abs()` folding + scale + translate + rotate in Cartesian space
- **Pipeline Position**: Same as other transform effects (user-ordered)

## References

- [KIFS Flythrough - Shadertoy](https://www.shadertoy.com/view/XsKXzc) - 3D raymarched KIFS with detailed comments
- [Knighty's Kaleidoscopic IFS](http://www.fractalforums.com/ifs-iterated-function-systems/kaleidoscopic-(escape-time-ifs)/) - Original technique description
- [Syntopia blog](http://blog.hvidtfeldts.net/) - Distance estimated 3D fractals series

## Current Implementation Problem

`shaders/kifs.fs` uses **polar kaleidoscope** folding, not KIFS:

```glsl
// CURRENT (WRONG) - polar angle subdivision
float a = atan(p.y, p.x);
float foldAngle = TWO_PI / float(segments);
a = abs(mod(a + foldAngle * 0.5, foldAngle) - foldAngle * 0.5);
p = vec2(cos(a), sin(a)) * length(p);
```

This divides the plane into radial wedges and mirrors into segments. It's a valid effect but it's **not KIFS**.

## True KIFS Algorithm

From the Shadertoy references, the core KIFS iteration is:

```glsl
// TRUE KIFS - Cartesian abs() folding
for (int i = 0; i < iterations; i++) {
    p = abs(p);                      // Fold: reflect across both axes
    // Optional octant fold:
    // if (p.x < p.y) p.xy = p.yx;   // Fold into one octant

    p = p * scale - offset;          // IFS contraction toward offset
    p = rotate2d(p, twistAngle);     // Per-iteration rotation
}
```

### Key Differences

| Aspect | Polar Kaleidoscope | True KIFS |
|--------|-------------------|-----------|
| Fold operation | `mod(angle, 2π/segments)` | `abs(p)` |
| Coordinate space | Polar | Cartesian |
| "Segments" parameter | Controls radial wedge count | **Does not exist** |
| Result | Radial mirror symmetry | Fractal self-similarity |
| Math basis | Angle subdivision | Reflection across planes |

### The `abs()` Fold Explained

`abs(p)` reflects all points into the positive quadrant:
- Points at (-1, 2) → (1, 2)
- Points at (3, -4) → (3, 4)
- Points at (-5, -6) → (5, 6)

This is equivalent to mirroring across both the X and Y axes simultaneously. When combined with scale and translate, it creates the characteristic KIFS fractal tiling.

### Optional Octant Fold

For more complex symmetry, fold into one octant after `abs()`:

```glsl
p = abs(p);
if (p.x < p.y) {
    float tmp = p.x;
    p.x = p.y;
    p.y = tmp;
}
// Or branchless: p.xy += step(p.x, p.y) * (p.yx - p.xy);
```

## Correct Parameters

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| iterations | int | 1-12 | Fold/scale/translate cycles |
| scale | float | 1.5-4.0 | Expansion factor per iteration |
| offsetX | float | 0.0-2.0 | X translation after fold |
| offsetY | float | 0.0-2.0 | Y translation after fold |
| rotationSpeed | float | rad/frame | Animation rotation rate |
| twistAngle | float | radians | Per-iteration rotation offset |
| octantFold | bool | - | Enable octant folding (optional) |

**Note**: `segments` parameter should be **removed**. It has no meaning in KIFS.

## Correct Shader Implementation

```glsl
#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;

uniform int iterations;      // 1-12
uniform float scale;         // 1.5-4.0
uniform vec2 kifsOffset;     // Translation after fold
uniform float rotation;      // Global rotation (accumulated)
uniform float twistAngle;    // Per-iteration rotation
uniform bool octantFold;     // Optional octant folding

out vec4 finalColor;

vec2 rotate2d(vec2 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

vec2 mirror(vec2 x) {
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

void main() {
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Initial rotation
    p = rotate2d(p, rotation);

    // KIFS iteration
    for (int i = 0; i < iterations; i++) {
        // Fold: reflect into positive quadrant
        p = abs(p);

        // Optional: fold into one octant
        if (octantFold && p.x < p.y) {
            p.xy = p.yx;
        }

        // IFS contraction: scale and translate toward offset
        p = p * scale - kifsOffset;

        // Per-iteration rotation
        p = rotate2d(p, twistAngle + float(i) * 0.1);
    }

    // Sample texture at transformed position
    vec2 newUV = mirror(p * 0.5 + 0.5);
    finalColor = texture(texture0, newUV);
}
```

## Migration Path

1. Remove `segments` from `KifsConfig`
2. Add `octantFold` bool (optional, default false)
3. Update `kifs.fs` with correct algorithm
4. Update UI to remove segments slider
5. Update param_registry.cpp
6. Update preset serialization (remove segments, add octantFold)
7. Migrate existing presets (drop segments field)

## Audio Mapping Ideas

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| scale | Bass energy | Pulse fractal density with kick |
| offsetX/Y | Mid frequencies | Shift fractal center rhythmically |
| twistAngle | High frequencies | Sparkle rotation on hats/cymbals |
| iterations | Sustained bass | More detail during drops |

## Notes

- The current "KIFS" effect produces visually interesting results but is mislabeled
- True KIFS creates blocky, crystalline fractal patterns vs polar kaleidoscope's smooth radial symmetry
- Both effects are valid - consider keeping polar version as separate "Polar Kaleidoscope" or "Radial IFS" effect
- Scale values < 1.0 cause the pattern to collapse to center; values > 2.0 create more fractal detail
