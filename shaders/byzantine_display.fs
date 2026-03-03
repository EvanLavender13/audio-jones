// Based on "Byzantine Buffering" by paniq
// https://www.shadertoy.com/view/7tBGz1
// License: CC BY-NC-SA 3.0 Unported

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float cycleProgress; // 0..1 smooth progress through current cycle
uniform float zoomAmount;
uniform vec2 center;

void main() {
    float m = cycleProgress;

    // All operations relative to zoom center
    vec2 off = fragTexCoord - center;

    // Barrel distortion (centered at zoom focus)
    off.x *= resolution.x / resolution.y;
    off = off / (1.0 + 0.1 * dot(off, off));
    off.x /= resolution.x / resolution.y;

    // Counter-zoom: smoothly compensate for discrete zoom reseeds
    off *= exp(mix(log(1.0), log(1.0 / zoomAmount), m));

    vec2 uv = off + center;

    // Chromatic zoom offset (relative to center)
    float f = 0.004;
    vec2 uv3 = uv;
    vec2 uv2 = center + (uv - center) * exp(-f);
    vec2 uv1 = center + (uv - center) * exp(-f * 2.0);

    float r = texture(texture0, uv1).r * 0.5 + 0.5;
    float g = texture(texture0, uv2).r * 0.5 + 0.5;
    float b = texture(texture0, uv3).r * 0.5 + 0.5;

    // Reference output: direct RGB + gamma (diagnostic — bypass gradient LUT)
    finalColor = vec4(pow(vec3(r, g, b), vec3(0.5)), 1.0);
}
