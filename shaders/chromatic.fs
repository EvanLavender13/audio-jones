#version 330

// Radial chromatic aberration post-process
// Splits RGB channels outward from screen center on beats

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float chromaticOffset; // Current offset in pixels (0 = no effect)

out vec4 finalColor;

void main()
{
    vec2 center = vec2(0.5);
    vec2 toCenter = fragTexCoord - center;
    float dist = length(toCenter);

    // Fade effect smoothly near center to avoid direction instability
    float fade = smoothstep(0.0, 0.15, dist);

    // Radial direction, uniform magnitude with center fade
    vec2 dir = dist > 0.001 ? toCenter / dist : vec2(0.0);
    vec2 offset = dir * chromaticOffset * fade / resolution.x;

    // Sample RGB channels with radial offset
    float r = texture(texture0, fragTexCoord + offset).r;
    float g = texture(texture0, fragTexCoord).g;
    float b = texture(texture0, fragTexCoord - offset).b;

    finalColor = vec4(r, g, b, 1.0);
}
