// Based on "Gltch" by Xor (@XorDev)
// https://www.shadertoy.com/view/mdfGRs
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT coloring, per-cell FFT reactivity, configurable noise/rotation

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform int iterations;
uniform float zoom;
uniform float aberrationSpread;
uniform float noiseScale;
uniform float rotationLevels;
uniform float softness;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Cell noise — hash per integer cell, no interpolation.
// Matches reference's 64x64 nearest-filtered texture exactly.
float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

#define N(x) hash21(floor((x) * 64.0 / noiseScale))

void main() {
    vec2 r = resolution;
    float angleStep = 6.2831853 / rotationLevels;
    float invIter = 1.0 / float(iterations);

    // Stable cell coordinate for FFT mapping (at nominal zoom)
    vec2 cellCoord = (fragTexCoord * 2.0 - 1.0) * vec2(r.x / r.y, 1.0) / zoom;
    cellCoord /= (0.1 + N(cellCoord));
    float cellId = N(cellCoord);

    // Accumulate scalar mask across iterations
    float O = 0.0;
    vec2 c;

    for (int j = 0; j < iterations; j++) {
        float i = float(j) * invIter * 2.0 - 1.0;

        c = (fragTexCoord * 2.0 - 1.0) * vec2(r.x / r.y, 1.0)
            / (zoom + i * aberrationSpread);
        c /= (0.1 + N(c));
        float cosVal = cos((c * mat2(cos(
            ceil(N(c) * rotationLevels) * angleStep
            + vec4(0, 33, 11, 0)))).x /
            N(N(c) + ceil(c) + time));
        // softness=0: hard binary snap (reference look); softness>0: smooth gradient
        float mask = smoothstep(-softness, softness + 0.001, cosVal);
        O += mask * invIter;
    }

    // Per-cell color from gradient using cell's random ID
    vec3 cellColor = texture(gradientLUT, vec2(cellId, 0.5)).rgb;

    // Per-cell FFT brightness
    float freq = baseFreq * pow(maxFreq / baseFreq, cellId);
    float bin = freq / (sampleRate * 0.5);
    float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    finalColor = vec4(cellColor * O * brightness, 1.0);
}
