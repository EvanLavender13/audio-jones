// Kali-family circuit fractals with FFT-driven layer glow. Three fold modes
// produce distinct trace geometries; winning iteration maps to a frequency
// band whose energy drives brightness. Gradient LUT tints each depth layer.
//
// Based on "Circuits" by Kali
// https://www.shadertoy.com/view/XlX3Rj
// License: MIT
//
// Based on "Circuits II" by Kali
// https://www.shadertoy.com/view/wlBcDK
// License: CC BY-NC-SA 3.0
//
// Based on "Circuits III" by Kali
// https://www.shadertoy.com/view/WlXBWN
// License: CC BY-NC-SA 3.0
//
// Modified: three fractal cores unified under mode uniform; gradient LUT
// replaces procedural/HSV coloring; FFT semitone mapping adds per-layer audio
// reactivity; all constants parameterized as uniforms; pan/flow/rotation
// accumulators replace iTime-based animation; 7x7 grid supersampling.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform int mode;
uniform int iterations;
uniform float zoom;
uniform float clampLo;
uniform float clampHi;
uniform float foldConstant;
uniform float traceWidth;

uniform float panAccum;
uniform float flowAccum;
uniform float flowIntensity;
uniform float rotationAccum;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const int BAND_SAMPLES = 4;
const int AA = 3;

// Per-layer FFT energy: winning iteration -> frequency band
float fftEnergy(int winIt) {
    float t0 = float(winIt) / float(iterations);
    float t1 = float(winIt + 1) / float(iterations);
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0)
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    return pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
}

// Mode 0: Circuits - dot-product inversion, compound axis+ring orbit trap,
// pow-based rendering with depth-scaled width.
vec3 fractalCircuits(vec2 z) {
    float ot = 1000.0, ot2 = 1000.0;
    float minit = 0.0;

    for (int i = 0; i < iterations; i++) {
        z = abs(z) / clamp(dot(z, z), clampLo, clampHi) - foldConstant;
        float l = length(z);
        float o = min(max(abs(min(z.x, z.y)), -l + 0.25), abs(l - 0.25));
        ot = min(ot, o);
        ot2 = min(l * 0.1, ot2);
        minit = max(minit, float(i) * (1.0 - abs(sign(ot - o))));
    }

    minit += 1.0;
    float w = traceWidth * minit * 2.0;
    float circ = pow(max(0.0, w - ot2) / w, 6.0);
    float shape = max(pow(max(0.0, w - ot) / w, 0.25), circ);

    vec3 layerColor = texture(gradientLUT, vec2(minit * 0.1, 0.5)).rgb;
    float colorMod =
        0.4 + mod(minit / float(iterations) - flowAccum + ot2 * 2.0, 1.0) * 1.6;
    vec3 color = layerColor * colorMod;
    color += layerColor * circ * (float(iterations) - minit) * 3.0;

    return mix(vec3(0.0), color, shape) * (baseBright + fftEnergy(int(minit)));
}

// Mode 1: Circuits II - product inversion, abs(p.x) trap with stepping,
// anisotropic fold constant, exp glow.
vec3 fractalCircuitsII(vec2 p) {
    p = fract(p) - 0.5;

    float ot1 = 1000.0, ot2 = 1000.0;
    int winIt = 0;

    for (int i = 0; i < iterations; i++) {
        p = abs(p);
        p = p / clamp(p.x * p.y, clampLo, clampHi) -
            vec2(foldConstant * 1.5, foldConstant);
        float m = abs(p.x);
        m += step(fract(flowAccum * 0.2 + float(i) * 0.05), 0.5 * abs(p.y)) *
             flowIntensity;
        if (m < ot1) {
            ot1 = m;
            winIt = i;
        }
        ot2 = min(ot2, length(p));
    }

    float tw = traceWidth * 6.6;
    ot1 = exp(-ot1 / max(tw, 0.001));
    ot2 = exp(-ot2 / max(tw, 0.001));

    vec3 layerColor = texture(gradientLUT,
        vec2((float(winIt) + 0.5) / float(iterations), 0.5)).rgb;
    return (layerColor * ot1 + ot2) * (baseBright + fftEnergy(winIt));
}

// Mode 2: Circuits III - 90-degree fold rotation, box-distance junctions,
// fract tiling, exp trace + smoothstep junction.
vec3 fractalCircuitsIII(vec2 p) {
    p += flowAccum * 0.02;
    p = fract(p);

    float o = 100.0, l = 100.0;
    int winIt = 0;
    float bestO = 100.0;

    for (int i = 0; i < iterations; i++) {
        p = vec2(p.y, -p.x);
        p.y = abs(p.y - 0.25);
        p = p / clamp(abs(p.x * p.y), clampLo, clampHi) - foldConstant;

        float trapDist = abs(p.y - 1.5) - 0.2;
        o = min(o, trapDist) +
            fract(p.x + flowAccum + float(i) * 0.2) * flowIntensity;

        if (trapDist < bestO) {
            bestO = trapDist;
            winIt = i;
        }

        l = min(l, length(max(vec2(0.0), abs(p) - 0.5)));
    }

    o = exp(-5.0 * o);
    l = smoothstep(0.1, 0.11, l);

    vec3 traceColor = texture(gradientLUT, vec2(o * 0.5, 0.5)).rgb;
    vec3 junctionColor = texture(gradientLUT, vec2(0.8, 0.5)).rgb;
    return (traceColor * o + l * junctionColor * 0.3) * (baseBright + fftEnergy(winIt));
}

void main() {
    // Centered, aspect-corrected coordinates
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

    // Global rotation
    float cr = cos(rotationAccum), sr = sin(rotationAccum);
    uv *= mat2(cr, -sr, sr, cr);

    // Pan
    uv.y += panAccum;

    // Zoom
    uv *= zoom;

    // Supersampling AA (7x7 = 49 samples)
    vec2 pixelSize = 1.0 / resolution / float(AA * 2);

    vec3 col = vec3(0.0);
    for (int i = -AA; i <= AA; i++) {
        for (int j = -AA; j <= AA; j++) {
            vec2 p = uv + vec2(float(i), float(j)) * pixelSize;

            if (mode == 0) {
                col += fractalCircuits(p);
            } else if (mode == 1) {
                col += fractalCircuitsII(p);
            } else {
                col += fractalCircuitsIII(p);
            }
        }
    }

    float totalSamples = float((AA * 2 + 1) * (AA * 2 + 1));
    col /= totalSamples;

    finalColor = vec4(col, 1.0);
}
