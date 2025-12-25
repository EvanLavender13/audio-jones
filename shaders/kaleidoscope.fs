#version 330

// Kaleidoscope post-process effect
// Mirrors visual output into radial segments around screen center

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform int segments;       // Number of mirror segments (1 = disabled)
uniform float rotation;     // Pattern rotation in radians

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

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

    // Center UV coordinates (range: -0.5 to 0.5)
    vec2 uv = fragTexCoord - 0.5;

    // Convert to polar coordinates
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

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
