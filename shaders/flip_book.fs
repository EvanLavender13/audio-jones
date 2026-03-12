#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // held frame
uniform vec2 resolution;
uniform float jitterSeed;     // float(frameIndex)
uniform float jitterAmount;   // 0.0-8.0, pixels of offset

void main() {
    vec2 jitter = vec2(
        fract(sin(jitterSeed * 12.9898) * 43758.5453),
        fract(sin(jitterSeed * 78.233 + 0.5) * 43758.5453)
    ) * 2.0 - 1.0;

    vec2 uv = fragTexCoord + jitter * jitterAmount / resolution;
    finalColor = texture(texture0, uv);
}
