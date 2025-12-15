#version 330

// Physarum trail map debug visualization
// Converts single-channel R32F texture to grayscale

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;

void main()
{
    float intensity = texture(texture0, fragTexCoord).r;
    finalColor = vec4(intensity, intensity, intensity, 1.0);
}
