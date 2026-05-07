// Based on "rrotationnal rotation [207ch]" and "irrotationnal rotation [208ch]"
// by FabriceNeyret2
// https://www.shadertoy.com/view/sXf3WH
// https://www.shadertoy.com/view/sXX3WH
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized scaffold (ringSpacing, baseDivisions, ringFrequency,
//   radialDrift, differentialTwist), three coloring modes (SMOOTH/WEDGE/RANDOM)
//   feeding through gradient LUT, per-cell FFT brightness via BAND_SAMPLES=4
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform vec2 resolution;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform int mode;
uniform float ringSpacing;
uniform int baseDivisions;
uniform float ringFrequency;
uniform float radialDrift;
uniform float spinPhase;
uniform float differentialTwist;
uniform float driftPhase;
uniform float wedgeWidth;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const int BAND_SAMPLES = 4;

float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float computeFftMag(float t) {
    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float ts = t + (float(s) + 0.5) / float(BAND_SAMPLES) /
                   float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    return pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    vec2 U = fc - resolution * 0.5;

    float l = length(U) / ringSpacing / resolution.y;
    float ringIdx = round(l);
    float ringSpin = spinPhase * (1.0 + differentialTwist * ringIdx);

    float a = atan(U.y, U.x) * float(baseDivisions) * ringIdx + ringSpin;
    float d = cos(a) * cos(ringFrequency * l + radialDrift);
    float aa = min(abs(d / fwidth(d)), 1.0);

    vec3 col;
    float brightness;

    if (mode == 0) {
        float t = fract(a / (TWO_PI * float(baseDivisions)));
        col = texture(gradientLUT, vec2(t, 0.5)).rgb;
        brightness = baseBright + computeFftMag(t);
    } else if (mode == 1) {
        bool inWedge = abs(mod(a / float(baseDivisions), TWO_PI) - PI) < wedgeWidth;
        if (inWedge) {
            float t = fract(a / (TWO_PI * float(baseDivisions)));
            col = texture(gradientLUT, vec2(t, 0.5)).rgb;
            brightness = baseBright + computeFftMag(t);
        } else {
            col = vec3(1.0);
            brightness = baseBright;
        }
    } else {
        float angularSlot = round(a / TWO_PI * float(baseDivisions) * ringIdx);
        float t = fract(hash2(vec2(ringIdx, angularSlot)) + driftPhase);
        col = texture(gradientLUT, vec2(t, 0.5)).rgb;
        brightness = baseBright + computeFftMag(t);
    }

    finalColor = vec4(col * brightness * aa, 1.0);
}
