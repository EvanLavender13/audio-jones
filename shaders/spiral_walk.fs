// Based on "Kungs vs. Cookin" by jorge2017a3
// https://www.shadertoy.com/view/sfl3zr
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced time-based angle morphing with uniform-driven rotation,
// rainbow HSL with gradient LUT, hardcoded glow with filaments-style
// inverse-distance glow, added FFT frequency band mapping per segment

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform int segments;
uniform float growthRate;
uniform float angleOffset;
uniform float rotationAccum;
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Point-to-segment distance (IQ sdSegment)
float segm(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 r = resolution;
    vec2 uv = (fragTexCoord * r * 2.0 - r) / min(r.x, r.y);

    // 2 pixels in the same R.y-normalized units as uv
    float glowWidth = 2.0 / r.y;

    vec3 result = vec3(0.0);

    vec2 prev = vec2(0.0);
    float segLen = 0.0;

    for (int n = 0; n < segments; n++) {
        float angle = float(n) * (angleOffset + rotationAccum);
        segLen += growthRate;
        vec2 next = prev + vec2(cos(angle), sin(angle)) * segLen;

        // FFT band-averaging lookup (log-spaced frequency spread)
        float t0 = float(n) / float(segments);
        float t1 = float(n + 1) / float(segments);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        // Inverse-distance glow (filaments pattern)
        float dist = segm(uv, prev, next);
        float glow = glowWidth / (glowWidth + dist);

        // Gradient LUT color mapped along chain
        float colorT = float(n) / float(segments);
        vec3 color = texture(gradientLUT, vec2(colorT, 0.5)).rgb;

        result += color * glow * glowIntensity * (baseBright + mag);

        prev = next;
    }

    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
