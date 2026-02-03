#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float offset;     // Current iteration offset in pixels
uniform float sharpness;  // 0.0 = soft Kawase, 1.0 = flat weights

void main() {
    vec2 texelSize = 1.0 / resolution;
    float centerWeight = mix(0.5, 0.34, sharpness);
    float sideWeight = (1.0 - centerWeight) * 0.5;

    vec3 sum = texture(texture0, fragTexCoord).rgb * centerWeight;
    sum += texture(texture0, fragTexCoord + vec2(-offset * texelSize.x, 0.0)).rgb * sideWeight;
    sum += texture(texture0, fragTexCoord + vec2( offset * texelSize.x, 0.0)).rgb * sideWeight;

    finalColor = vec4(sum, 1.0);
}
