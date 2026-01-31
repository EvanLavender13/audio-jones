#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float brickScale;      // Grid cell size as fraction of screen width
uniform float studHeight;      // Stud highlight intensity
uniform float edgeShadow;      // Edge shadow darkness
uniform float colorThreshold;  // Squared color distance for merging
uniform int maxBrickSize;      // 1 = all 1x1, 2 = up to 2x2
uniform float lightAngle;      // Light direction in radians

// Brick sizes to try, largest first (for maxBrickSize=2)
const ivec2 BRICK_SIZES_2[4] = ivec2[](
    ivec2(2, 2), ivec2(2, 1), ivec2(1, 2), ivec2(1, 1)
);

// Sample color at grid cell center
vec3 sampleCell(ivec2 cell, float cellPixels) {
    vec2 cellCenter = (vec2(cell) + 0.5) * cellPixels / resolution;
    return texture(texture0, cellCenter).rgb;
}

// Squared Euclidean color distance
float colorDist(vec3 a, vec3 b) {
    vec3 d = a - b;
    return dot(d, d);
}

// Find brick anchor and size for this grid cell
// Returns: x,y = anchor cell coords, z = brick width, w = brick height
ivec4 findBrick(ivec2 gridPos, float cellPixels, float threshold) {
    int numSizes = (maxBrickSize >= 2) ? 4 : 1;

    for (int i = 0; i < numSizes; i++) {
        ivec2 size = (maxBrickSize >= 2) ? BRICK_SIZES_2[i] : ivec2(1, 1);

        // Try all anchor positions where this cell could be part of this brick
        for (int ay = 0; ay < size.y; ay++) {
            for (int ax = 0; ax < size.x; ax++) {
                ivec2 anchor = gridPos - ivec2(ax, ay);

                // Anchor must align to brick-size grid
                if (anchor.x % size.x != 0 || anchor.y % size.y != 0) continue;
                if (anchor.x < 0 || anchor.y < 0) continue;

                // Check all cells in this potential brick have similar color
                vec3 baseColor = sampleCell(anchor, cellPixels);
                bool valid = true;

                for (int cy = 0; cy < size.y && valid; cy++) {
                    for (int cx = 0; cx < size.x && valid; cx++) {
                        vec3 cellColor = sampleCell(anchor + ivec2(cx, cy), cellPixels);
                        if (colorDist(cellColor, baseColor) > threshold) {
                            valid = false;
                        }
                    }
                }

                if (valid) {
                    return ivec4(anchor, size);
                }
            }
        }
    }

    // Fallback: 1x1 brick at current cell
    return ivec4(gridPos, 1, 1);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    float cellPixels = brickScale * resolution.x;

    // Which grid cell is this pixel in?
    ivec2 gridPos = ivec2(floor(fc / cellPixels));

    // Find which brick this cell belongs to
    ivec4 brick = findBrick(gridPos, cellPixels, colorThreshold);
    ivec2 anchor = brick.xy;
    ivec2 brickSize = brick.zw;

    // Position within the brick (0-brickSize range)
    ivec2 localCell = gridPos - anchor;

    // Brick bounds in pixels
    vec2 brickStart = vec2(anchor) * cellPixels;
    vec2 brickEnd = vec2(anchor + brickSize) * cellPixels;

    // Sample color at brick anchor cell
    vec3 color = sampleCell(anchor, cellPixels);

    // Calculate stud for this grid cell (each cell in a multi-cell brick gets its own stud)
    vec2 cellStart = vec2(gridPos) * cellPixels;
    vec2 cellCenter = cellStart + cellPixels * 0.5;

    // Stud highlight (circular raised bump)
    float studDist = abs(distance(fc, cellCenter) / cellPixels * 2.0 - 0.6);
    float studHighlight = smoothstep(0.1, 0.05, studDist) * studHeight;
    vec2 studDir = normalize(fc - cellCenter);
    vec2 lightDir = vec2(cos(lightAngle), sin(lightAngle));
    color *= studHighlight * dot(lightDir, studDir) * 0.5 + 1.0;

    // Edge shadow (darker near brick edges, not cell edges)
    vec2 brickCenter = (brickStart + brickEnd) * 0.5;
    vec2 brickHalfSize = (brickEnd - brickStart) * 0.5;
    vec2 delta = abs(fc - brickCenter) / brickHalfSize;
    float edgeDist = max(delta.x, delta.y);
    color *= (1.0 - edgeShadow) + smoothstep(0.95, 0.8, edgeDist) * edgeShadow;

    finalColor = vec4(color, 1.0);
}
