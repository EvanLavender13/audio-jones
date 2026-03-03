// Based on "Byzantine Buffering" by paniq
// https://www.shadertoy.com/view/7tBGz1
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced yin-yang SDF seed with concentric sine rings,
//           parameterized kernel weights and cycle length for AudioJones

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // Previous ping-pong buffer
uniform vec2 resolution;
uniform int frameCount;
uniform int cycleLength;
uniform float diffusionWeight;
uniform float sharpenWeight;
uniform float zoomAmount;
uniform vec2 center;

void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    if (frameCount == 0) {
        // Seed: concentric sine rings centered at zoom focus
        vec2 centered = (uv - center) * vec2(resolution.x / resolution.y, 1.0);
        float d = length(centered);
        float w = sin(d * 80.0);
        finalColor = vec4(w);
    } else if (frameCount % cycleLength == 0) {
        // Zoom reseed: resample at reduced scale toward center
        // Upscaling interpolation artifacts feed fresh perturbation
        vec2 resampled = (uv - center) / zoomAmount + center;
        finalColor = texture(texture0, resampled);
    } else {
        // 5-tap von Neumann kernel with alternating weights
        // Even frames: diffusion (w < 1, k > 0 => blur)
        // Odd frames: sharpening (w > 1, k < 0 => edge enhance)
        vec4 v0 = texture(texture0, uv + vec2(-texel.x, 0.0));
        vec4 v1 = texture(texture0, uv + vec2( texel.x, 0.0));
        vec4 v2 = texture(texture0, uv + vec2(0.0, -texel.y));
        vec4 v3 = texture(texture0, uv + vec2(0.0,  texel.y));
        vec4 v4 = texture(texture0, uv);

        float w = ((frameCount % 2) == 0) ? diffusionWeight : sharpenWeight;
        float k = (1.0 - w) / 4.0;

        finalColor = clamp(w * v4 + k * (v0 + v1 + v2 + v3), -1.0, 1.0);
    }
}
