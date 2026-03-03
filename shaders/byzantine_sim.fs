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
uniform float diffusionWeight;
uniform float sharpenWeight;
uniform float zoomAmount;
uniform vec2 center;

vec4 process(vec2 uv) {
    vec2 texel = 1.0 / resolution;
    vec4 v0 = texture(texture0, uv + vec2(-texel.x, 0.0));
    vec4 v1 = texture(texture0, uv + vec2( texel.x, 0.0));
    vec4 v2 = texture(texture0, uv + vec2(0.0, -texel.y));
    vec4 v3 = texture(texture0, uv + vec2(0.0,  texel.y));
    vec4 v4 = texture(texture0, uv);
    float w = ((frameCount % 2) == 0) ? diffusionWeight : sharpenWeight;
    float k = (1.0 - w) / 4.0;
    return w * v4 + k * (v0 + v1 + v2 + v3);
}

void main() {
    vec2 uv = fragTexCoord;

    if (frameCount == 0) {
        vec2 p = uv * 2.0 - 1.0;
        p.x *= resolution.x / resolution.y;
        float c0 = length(p) - 0.5;
        float c1 = length(p - vec2(0.0, 0.25));
        float c2 = length(p - vec2(0.0, -0.25));
        float d = max(min(max(c0, -p.x), c1 - 0.25), -c2 + 0.25);
        d = max(d, -c1 + 0.07);
        d = min(d, c2 - 0.07);
        d = min(d, abs(c0) - 0.01);
        float w = sign(d) * sin(c0 * 80.0);
        finalColor = vec4(w, w, w, 1.0);
    } else if ((frameCount % cycleLength) == 0) {
        // Zoom reseed: resample at zoomAmount toward center
        vec2 zoomed = (uv - center) / zoomAmount + center;
        finalColor = vec4(texture(texture0, zoomed).rgb, 1.0);
    } else {
        vec3 c = clamp(process(uv).rgb, -1.0, 1.0);
        finalColor = vec4(c, 1.0);
    }
}
