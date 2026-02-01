#version 330

// Radial IFS: Iterated polar wedge folding for snowflake/flower fractals

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform int segments;       // Wedge count (3-12)
uniform int iterations;     // Recursion depth (1-8)
uniform float scale;        // Expansion per iteration (1.2-2.5)
uniform float offset;       // Translation after fold (0.0-2.0)
uniform float rotation;     // Global rotation (accumulated)
uniform float twistAngle;   // Per-iteration rotation (accumulated)
uniform float smoothing;    // Blend width at wedge seams (0.0-0.5)

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float PI = 3.14159265359;

// Polynomial smooth min (for smooth folding)
float pmin(float a, float b, float k)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// Polynomial smooth absolute (for soft wedge edges)
float pabs(float a, float k)
{
    return -pmin(a, -a, k);
}

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

vec2 radialFold(vec2 p, int segs, float smoothK)
{
    float segmentAngle = TWO_PI / float(segs);
    float halfSeg = PI / float(segs);

    // Convert to polar
    float r = length(p);
    float a = atan(p.y, p.x);

    // Fold angle into wedge
    float c = floor((a + halfSeg) / segmentAngle);
    a = mod(a + halfSeg, segmentAngle) - halfSeg;
    a *= mod(c, 2.0) * 2.0 - 1.0;  // Mirror alternating segments

    // Apply smooth fold when smoothing > 0
    if (smoothK > 0.0) {
        float sa = halfSeg - pabs(halfSeg - abs(a), smoothK);
        a = sign(a) * sa;
    }

    // Back to Cartesian, then translate
    return vec2(cos(a), sin(a)) * r - vec2(offset, 0.0);
}

void main()
{
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Global rotation
    p = rotate2d(p, rotation);

    // Radial IFS iteration
    for (int i = 0; i < iterations; i++) {
        p = radialFold(p, segments, smoothing);
        p *= scale;
        p = rotate2d(p, twistAngle);
    }

    // Sample texture at transformed position
    vec2 newUV = mirror(p * 0.5 + 0.5);
    finalColor = texture(texture0, newUV);
}
