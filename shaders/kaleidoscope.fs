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

    // Apply rotation offset
    angle += rotation;

    // Segment and mirror
    float segmentAngle = TWO_PI / float(segments);
    angle = mod(angle, segmentAngle);
    angle = min(angle, segmentAngle - angle);

    // Convert back to cartesian
    vec2 kaleidoUV = vec2(cos(angle), sin(angle)) * radius + 0.5;
    kaleidoUV = clamp(kaleidoUV, 0.0, 1.0);

    finalColor = texture(texture0, kaleidoUV);
}
