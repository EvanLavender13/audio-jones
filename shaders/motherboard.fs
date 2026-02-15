// Motherboard: Recursive abs-fold fractal with FFT semitone-driven glow per iteration layer.
// Each fold iteration maps to a musical note; FFT energy at that frequency drives brightness.
// Gradient LUT tints each layer by pitch class.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

uniform int iterations;
uniform float rangeX;
uniform float rangeY;
uniform float size;
uniform float fallOff;
uniform float rotAngle;
uniform float glowIntensity;
uniform float accentIntensity;
uniform float time;
uniform float rotationAccum;

#define TAU 6.28318530718
#define THIN 0.1
#define ACCENT_FREQ 12.0

void main() {
    vec2 p = (fragTexCoord * resolution - resolution * 0.5) / resolution.y * 4.0;

    float a = 1.0;
    vec3 color = vec3(0.0);

    float totalAngle = rotAngle + rotationAccum;
    float c = cos(totalAngle), s = sin(totalAngle);
    mat2 rot = mat2(c, -s, s, c);

    for (int i = 0; i < iterations; i++) {
        p = abs(p) - vec2(rangeX, rangeY) * a;
        p = rot * p;
        p.y = abs(p.y) - rangeY;

        float dist = max(abs(p.x) + a * sin(TAU * float(i) / float(iterations)), p.y - size);

        // Log-space frequency band for this iteration
        float t0 = float(i) / float(iterations);
        float t1 = float(i + 1) / float(iterations);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        // Average energy across the band
        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float brightness = baseBright + energy;

        float glow = smoothstep(THIN, 0.0, dist) * glowIntensity / max(abs(dist), 0.001);
        vec3 layerColor = texture(gradientLUT, vec2((float(i) + 0.5) / float(iterations), 0.5)).rgb;
        color += glow * layerColor * brightness;

        a /= fallOff;
    }

    // Glow accent on fold seams
    if (accentIntensity > 0.0) {
        color += accentIntensity / max(abs(sin(p.y * ACCENT_FREQ + time)), 0.01);
    }

    finalColor = vec4(color, 1.0);
}
