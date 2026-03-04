// Counter-zoom display pass with barrel distortion, chromatic aberration,
// gradient LUT tinting, and counter-rotation for spiral zoom.
//
// Based on "Byzantine Buffering" by paniq
// https://www.shadertoy.com/view/7tBGz1
// License: CC BY-NC-SA 3.0 Unported
//
// Modified: pow(0.5) grayscale replaced with gradient LUT tinting via luminance;
// counter-rotation and twist added for spiral zoom; barrel distortion and
// chromatic offset recentered at configurable zoom focus; hardcoded zoom/center
// parameterized as uniforms.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float cycleProgress;
uniform float zoomAmount;
uniform vec2 center;
uniform float cycleRotation;
uniform float cycleTwist;

vec2 rotate2D(vec2 v, float angle) {
    float c = cos(angle), s = sin(angle);
    return vec2(c * v.x - s * v.y, s * v.x + c * v.y);
}

void main() {
    float m = cycleProgress;

    vec2 off = fragTexCoord - center;

    // Barrel distortion (centered at zoom focus)
    off.x *= resolution.x / resolution.y;
    const float BARREL_K = 0.1; // Radial distortion strength
    off = off / (1.0 + BARREL_K * dot(off, off));
    off.x /= resolution.x / resolution.y;

    // Counter-zoom
    off *= exp(mix(log(1.0), log(1.0 / zoomAmount), m));

    // Counter-rotation (matches reseed rotation over cycle)
    float dist = length(off);
    float angle = (cycleRotation + cycleTwist * dist) * m;
    off = rotate2D(off, angle);

    vec2 uv = off + center;

    // Chromatic zoom offset (relative to center)
    const float CHROMA_OFFSET = 0.004; // Per-channel zoom separation
    vec2 uv3 = uv;
    vec2 uv2 = center + (uv - center) * exp(-CHROMA_OFFSET);
    vec2 uv1 = center + (uv - center) * exp(-CHROMA_OFFSET * 2.0);

    float r = texture(texture0, uv1).r * 0.5 + 0.5;
    float g = texture(texture0, uv2).r * 0.5 + 0.5;
    float b = texture(texture0, uv3).r * 0.5 + 0.5;

    vec3 bw = pow(vec3(r, g, b), vec3(0.5));
    float lum = dot(bw, vec3(0.299, 0.587, 0.114));
    vec3 tint = texture(gradientLUT, vec2(lum, 0.5)).rgb;
    finalColor = vec4(tint * bw, 1.0);
}
