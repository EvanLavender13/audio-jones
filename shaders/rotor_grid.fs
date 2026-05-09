// Based on "rrotationnal rotation [207ch]" and "irrotationnal rotation [208ch]"
// by FabriceNeyret2
// https://www.shadertoy.com/view/sXf3WH
// https://www.shadertoy.com/view/sXX3WH
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized scaffold (ringSpacing, baseDivisions, ringFrequency,
//   radialDrift, differentialTwist); per-cell discrete coloring via cell IDs
//   (angIdx, radIdx); density slider gates lit fraction; randomness slider
//   blends ordered rainbow t with per-cell hash t; per-cell FFT brightness
//   via BAND_SAMPLES=4
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
uniform float density;
uniform float randomness;
uniform float ringSpacing;
uniform int baseDivisions;
uniform float ringFrequency;
uniform float spinPhase;
uniform float twistPhase;
uniform float driftPhase;

const float PI = 3.14159265359;
const int BAND_SAMPLES = 4;

float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float computeFftMag(float t) {
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float baseBin = freq / (sampleRate * 0.5);
    float binStep = 1.0 / float(textureSize(fftTexture, 0).x);

    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = baseBin + (float(s) - 1.5) * binStep;
        if (bin >= 0.0 && bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    return pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    vec2 U = fc - resolution * 0.5;

    float l = length(U) / ringSpacing / resolution.y;

    // Discrete radial cell index, aligned with cos(ringFrequency*l) lobes
    float radIdx = round(ringFrequency * l / PI);
    float ringIdx = max(radIdx, 1.0);

    float ringSpin = spinPhase + twistPhase * ringIdx * ringIdx;
    float a = atan(U.y, U.x) * float(baseDivisions) * ringIdx + ringSpin;

    float d = cos(a) * cos(ringFrequency * l);
    float aa = min(abs(d / fwidth(d)), 1.0);

    // Wrap modulo cells-per-ring so cells straddling the atan() branch cut
    // get one cell ID instead of two halves.
    float cellsPerRing = 2.0 * float(baseDivisions) * ringIdx;
    float angIdx = mod(round(a / PI), cellsPerRing);

    // Per-cell hash drives both lit/unlit decision and random color
    float cellHash = hash2(vec2(angIdx, radIdx));
    bool lit = cellHash < density;

    vec3 col;
    float brightness;

    if (lit) {
        // Ordered t: position around the ring (one rainbow per ringIdx revolutions per ring)
        float t_ordered = fract(angIdx / (2.0 * float(baseDivisions)));
        // Random t: per-cell hash drifted by driftPhase for shimmer
        float t_random = fract(cellHash + driftPhase);
        float t = fract(mix(t_ordered, t_random, randomness));

        col = texture(gradientLUT, vec2(t, 0.5)).rgb;
        brightness = baseBright + computeFftMag(t);
    } else {
        col = vec3(1.0);
        brightness = baseBright;
    }

    finalColor = vec4(col * brightness * aa, 1.0);
}
