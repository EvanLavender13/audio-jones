// Based on "大龙猫 - Quicky#031" by totetmatt
// https://www.shadertoy.com/view/3sXyDS
// License: CC BY-NC-SA 3.0 Unported
// Modified: FFT-driven brightness, gradient LUT coloring, orbital motion,
// bias-controlled rotation differential, configurable sweep strobe
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform int layers;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float rotationAccum;
uniform float rotationBias;
uniform float orbitRadius;
uniform float orbitBias;
uniform float orbitAccum;
uniform float sizeMin;
uniform float sizeMax;
uniform float aspectRatio;
uniform float glowWidth;
uniform float glowIntensity;
uniform float sweepAccum;
uniform float sweepIntensity;
uniform sampler2D gradientLUT;

const float TAU = 6.28318530718;

float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdEquilateralTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r / k;
    if (p.x + k * p.y > 0.0) p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    vec2 uv0 = (fc - 0.5 * resolution) / min(resolution.x, resolution.y);

    vec3 total = vec3(0.0);

    float freqRatio = maxFreq / baseFreq;

    for (int i = 0; i < layers; i++) {
        float t = float(i) / float(layers);

        // Per-layer rotation: bias controls speed spread, all layers always rotate
        // bias=0: uniform speed. bias=+1: inner 0.5x, outer 1.5x. bias=-1: reversed.
        float differential = rotationBias * (t - 0.5);
        float angle = (1.0 + differential) * rotationAccum;
        float ca = cos(angle), sa = sin(angle);
        vec2 uv = vec2(uv0.x * ca - uv0.y * sa, uv0.x * sa + uv0.y * ca);

        // Orbital offset: bias +1=outer wide, 0=all same, -1=inner wide
        float orbT = orbitBias >= 0.0 ? t : 1.0 - t;
        float orbWeight = mix(1.0, orbT, abs(orbitBias));
        uv += vec2(sin(orbitAccum), cos(orbitAccum)) * orbitRadius * orbWeight;

        // Per-layer size
        float size = mix(sizeMin, sizeMax, t);

        // SDF: even layers = box, odd layers = equilateral triangle
        float sdf;
        if (i % 2 == 0) {
            sdf = sdBox(uv, vec2(size * aspectRatio, size));
        } else {
            sdf = sdEquilateralTriangle(uv, size);
        }

        // Raw SDF distance — no solid band, glow alone creates line width
        float d = abs(sdf);

        // Reciprocal glow with cutoff — tight hot core, no screen-wide haze
        float glow = glowWidth / (glowWidth + d) * glowIntensity;
        glow *= smoothstep(glowWidth * 30.0, 0.0, d);

        // Sweep boost — bright pulse at sweep front, fades over 1/3 of cycle
        float sweepPhase = fract(sweepAccum + t);
        float sweepBoost = sweepIntensity * pow(max(1.0 - sweepPhase * 3.0, 0.0), 2.0);

        // FFT frequency band — spread across full spectrum in log space
        float t0 = float(i) / float(layers);
        float t1 = float(i + 1) / float(layers);
        float freqLo = baseFreq * pow(freqRatio, t0);
        float freqHi = baseFreq * pow(freqRatio, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float mag = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                mag += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        mag = pow(clamp(mag / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        // Color from gradient LUT by normalized position (low freq → high freq)
        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // Composite: glow * brightness * sweep
        total += color * glow * (baseBright + mag) * (1.0 + sweepBoost);
    }

    // tanh tonemap — preserves color saturation at high brightness
    total = tanh(total);

    finalColor = vec4(total, 1.0);
}
