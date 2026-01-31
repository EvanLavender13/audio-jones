# LEGO Bricks

Pixelates image into 3D-styled LEGO bricks with raised studs and edge shadows. Optionally merges adjacent similar-colored cells into larger rectangular bricks (1x2, 2x1, 2x2, etc.) rather than uniform 1x1 bricks.

## Classification

- **Category**: TRANSFORMS > Style
- **Pipeline Position**: After feedback, with other transforms (user-reorderable)

## References

- User-provided Shadertoy shader (simple stud/shadow approach) - base rendering technique
- [Greedy Voxel Meshing](https://gedge.ca/blog/2014-08-17-greedy-voxel-meshing/) - rectangle merging concept
- [Roblox Greedy Meshing Tutorial](https://devforum.roblox.com/t/consume-everything-how-greedy-meshing-works/452717) - algorithm walkthrough

## Algorithm

### Base Brick Rendering (from user's shader)

```glsl
// Grid quantization
vec2 middle = floor(fragCoord * scale + 0.5) / scale;
vec3 color = texture(input, middle / resolution).rgb;

// Stud highlight (circular raised bump)
float studDist = abs(distance(fragCoord, middle) * scale * 2.0 - 0.6);
float studHighlight = smoothstep(0.1, 0.05, studDist);
vec2 studDir = normalize(fragCoord - middle);
color *= studHighlight * dot(vec2(0.707), studDir) * 0.5 + 1.0;

// Edge shadow (darker near brick edges)
vec2 delta = abs(fragCoord - middle) * scale * 2.0;
float edgeDist = max(delta.x, delta.y);
color *= 0.8 + smoothstep(0.95, 0.8, edgeDist) * 0.2;
```

### Variable Brick Sizing (novel extension)

Traditional greedy meshing iterates globally with visited flags, incompatible with fragment shaders. Instead, use a **deterministic local heuristic** where each pixel independently computes which brick it belongs to.

**Core insight**: If all pixels use the same priority rules (larger bricks first, top-left anchor wins), they independently arrive at consistent brick assignments.

```glsl
// Brick sizes to try, largest first
const ivec2 BRICK_SIZES[] = ivec2[](
    ivec2(2, 2), ivec2(2, 1), ivec2(1, 2), ivec2(1, 1)
);

// For this pixel's grid cell, find which brick it belongs to
ivec2 findBrick(ivec2 gridPos, sampler2D tex, float threshold) {
    for (int i = 0; i < BRICK_SIZES.length(); i++) {
        ivec2 size = BRICK_SIZES[i];

        // Check all possible anchors where this cell could be part of this brick
        for (int ay = 0; ay < size.y; ay++) {
            for (int ax = 0; ax < size.x; ax++) {
                ivec2 anchor = gridPos - ivec2(ax, ay);

                // Validate: anchor must align to brick-size grid
                if (anchor.x % size.x != 0 || anchor.y % size.y != 0) continue;

                // Sample all cells in this potential brick
                vec3 baseColor = sampleCell(tex, anchor);
                bool valid = true;

                for (int cy = 0; cy < size.y && valid; cy++) {
                    for (int cx = 0; cx < size.x && valid; cx++) {
                        vec3 cellColor = sampleCell(tex, anchor + ivec2(cx, cy));
                        if (colorDistance(cellColor, baseColor) > threshold) {
                            valid = false;
                        }
                    }
                }

                if (valid) {
                    // This pixel belongs to brick at anchor with this size
                    return anchor; // Also need to return size and local position
                }
            }
        }
    }
    return gridPos; // Fallback: 1x1 brick
}
```

**Stud placement for multi-cell bricks**: Each cell in a 2x2 brick gets its own stud. The stud center is at the center of each grid cell, not the center of the entire brick.

### Color Distance

Use perceptual distance in LAB space or simple Euclidean RGB:

```glsl
float colorDistance(vec3 a, vec3 b) {
    vec3 diff = a - b;
    return dot(diff, diff); // Squared Euclidean, compare against threshold^2
}
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| brickScale | float | 0.01-0.2 | 0.04 | Brick size relative to screen (higher = fewer, larger bricks) |
| studHeight | float | 0.0-1.0 | 0.5 | Intensity of stud highlight/shadow |
| edgeShadow | float | 0.0-1.0 | 0.2 | Darkness of brick edge shadows |
| colorThreshold | float | 0.0-0.5 | 0.1 | Color similarity required to merge bricks (0 = exact match only) |
| maxBrickSize | int | 1-4 | 2 | Largest brick dimension to attempt (1 = all 1x1, 2 = up to 2x2) |
| lightAngle | float | 0-360 deg | 45 | Direction of simulated light for stud highlights |

## Modulation Candidates

- **brickScale**: Changes brick density, creating zoom-like effect
- **studHeight**: Flattens or emphasizes 3D appearance
- **colorThreshold**: Dynamically adjusts brick merging aggressiveness
- **lightAngle**: Rotating light creates shimmering stud highlights

## Notes

**Performance**: Each pixel samples up to `maxBrickSize^4` neighbor cells (e.g., 2x2 checks 16 potential positions × 4 cells = 64 samples worst case). Recommend keeping maxBrickSize ≤ 2 for real-time use. Larger bricks (2x4, 4x4) are expensive.

**Grid alignment constraint**: Larger bricks must align to their own size grid. A 2x2 brick anchors only at even grid coordinates. This ensures deterministic assignment without conflicts.

**Aspect ratio**: The user's shader uses square cells. For authentic LEGO proportions (studs are 8mm diameter, plates are 9.6mm wide), cells should be square. Brick height could differ from width for "plate" vs "brick" appearance, but adds complexity.

**No palette quantization**: The simpler shader (user's preference) preserves original colors. The palette-quantized version maps to official LEGO colors, which could be added as an optional mode.
