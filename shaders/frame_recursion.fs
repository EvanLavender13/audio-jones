// Based on "Icosahedron frame" by gaz
// https://www.shadertoy.com/view/st2GRd
// License: CC BY-NC-SA 3.0 Unported
// Modified: per-axis CPU-accumulated rotation, shape selector for 3 platonic
//           solids, gradient LUT replaces cosine palette, FFT brightness on
//           shared depth-cycle t index
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform vec3 rotateAngle;  // CPU-accumulated per-axis (rad)
uniform float zoomPhase;   // CPU-accumulated tunnel zoom phase
uniform int shape;         // 0=Octa, 1=Dodeca, 2=Icosa
uniform float edgeRadius;
uniform float glowIntensity;
uniform int marchSteps;
uniform int colorMode;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define R(p, a, r) (mix(a * dot(p, a), p, cos(r)) + sin(r) * cross(p, a))

float sampleFFTBand(float t0, float t1) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int b = 0; b < BAND_SAMPLES; b++) {
        float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return baseBright + mag;
}

void main() {
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;
    vec3 d = normalize(vec3(uv, 1.0));

    // Per-shape fold normal and fold count (verified gaz-family values)
    vec3 n;
    int foldCount;
    if (shape == 0) {
        n = vec3(-0.5, -0.7071068, 0.5);
        foldCount = 4;
    } else if (shape == 1) {
        n = vec3(-0.5, -0.809, 0.309);
        foldCount = 5;
    } else {
        n = vec3(-0.5, -0.809, 0.309);
        foldCount = 5;
    }

    vec3 accumulator = vec3(0.0);
    vec3 p;
    float g = 0.0;
    float e = 0.0;
    const float intensity = 0.05;
    const float falloff = 0.05;

    for (int i = 0; i < marchSteps; i++) {
        p = g * d;
        p.z -= 10.0;
        p = R(p, vec3(1.0, 0.0, 0.0), rotateAngle.x);
        p = R(p, vec3(0.0, 1.0, 0.0), rotateAngle.y);
        p = R(p, vec3(0.0, 0.0, 1.0), rotateAngle.z);

        for (int j = 0; j < foldCount; j++) {
            p.xy = abs(p.xy);
            p -= 2.0 * min(0.0, dot(p, n)) * n;
        }
        p.z = fract(log(p.z) - zoomPhase) - 0.5;

        if (shape == 0) {
            e = abs(min(length(p.yz), length(p.xz)) - edgeRadius) + 0.001;
        } else if (shape == 1) {
            e = length(p.xz) - edgeRadius;
        } else {
            e = length(p.yz) - edgeRadius;
        }
        g += e;

        float fi = float(i + 1);
        float t;
        if (colorMode == 1) {
            t = fi / float(marchSteps);
        } else if (colorMode == 2) {
            t = fract(log(g + 1.0));
        } else {
            t = fract(dot(p, p) * 0.5);
        }
        vec3 baseColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
        float brightness = sampleFFTBand(t, t + 1.0 / float(marchSteps));

        accumulator += baseColor * brightness * intensity * exp(-falloff * fi * fi * e);
    }

    finalColor = vec4(accumulator * glowIntensity, 1.0);
}
