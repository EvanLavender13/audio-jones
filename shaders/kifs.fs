#version 330

// KIFS (Kaleidoscopic IFS): Fractal folding via Cartesian abs() reflection

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform int iterations;      // 1-6
uniform float scale;         // 1.5-2.5
uniform vec2 kifsOffset;     // Translation after fold
uniform float rotation;      // Global rotation (accumulated)
uniform float twistAngle;    // Per-iteration rotation
uniform bool octantFold;     // Optional octant folding
uniform bool polarFold;      // Enable polar pre-fold for radial symmetry
uniform int polarFoldSegments; // Wedge count for polar fold (2-12)
uniform float polarFoldSmoothing; // Blend width at polar fold seams (0.0-0.5)

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

vec2 doPolarFold(vec2 p, int segments, float smoothing)
{
    float radius = length(p);
    float angle = atan(p.y, p.x);

    // Fold angle into segment
    float segmentAngle = TWO_PI / float(segments);
    float halfSeg = PI / float(segments);
    float c = floor((angle + halfSeg) / segmentAngle);
    angle = mod(angle + halfSeg, segmentAngle) - halfSeg;
    angle *= mod(c, 2.0) * 2.0 - 1.0;  // Mirror alternating segments

    // Apply smooth fold when smoothing > 0
    if (smoothing > 0.0) {
        float sa = halfSeg - pabs(halfSeg - abs(angle), smoothing);
        angle = sign(angle) * sa;
    }

    return vec2(cos(angle), sin(angle)) * radius;
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

void main()
{
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Initial rotation
    p = rotate2d(p, rotation);

    // Polar fold before KIFS iteration
    if (polarFold) {
        p = doPolarFold(p, polarFoldSegments, polarFoldSmoothing);
    }

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
