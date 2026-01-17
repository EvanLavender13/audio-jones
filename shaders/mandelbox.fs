#version 330

// Mandelbox: Box fold + sphere fold fractal transform

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform int iterations;        // 1-12
uniform float boxLimit;        // Box fold boundary (0.5-2.0)
uniform float sphereMin;       // Inner sphere radius (0.1-0.5)
uniform float sphereMax;       // Outer sphere radius (0.5-2.0)
uniform float scale;           // Expansion per iteration (1.5-3.0)
uniform vec2 mandelboxOffset;  // Translation after fold
uniform float rotation;        // Global rotation (accumulated)
uniform float twistAngle;      // Per-iteration rotation (accumulated)
uniform float boxIntensity;    // Box fold contribution (0.0-1.0)
uniform float sphereIntensity; // Sphere fold contribution (0.0-1.0)

out vec4 finalColor;

vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

vec2 mirror(vec2 x)
{
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

void main()
{
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Global rotation
    p = rotate2d(p, rotation);

    float sphereMin2 = sphereMin * sphereMin;
    float sphereMax2 = sphereMax * sphereMax;

    // Mandelbox iteration
    for (int i = 0; i < iterations; i++) {
        // Box fold: conditional reflection at +-boxLimit
        vec2 boxed = clamp(p, -boxLimit, boxLimit) * 2.0 - p;
        p = mix(p, boxed, boxIntensity);

        // Sphere fold: inversion through nested spheres
        float r2 = dot(p, p);
        vec2 sphered = p;
        if (r2 < sphereMin2) {
            sphered *= sphereMax2 / sphereMin2;
        } else if (r2 < sphereMax2) {
            sphered *= sphereMax2 / r2;
        }
        p = mix(p, sphered, sphereIntensity);

        // Scale and translate
        p = p * scale - mandelboxOffset;

        // Per-iteration rotation
        p = rotate2d(p, twistAngle);
    }

    // Sample texture at transformed position
    vec2 newUV = mirror(p * 0.5 + 0.5);
    finalColor = texture(texture0, newUV);
}
