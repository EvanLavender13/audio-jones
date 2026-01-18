#version 330

// Kaleidoscope (Polar): Wedge-based radial mirroring with optional smooth edges

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform int segments;         // Mirror segments / wedge count (1-12)
uniform float rotation;       // Pattern rotation (radians)
uniform float twistAngle;     // Radial twist offset (radians)
uniform float smoothing;      // Blend width at wedge seams (0.0-0.5, 0 = hard edge)

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

void main()
{
    // Center UV coordinates
    vec2 uv = fragTexCoord - 0.5;

    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    // Radial twist (inner rotates more)
    angle += twistAngle * (1.0 - radius);

    // Mirror corners into circle
    if (radius > 0.5) {
        radius = 1.0 - radius;
    }

    // Apply rotation
    angle += rotation;

    // Segment mirroring with optional smooth edges
    float segmentAngle = TWO_PI / float(segments);
    float halfSeg = PI / float(segments);
    float c = floor((angle + halfSeg) / segmentAngle);
    angle = mod(angle + halfSeg, segmentAngle) - halfSeg;
    angle *= mod(c, 2.0) * 2.0 - 1.0;  // flip alternating segments

    // Apply smooth fold when smoothing > 0
    if (smoothing > 0.0) {
        float sa = halfSeg - pabs(halfSeg - abs(angle), smoothing);
        angle = sign(angle) * sa;
    } else {
        // Hard edge (original behavior)
        angle = min(abs(angle), segmentAngle - abs(angle));
    }

    // 4x supersampling for smooth edges
    float offset = 0.002;
    vec3 color = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        float aOff = angle + (float(i / 2) - 0.5) * offset;
        float rOff = radius + (float(i % 2) - 0.5) * offset * 0.5;
        vec2 newUV = vec2(cos(aOff), sin(aOff)) * rOff + 0.5;
        color += texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
    }

    finalColor = vec4(color * 0.25, 1.0);
}
