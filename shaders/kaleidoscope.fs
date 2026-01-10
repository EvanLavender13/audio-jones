#version 330

// Kaleidoscope (Polar): Wedge-based radial mirroring with optional smooth edges

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform int segments;         // Mirror segments / wedge count (1-12)
uniform float rotation;       // Pattern rotation (radians)
uniform float time;           // Animation time (seconds)
uniform float twistAngle;     // Radial twist offset (radians)
uniform float smoothing;      // Blend width at wedge seams (0.0-0.5, 0 = hard edge)
uniform vec2 focalOffset;     // Lissajous center offset (UV units)
uniform float warpStrength;   // fBM warp intensity (0 = disabled)
uniform float warpSpeed;      // fBM animation speed
uniform float noiseScale;     // fBM spatial scale

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float PI = 3.14159265359;

// Hash function for gradient noise
vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy) * 2.0 - 1.0;
}

// 2D gradient noise
float gnoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(dot(hash22(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
                   dot(hash22(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
               mix(dot(hash22(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
                   dot(hash22(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x), u.y);
}

// fBM: 4 octaves, lacunarity=2.0, gain=0.5
float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * gnoise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

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
    // Center UV coordinates and apply focal offset
    vec2 uv = fragTexCoord - 0.5 - focalOffset;

    // Apply fBM warp to input UV
    if (warpStrength > 0.0) {
        vec2 noiseCoord = uv * noiseScale + time * warpSpeed;
        uv += vec2(fbm(noiseCoord), fbm(noiseCoord + vec2(5.2, 1.3))) * warpStrength;
    }

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
