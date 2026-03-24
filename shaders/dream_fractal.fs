// Based on "Dream Within A Dream" by OldEclipse
// https://www.shadertoy.com/view/fclGWs
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, gradient LUT coloring, FFT frequency-band brightness
// Fold operations from Syntopia (http://blog.hvidtfeldts.net/index.php/2011/08/distance-estimated-3d-fractals-iii-folding-space/)
// and cglearn.eu (https://cglearn.eu/pub/advanced-computer-graphics/fractal-rendering)

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float orbitPhase;
uniform float driftPhase;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform int fractalIters;
uniform float carveRadius;
uniform float scaleFactor;
uniform int marchSteps;
uniform float colorScale;
uniform float turbulenceIntensity;
uniform int carveMode;
uniform int foldEnabled;
uniform int foldMode;
uniform vec3 juliaOffset;
#define TURBULENCE_PASSES 3
const int BAND_SAMPLES = 4;

float carveSDF(vec3 q, int mode) {
    if (mode == 0) return length(q);
    if (mode == 1) return max(max(q.x, q.y), q.z);
    if (mode == 2) return min(max(q.y, q.z), min(max(q.x, q.z), max(q.x, q.y)));
    if (mode == 3) return length(q.xz);
    return (q.x + q.y + q.z) * 0.577;
}

void applyFold(inout vec3 p, int mode) {
    if (mode == 0) {
        // Box fold
        p = clamp(p, -1.0, 1.0) * 2.0 - p;
    } else if (mode == 1) {
        // Sierpinski: tetrahedral plane reflections
        if (p.x + p.y < 0.0) p.xy = -p.yx;
        if (p.x + p.z < 0.0) p.xz = -p.zx;
        if (p.y + p.z < 0.0) p.zy = -p.yz;
    } else if (mode == 2) {
        // Menger: abs + descending 3-axis sort
        p = abs(p);
        if (p.x < p.y) p.xy = p.yx;
        if (p.x < p.z) p.xz = p.zx;
        if (p.y < p.z) p.yz = p.zy;
    } else {
        // Burning Ship: abs reflection
        p = abs(p);
    }
}

void main() {
    vec2 I = gl_FragCoord.xy;
    vec3 r = normalize(vec3(I + I, 0.0) - vec3(resolution.xy, resolution.y));

    float co = cos(orbitPhase);
    float so = sin(orbitPhase);
    r.xz *= mat2(co, -so, so, co);

    vec3 p, q;
    float t = 0.0, l = 0.0, d, s;

    int lStep = marchSteps * 2 / 7;
    for (int i = 0; i < marchSteps; i++) {
        p = t * r;
        p.z -= driftPhase;
        q = p * colorScale;

        d = 0.0;
        s = 1.0;
        float ct = 0.8;
        float st = 0.6;
        for (int j = 0; j < fractalIters; j++) {
            // Fold (start of iteration, before scaling)
            if (foldEnabled != 0) {
                applyFold(p, foldMode);
            }

            p *= scaleFactor;
            s *= scaleFactor;

            // Carve with mode selection
            vec3 q = abs(mod(p - 1.0, 2.0) - 1.0);
            d = max(d, (carveRadius - carveSDF(q, carveMode)) / s);

            // Twist rotation
            p.xz *= mat2(ct, st, -st, ct);

            // Julia offset (after twist)
            p += juliaOffset;
        }

        if (i == lStep) {
            l = t;
        }

        t += d;
    }

    const float LUT_FREQ = 0.1;
    vec3 color = vec3(0.0);
    float tColorAvg = 0.0;

    for (int pass = 0; pass < TURBULENCE_PASSES; pass++) {
        float n = -0.2;
        for (int k = 0; k < 9; k++) {
            n += 1.0;
            q += turbulenceIntensity * sin(q.zxy * n) / n;
        }
        float comp = pass == 0 ? q.x : (pass == 1 ? q.y : q.z);
        float tc = 0.5 + 0.5 * sin(comp * LUT_FREQ);
        color += texture(gradientLUT, vec2(tc, 0.5)).rgb;
        tColorAvg += tc;
    }
    float tColor = tColorAvg / 3.0;

    float freqLo = baseFreq * pow(maxFreq / baseFreq, tColor);
    float tHi = min(tColor + 1.0 / 8.0, 1.0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, tHi);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    for (int sb = 0; sb < BAND_SAMPLES; sb++) {
        float bin = mix(binLo, binHi, (float(sb) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    color *= brightness * l / t / 3.0;

    finalColor = vec4(color, 1.0);
}
