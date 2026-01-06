#version 330

// Clarity pass: enhances local contrast at mid-frequencies
// Samples at multiple radii to create a regional blur, then boosts difference

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float clarity;

void main()
{
    vec2 texelSize = 1.0 / resolution;
    vec3 center = texture(texture0, fragTexCoord).rgb;

    // Sample at multiple radii (8, 16, 24 pixels) in 8 directions
    vec3 blur = vec3(0.0);
    float radii[3] = float[](8.0, 16.0, 24.0);

    for (int r = 0; r < 3; r++) {
        float radius = radii[r];
        vec2 offset = texelSize * radius;

        // 8 directions: cardinal + diagonal
        blur += texture(texture0, fragTexCoord + vec2(offset.x, 0.0)).rgb;
        blur += texture(texture0, fragTexCoord - vec2(offset.x, 0.0)).rgb;
        blur += texture(texture0, fragTexCoord + vec2(0.0, offset.y)).rgb;
        blur += texture(texture0, fragTexCoord - vec2(0.0, offset.y)).rgb;
        blur += texture(texture0, fragTexCoord + vec2(offset.x, offset.y) * 0.707).rgb;
        blur += texture(texture0, fragTexCoord + vec2(-offset.x, offset.y) * 0.707).rgb;
        blur += texture(texture0, fragTexCoord + vec2(offset.x, -offset.y) * 0.707).rgb;
        blur += texture(texture0, fragTexCoord + vec2(-offset.x, -offset.y) * 0.707).rgb;
    }
    blur /= 24.0; // 3 radii * 8 directions

    // Mid-frequency detail: difference between center and regional average
    vec3 detail = center - blur;

    // Boost local contrast
    vec3 result = max(center + detail * clarity, vec3(0.0));

    finalColor = vec4(result, 1.0);
}
