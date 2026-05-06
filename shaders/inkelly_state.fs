// Based on "Inkelly" by leon
// https://www.shadertoy.com/view/4fl3Wl
// License: CC BY-NC-SA 3.0 Unported
// Modified: continuous-morph time (replaces stepped delay), lissajous-curve
// spawn shape (replaces horizontal bar), parameterized decay/distortion/advection
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // previous mask field (auto-bound by DrawTextureRec)
uniform vec2 resolution;
uniform float morphPhase;         // CPU-accumulated noise time
uniform float deltaTime;
uniform float decayRate;
uniform float spawnDistort;
uniform float advectScale;
uniform float lineThickness;
uniform int lissajousSamples;

uniform float lissAmplitude;
uniform float lissPhase;
uniform float lissFreqX1;
uniform float lissFreqY1;
uniform float lissFreqX2;
uniform float lissFreqY2;
uniform float lissOffsetX2;
uniform float lissOffsetY2;

const float TWO_PI = 6.28318530718;

float gyroid(vec3 seed) { return dot(sin(seed), cos(seed.yzx)); }

float fbm(vec2 pos) {
    float t = morphPhase;
    float t2 = t * 1.354;
    vec3 p = vec3(pos, t);
    float result = 0.0;
    float a = 0.5;
    for (int i = 0; i < 3; ++i) {
        result += abs(gyroid(p / a) * a);
        a /= 2.0;
    }
    result = sin(result * 6.283 + t2 - pos.x);
    return result;
}

vec2 lissajous(float t) {
    float x = sin(lissFreqX1 * t + lissPhase);
    float y = cos(lissFreqY1 * t + lissPhase);
    float nx = 1.0;
    float ny = 1.0;
    if (lissFreqX2 != 0.0) {
        x += sin(lissFreqX2 * t + lissOffsetX2 + lissPhase);
        nx = 2.0;
    }
    if (lissFreqY2 != 0.0) {
        y += cos(lissFreqY2 * t + lissOffsetY2 + lissPhase);
        ny = 2.0;
    }
    return vec2((x / nx) * lissAmplitude, (y / ny) * lissAmplitude);
}

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p = (fragTexCoord * resolution * 2.0 - resolution) / resolution.y;

    vec2 e = vec2(4.0 / resolution.y, 0.0);
    vec2 curl = vec2(fbm(p + e.xy) - fbm(p - e.xy),
                     fbm(p + e.yx) - fbm(p - e.yx)) / (2.0 * e.x);
    curl = vec2(curl.y, -curl.x);

    p += curl * spawnDistort;

    float minDist = 1e9;
    for (int i = 0; i < lissajousSamples; ++i) {
        float t0 = TWO_PI * float(i) / float(lissajousSamples);
        float t1 = TWO_PI * float(i + 1) / float(lissajousSamples);
        vec2 A = lissajous(t0);
        vec2 B = lissajous(t1);
        float d = sdSegment(p, A, B);
        minDist = min(minDist, d);
    }
    float spawnMask = smoothstep(lineThickness, 0.0, minDist);

    float prevMask = texture(texture0, uv + curl * advectScale).r;

    float mask = max(spawnMask, prevMask - decayRate * deltaTime);

    finalColor = vec4(mask, 0.0, 0.0, 1.0);
}
