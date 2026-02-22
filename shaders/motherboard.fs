// Motherboard: Kali inversion fractal with dual orbit traps and FFT-driven layer glow.
// Hyperbolic fold iterations create recursive neon traces; winning iteration maps to a
// frequency band whose energy drives brightness. Gradient LUT tints each depth layer.
#version 430

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

uniform int iterations;
uniform float zoom;
uniform float clampLo;
uniform float clampHi;
uniform float foldConstant;
uniform float rotAngle;

uniform float panAccum;
uniform float flowAccum;
uniform float flowIntensity;
uniform float rotationAccum;

uniform float glowIntensity;
uniform float accentIntensity;

uniform sampler2D gradientLUT;

const int BAND_SAMPLES = 4;

void main() {
    // Coordinate setup: center, aspect-correct
    vec2 p = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

    // Rotate by accumulated angle
    float cr = cos(rotationAccum), sr = sin(rotationAccum);
    p *= mat2(cr, -sr, sr, cr);

    // Pan (Y-axis drift; rotation above controls effective direction)
    p.y += panAccum;

    // Scale + infinite tile
    p *= zoom;
    p = fract(p) - 0.5;

    // Kali inversion iteration with dual orbit traps
    float cf = cos(rotAngle), sf = sin(rotAngle);
    mat2 foldRot = mat2(cf, -sf, sf, cf);

    float ot1 = 1000.0, ot2 = 1000.0;
    int minit = 0;

    for (int i = 0; i < iterations; i++) {
        p = abs(p);
        p = p / clamp(abs(p.x * p.y), clampLo, clampHi) - foldConstant;
        p *= foldRot;

        // Primary trap: trace distance (determines winning layer)
        float m = abs(p.x);
        // Data streaming modulation
        m += fract(p.x + flowAccum + float(i) * 0.2) * flowIntensity;
        if (m < ot1) {
            ot1 = m;
            minit = i;
        }

        // Secondary trap: junction distance
        ot2 = min(ot2, length(p));
    }

    // Rendering via exp() glow
    float trace = exp(-ot1 / max(glowIntensity, 0.001));

    float junction = 0.0;
    if (accentIntensity > 0.0) {
        junction = exp(-ot2 / accentIntensity);
    }

    // FFT mapping: winning iteration -> frequency band
    float t0 = float(minit) / float(iterations);
    float t1 = float(minit + 1) / float(iterations);
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + energy;

    // Final color composite
    vec3 layerColor = texture(gradientLUT, vec2((float(minit) + 0.5) / float(iterations), 0.5)).rgb;
    vec3 color = trace * layerColor * brightness + junction;
    finalColor = vec4(color, 1.0);
}
