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

    if (dist < 0.001) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    vec2 dir = normalize(toCenter);

    // Convert pixel offset to UV, scale with distance from center
    float pixelToUV = 1.0 / min(resolution.x, resolution.y);
    vec2 offset = dir * chromaticOffset * pixelToUV * dist * 4.0;

    // Clamp UVs to prevent wrapping at screen edges
    float r = texture(texture0, clamp(fragTexCoord + offset, 0.0, 1.0)).r;
    float g = texture(texture0, fragTexCoord).g;
    float b = texture(texture0, clamp(fragTexCoord - offset, 0.0, 1.0)).b;

    finalColor = vec4(r, g, b, 1.0);
}
