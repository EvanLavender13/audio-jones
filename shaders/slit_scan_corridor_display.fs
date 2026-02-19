#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;  // corridor ping-pong result
uniform float rotation;

void main() {
    vec2 centered = fragTexCoord - vec2(0.5);
    float c = cos(rotation), s = sin(rotation);
    vec2 rotated = vec2(c * centered.x + s * centered.y,
                        -s * centered.x + c * centered.y) + vec2(0.5);
    finalColor = texture(texture0, rotated);
}
