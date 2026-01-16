#version 330

// Bloom composite: Additive blend with intensity control
// Combines original image with blurred bloom texture

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D bloomTexture;
uniform float intensity;

out vec4 finalColor;

void main()
{
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 bloom = texture(bloomTexture, fragTexCoord).rgb;
    finalColor = vec4(original + bloom * intensity, 1.0);
}
