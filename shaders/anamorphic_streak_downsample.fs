#version 330

// Anamorphic streak downsample: 6 horizontal taps with triangle weights
// Offsets -5,-3,-1,+1,+3,+5 texels, weights 1,2,3,3,2,1 (sum 12)

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float texelSize; // 1.0 / source width

void main() {
    float y = fragTexCoord.y;
    vec3 sum = texture(texture0, vec2(fragTexCoord.x - 5.0 * texelSize, y)).rgb * 1.0
             + texture(texture0, vec2(fragTexCoord.x - 3.0 * texelSize, y)).rgb * 2.0
             + texture(texture0, vec2(fragTexCoord.x - 1.0 * texelSize, y)).rgb * 3.0
             + texture(texture0, vec2(fragTexCoord.x + 1.0 * texelSize, y)).rgb * 3.0
             + texture(texture0, vec2(fragTexCoord.x + 3.0 * texelSize, y)).rgb * 2.0
             + texture(texture0, vec2(fragTexCoord.x + 5.0 * texelSize, y)).rgb * 1.0;
    finalColor = vec4(sum / 12.0, 1.0);
}
