#version 330

// Iterative Mirror: Repeated rotation + mirror operations for fractal symmetry

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform int iterations;     // Mirror iterations (1-8)
uniform float rotation;     // Pattern rotation (radians)
uniform float time;         // Animation time (seconds)
uniform float twistAngle;   // Per-iteration rotation offset (radians)

out vec4 finalColor;

// Mirror UV into 0-1 range
vec2 mirror(vec2 x)
{
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

void main()
{
    vec2 uv = (fragTexCoord - 0.5) * 2.0;  // Scale to -1..1 range
    float a = rotation + time * 0.1;

    for (int i = 0; i < iterations; i++) {
        // Rotate
        float c = cos(a), s = sin(a);
        uv = vec2(s * uv.y - c * uv.x, s * uv.x + c * uv.y);

        // Mirror
        uv = mirror(uv);

        // Evolve angle (creates variation per iteration)
        a += float(i + 1);
        a /= float(i + 1);
    }

    // Apply twist based on distance
    float r = length(uv);
    float angle = atan(uv.y, uv.x) + twistAngle * (1.0 - r);
    uv = vec2(cos(angle), sin(angle)) * r;

    finalColor = texture(texture0, mirror(uv * 0.5 + 0.5));
}
