// Based on "Dancing lines (Qix)" by FabriceNeyret2
// https://www.shadertoy.com/view/ffSSRt
// License: CC BY-NC-SA 3.0 Unported
// Modified: snap clock + trail length as uniforms, dual-Lissajous endpoints
// from shared config, gradient LUT coloring, per-echo FFT band brightness

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float phase;

uniform float amplitude;
uniform float freqX1;
uniform float freqY1;
uniform float freqX2;
uniform float freqY2;
uniform float offsetX2;
uniform float offsetY2;

uniform float accumTime;
uniform float snapRate;
uniform int trailLength;
uniform float trailFade;
uniform float colorPhaseStep;

uniform float lineThickness;
uniform float endpointOffset;
uniform float baseBright;
uniform float glowIntensity;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;

const float GLOW_WIDTH = 0.001; // AA outer edge offset above lineThickness (R.y units)

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

vec2 lissajous(float t) {
    float x = sin(freqX1 * t + phase);
    float y = cos(freqY1 * t + phase);

    float nx = 1.0;
    float ny = 1.0;

    if (freqX2 != 0.0) {
        x += sin(freqX2 * t + offsetX2 + phase);
        nx = 2.0;
    }
    if (freqY2 != 0.0) {
        y += cos(freqY2 * t + offsetY2 + phase);
        ny = 2.0;
    }

    float aspect = resolution.x / resolution.y;
    return vec2((x / nx) * amplitude * aspect, (y / ny) * amplitude);
}

void main() {
    vec2 uv = (fragTexCoord * resolution * 2.0 - resolution) / min(resolution.x, resolution.y);

    vec3 result = vec3(0.0);
    float fadeAccum = 1.0;
    float snapTick = floor(accumTime * snapRate);
    float fTrail = float(trailLength);

    for (int i = 0; i < trailLength; i++) {
        float fi = float(i);

        float t = colorPhaseStep * (fi + snapTick);

        vec2 P = lissajous(t);
        vec2 Q = lissajous(t + endpointOffset);

        float d = sdSegment(uv, P, Q);
        float seg = smoothstep(lineThickness + GLOW_WIDTH, lineThickness, d);

        float t0 = fi / fTrail;
        float t1 = (fi + 1.0) / fTrail;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        vec3 color = texture(gradientLUT, vec2(t0, 0.5)).rgb;

        float brightness = baseBright + mag;
        result += color * seg * brightness * fadeAccum;

        fadeAccum *= trailFade;
    }

    result *= glowIntensity;
    finalColor = vec4(result, 1.0);
}
