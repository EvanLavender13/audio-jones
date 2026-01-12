#version 330

// Duotone: Maps luminance to two-color gradient
// Shadow color fills dark regions, highlight color fills bright regions

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec3 shadowColor;     // Color for dark regions
uniform vec3 highlightColor;  // Color for bright regions
uniform float intensity;      // Blend: 0 = original, 1 = full duotone

out vec4 finalColor;

// BT.601 luminance weights
const vec3 LUMINANCE_WEIGHTS = vec3(0.299, 0.587, 0.114);

void main()
{
    vec3 color = texture(texture0, fragTexCoord).rgb;

    // Extract luminance
    float lum = dot(color, LUMINANCE_WEIGHTS);

    // Map luminance to two-color gradient
    vec3 toned = mix(shadowColor, highlightColor, lum);

    // Blend between original and duotone
    vec3 result = mix(color, toned, intensity);

    finalColor = vec4(result, 1.0);
}
