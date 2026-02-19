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

// Same hash as bit_crush.fs
float r(vec2 p, float t) {
    return cos(t * cos(p.x * p.y));
}

void main() {
    // Center-relative, resolution-scaled coordinates (same as generator)
    vec2 p = (fragTexCoord - center) * resolution * scale;

    // Iterative lattice walk (identical to generator)
    for (int i = 0; i < iterations; i++) {
        vec2 ceilCell = ceil(p / cellSize);
        vec2 floorCell = floor(p / cellSize);
        p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
    }

    // Map final position back to UV and sample input (clamp edges)
    vec2 sampleUV = clamp(p / (resolution * scale) + center, 0.0, 1.0);
    vec3 crushed = texture(texture0, sampleUV).rgb;
    vec3 original = texture(texture0, fragTexCoord).rgb;

    finalColor = vec4(mix(original, crushed, mixAmount), 1.0);
}
