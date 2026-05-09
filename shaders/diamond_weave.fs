// Based on "Pixel Weave" by TekF
// https://www.shadertoy.com/view/XdS3RK
// Itself derived from "no title" by gaz (https://www.shadertoy.com/view/Md2Gzy)
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT replaces HSV, FFT BAND_SAMPLES brightness indexed by
// ring value, rotation and differential twist split into independent
// CPU-accumulated rates

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float phaseAccum;
uniform float rotationAccum;
uniform float twistAccum;
uniform float driftAccum;

uniform float cellSize;
uniform float baseAngle;
uniform float glowIntensity;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

float shape(vec2 p) {
    return abs(p.x) + abs(p.y) - 1.0;
}

void main() {
    vec2 pos = fragTexCoord * resolution - resolution * 0.5;

    float radial = length(pos / resolution.y);
    float a = baseAngle + rotationAccum + twistAccum * radial * radial;
    pos = pos * cos(a) + vec2(pos.y, -pos.x) * sin(a);

    pos = mod(pos / cellSize, 2.0) - 1.0;

    float h = abs(sin(phaseAccum * shape(3.0 * pos)));
    float c = 0.05 / max(h, 1e-4);

    float freq = baseFreq * pow(maxFreq / baseFreq, h);
    float baseBin = freq / (sampleRate * 0.5);
    float binStep = 1.0 / float(textureSize(fftTexture, 0).x);

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = baseBin + (float(s) - 1.5) * binStep;
        if (bin >= 0.0 && bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 color = texture(gradientLUT, vec2(fract(driftAccum + h), 0.5)).rgb;
    finalColor = vec4(color * (c * 0.5 + 0.5) * brightness * glowIntensity, 1.0);
}
