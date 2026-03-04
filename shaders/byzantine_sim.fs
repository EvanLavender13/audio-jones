// Based on "Byzantine Buffering" by paniq
// https://www.shadertoy.com/view/7tBGz1
// License: CC BY-NC-SA 3.0 Unported

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int frameCount;
uniform int cycleLength;
uniform float zoomAmount;
uniform vec2 center;
uniform float rotation;
uniform float twist;

const float DIFFUSION_W = 0.367879;
const float SHARPEN_W = 3.0;

vec2 rotate2D(vec2 v, float angle) {
    float c = cos(angle), s = sin(angle);
    return vec2(c * v.x - s * v.y, s * v.x + c * v.y);
}

vec4 process(vec2 uv) {
    vec2 off = uv - center;
    float dist = length(off);
    off = rotate2D(off, rotation + twist * dist);
    vec2 rotUv = off + center;

    vec2 texel = 1.0 / resolution;
    vec4 v0 = texture(texture0, rotUv + vec2(-texel.x, 0.0));
    vec4 v1 = texture(texture0, rotUv + vec2( texel.x, 0.0));
    vec4 v2 = texture(texture0, rotUv + vec2(0.0, -texel.y));
    vec4 v3 = texture(texture0, rotUv + vec2(0.0,  texel.y));
    vec4 v4 = texture(texture0, rotUv);
    float w = ((frameCount % 2) == 0) ? DIFFUSION_W : SHARPEN_W;
    float k = (1.0 - w) / 4.0;
    return w * v4 + k * (v0 + v1 + v2 + v3);
}

void main() {
    vec2 uv = fragTexCoord;

    if (frameCount == 0) {
        vec2 p = (uv - center) * 2.0;
        p.x *= resolution.x / resolution.y;
        float d = length(p);
        float w = sin(d * 40.0);
        finalColor = vec4(w, w, w, 1.0);
    } else if ((frameCount % cycleLength) == 0) {
        // Zoom + rotate reseed: matches display counter-zoom/rotation
        vec2 off = (uv - center) / zoomAmount;
        float dist = length(off);
        float cycleAngle = rotation * float(cycleLength)
                         + twist * float(cycleLength) * dist;
        off = rotate2D(off, cycleAngle);
        vec3 resampled = texture(texture0, off + center).rgb;
        // Inject seed pattern to prevent convergence to uniform
        vec2 p = (uv - center) * 2.0;
        p.x *= resolution.x / resolution.y;
        float seed = sin(length(p) * 40.0) * 0.05;
        finalColor = vec4(resampled + seed, 1.0);
    } else {
        vec3 c = clamp(process(uv).rgb, -1.0, 1.0);
        finalColor = vec4(c, 1.0);
    }
}
