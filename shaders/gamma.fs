#version 330

// Final pass: soft shoulder tone mapping + gamma correction

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float gamma;

void main()
{
    vec3 color = texture(texture0, fragTexCoord).rgb;

    // Hue-preserving normalization: scale all channels equally when any exceeds 1.0
    float maxC = max(color.r, max(color.g, color.b));
    if (maxC > 1.0) {
        color /= maxC;
    }

    vec3 corrected = pow(color, vec3(1.0 / gamma));
    finalColor = vec4(corrected, 1.0);
}
