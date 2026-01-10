#version 330

// KIFS (Kaleidoscopic IFS): Fractal folding via iterated mirror/rotate/scale

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform int segments;       // Mirror segments per iteration (1-12)
uniform float rotation;     // Pattern rotation (radians)
uniform float time;         // Animation time (seconds)
uniform float twistAngle;   // Per-iteration rotation offset (radians)
uniform int iterations;     // Folding iterations (1-8)
uniform float scale;        // Per-iteration scale factor
uniform vec2 kifsOffset;    // Translation after fold

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

// 2D rotation
vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

// Mirror UV into 0-1 range
vec2 mirror(vec2 x)
{
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

void main()
{
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Apply twist + rotation
    float r = length(p);
    p = rotate2d(p, twistAngle * (1.0 - r * 0.5) + rotation);

    // Apply all transformations, sample ONCE at end
    for (int i = 0; i < iterations; i++) {
        float ri = length(p);
        float a = atan(p.y, p.x);

        // Polar fold into segment
        float foldAngle = TWO_PI / float(segments);
        a = abs(mod(a + foldAngle * 0.5, foldAngle) - foldAngle * 0.5);

        // Back to Cartesian
        p = vec2(cos(a), sin(a)) * ri;

        // IFS contraction
        p = p * scale - kifsOffset;

        // Per-iteration rotation
        p = rotate2d(p, float(i) * 0.3 + time * 0.05);
    }

    // Single sample at final transformed position
    vec2 newUV = mirror(p * 0.5 + 0.5);
    finalColor = texture(texture0, newUV);
}
