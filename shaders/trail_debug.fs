#version 330

// Physarum trail map debug visualization
// Displays RGBA32F trail color directly

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;

void main()
{
    vec3 trailColor = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(trailColor, 1.0);
}
