#version 330

// Kaleidoscope post-process effect
// Mirrors visual output into radial segments around screen center
// Supports radial twist, moving focal point, and fBM domain warping

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform int segments;         // Mirror segments (1 = disabled)
uniform float rotation;       // Pattern rotation (radians)
uniform float time;           // Animation time (seconds)
uniform float twist;          // Radial twist amount (radians)
uniform vec2 focalOffset;     // Lissajous center offset (UV units)
uniform float warpStrength;   // fBM warp intensity (0 = disabled)
uniform float warpSpeed;      // fBM animation speed
uniform float noiseScale;     // fBM spatial scale

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

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

// Sample kaleidoscope at given angle and radius
vec4 sampleKaleido(float angle, float radius, float segmentAngle)
{
    float a = mod(angle, segmentAngle);
    a = min(a, segmentAngle - a);
    vec2 uv = vec2(cos(a), sin(a)) * radius + 0.5;
    return texture(texture0, clamp(uv, 0.0, 1.0));
}

void main()
{
    // Bypass when disabled
    if (segments <= 1) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    // Center UV coordinates and apply focal offset
    vec2 uv = fragTexCoord - 0.5 - focalOffset;

    // Convert to polar coordinates
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    // Apply radial twist (inner regions rotate more)
    angle += twist * (1.0 - radius);

    // Apply fBM domain warping
    if (warpStrength > 0.0) {
        vec2 polar = vec2(cos(angle), sin(angle)) * radius;
        vec2 noiseCoord = polar * noiseScale + time * warpSpeed;
        float warpX = fbm(noiseCoord);
        float warpY = fbm(noiseCoord + vec2(5.2, 1.3));
        polar += vec2(warpX, warpY) * warpStrength;
        radius = length(polar);
        angle = atan(polar.y, polar.x);
    }

    // Mirror corners back into the circle (keeps circular mandala shape)
    if (radius > 0.5) {
        radius = 1.0 - radius;
    }

    // Apply rotation offset
    angle += rotation;

    float segmentAngle = TWO_PI / float(segments);

    // 4x supersampling with angular offsets to smooth segment boundaries
    float offset = 0.002;  // Angular offset for sub-pixel sampling
    vec4 color = sampleKaleido(angle - offset, radius, segmentAngle)
               + sampleKaleido(angle + offset, radius, segmentAngle)
               + sampleKaleido(angle, radius - offset * 0.5, segmentAngle)
               + sampleKaleido(angle, radius + offset * 0.5, segmentAngle);

    finalColor = color * 0.25;
}
