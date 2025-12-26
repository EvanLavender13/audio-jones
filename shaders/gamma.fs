#version 330

// Gamma correction pass: applies display gamma adjustment

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float gamma;

void main()
{
    vec3 color = texture(texture0, fragTexCoord).rgb;
    vec3 corrected = pow(color, vec3(1.0 / gamma));

    finalColor = vec4(corrected, 1.0);
}
