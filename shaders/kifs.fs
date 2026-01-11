#version 330

// KIFS (Kaleidoscopic IFS): Fractal folding via Cartesian abs() reflection

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform int iterations;      // 1-12
uniform float scale;         // 1.5-4.0
uniform vec2 kifsOffset;     // Translation after fold
uniform float rotation;      // Global rotation (accumulated)
uniform float twistAngle;    // Per-iteration rotation
uniform bool octantFold;     // Optional octant folding

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
