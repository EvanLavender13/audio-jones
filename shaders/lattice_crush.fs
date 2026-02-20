#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 center;
uniform float scale;
uniform float cellSize;
uniform int iterations;
uniform float time;
uniform float mixAmount;
uniform int walkMode;

// Same hash as bit_crush.fs
float r(vec2 p, float t) {
    return cos(t * cos(p.x * p.y));
}

// Asymmetric hash for mode 5
float rAsym(vec2 p, float t) {
    return cos(t * cos(dot(p, vec2(0.7, 1.3))));
}

void main() {
    // Center-relative, resolution-scaled coordinates (same as generator)
    vec2 p = (fragTexCoord - center) * resolution * scale;

    // Iterative lattice walk with walk-mode variants
    for (int i = 0; i < iterations; i++) {
        if (walkMode == 1) {
            // Rotating direction vector
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            vec2 dir = vec2(-cos(float(i)), sin(float(i)));
            p = ceil(p + r(ceilCell, time) + r(floorCell, time) * dir);
        } else if (walkMode == 2) {
            // Offset neighborhood query
            vec2 cellA = floor(p / cellSize) + vec2(-1.0, 0.0);
            vec2 cellB = floor(p / cellSize) + vec2(0.0, 1.0);
            p = ceil(p + r(cellA, time) + r(cellB, time) * vec2(-1.0, 1.0));
        } else if (walkMode == 3) {
            // Alternating snap function
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            vec2 stepped = p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0);
            int mode = i % 3;
            if (mode == 0) p = ceil(stepped);
            else if (mode == 1) p = floor(stepped);
            else p = round(stepped);
        } else if (walkMode == 4) {
            // Cross-coupled axes
            vec2 ceilCell = ceil(p / cellSize);
            float rx = r(ceilCell, time);
            float ry = r(ceilCell.yx, time);
            p = ceil(vec2(p.x + rx + ry * 0.5, p.y + ry - rx * 0.5));
        } else if (walkMode == 5) {
            // Asymmetric hash
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            p = ceil(p + rAsym(ceilCell, time) + rAsym(floorCell, time) * vec2(-1.0, 1.0));
        } else {
            // Mode 0 — Original
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
        }
    }

    // Map final position back to UV and sample input (clamp edges)
    vec2 sampleUV = clamp(p / (resolution * scale) + center, 0.0, 1.0);
    vec3 crushed = texture(texture0, sampleUV).rgb;
    vec3 original = texture(texture0, fragTexCoord).rgb;

    finalColor = vec4(mix(original, crushed, mixAmount), 1.0);
}
