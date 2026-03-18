// Based on "Vortex [265]" by Xor
// https://www.shadertoy.com/view/wctXWN
// License: CC BY-NC-SA 3.0
// Modified: ported to GLSL 330; parameterized march steps, turbulence,
// sphere radius, surface detail, camera distance; replaced cos() coloring
// with gradient LUT; added FFT audio reactivity; added brightness tonemap
// control.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform int marchSteps;
uniform int turbulenceOctaves;
uniform float turbulenceGrowth;
uniform float sphereRadius;
uniform float surfaceDetail;
uniform float cameraDistance;
uniform float rotationAngle;     // CPU-accumulated Y-axis spin
uniform float colorPhase;        // CPU-accumulated color scroll
uniform float colorStretch;
uniform float brightness;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    vec3 rayDir = normalize(vec3(fragCoord * 2.0, 0.0) - vec3(resolution.x, resolution.y, resolution.y));

    // Dithered ray start - suppresses banding
    float z = fract(dot(fragCoord, sin(fragCoord)));
    float d;
    vec3 color = vec3(0.0);
    float stepCount = float(max(marchSteps - 1, 1));

    for (int i = 0; i < marchSteps; i++) {
        vec3 p = z * rayDir;
        p.z += cameraDistance;

        // Y-axis rotation - reveals different faces of the turbulence volume
        float cs = cos(rotationAngle);
        float sn = sin(rotationAngle);
        p.xz = mat2(cs, -sn, sn, cs) * p.xz;

        // Turbulence loop - geometric frequency scaling
        d = 1.0;
        for (int j = 0; j < turbulenceOctaves; j++) {
            p += cos(p.yzx * d - time) / d;
            d /= turbulenceGrowth;
        }

        // Hollow sphere distance in warped space
        d = 0.002 + abs(length(p) - sphereRadius) / surfaceDetail;
        z += d;

        // Color from gradient LUT
        float lutCoord = fract(z * colorStretch + colorPhase);
        vec3 stepColor = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

        // Per-step FFT - map march step to frequency band in log space
        // Same pattern as Muons additive volume mode
        float t0 = float(i) / stepCount;
        float t1 = float(i + 1) / stepCount;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);
        const int BAND_SAMPLES = 4;
        float energy = 0.0;
        for (int bs = 0; bs < BAND_SAMPLES; bs++) {
            float bin = mix(binLo, binHi, (float(bs) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float audio = baseBright + energy;

        // Additive accumulation - glow inversely proportional to step distance
        color += stepColor * audio / d;
    }

    // Tanh tonemapping - brightness folds into divisor
    color = tanh(color * brightness / 7000.0);

    finalColor = vec4(color, 1.0);
}
