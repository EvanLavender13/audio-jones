#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float texZoom;
uniform float texAngle;
uniform float texBrightness;

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord - 0.5;
    float c = cos(texAngle);
    float s = sin(texAngle);
    uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);
    uv /= texZoom;
    uv = clamp(uv + 0.5, 0.0, 1.0);
    vec4 sampled = texture(texture0, uv);
    sampled.rgb *= texBrightness;
    finalColor = sampled * fragColor;
}
