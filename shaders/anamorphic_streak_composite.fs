#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // Original image
uniform sampler2D streakTexture; // Blurred streak
uniform float intensity;

void main() {
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 streak = texture(streakTexture, fragTexCoord).rgb;
    finalColor = vec4(original + streak * intensity, 1.0);
}
