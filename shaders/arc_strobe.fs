// Arc Strobe: Glowing line segments on a Lissajous curve with sequential strobe
// gating and per-segment FFT band brightness.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float phase;

uniform float amplitude;
uniform float orbitOffset;
uniform float lineThickness;

uniform float freqX1;
uniform float freqY1;
uniform float freqX2;
uniform float freqY2;
uniform float offsetX2;
uniform float offsetY2;

uniform float glowIntensity;

uniform float strobeSpeed;
uniform float strobeTime;
uniform float strobeDecay;
uniform float strobeBoost;
uniform int strobeStride;

uniform float baseFreq;
uniform float maxFreq;
uniform int layers;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float TWO_PI = 6.28318530718;
const float GLOW_WIDTH = 0.002; // Fixed tight falloff (~1px at 1080p in this UV space)

// Point-to-segment distance (IQ sdSegment)
float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

// Evaluate dual-Lissajous 2D point at parameter t.
// Freq multiplies t (spatial ratio) — different per axis creates the figure.
// Phase is additive time offset for animation.
vec2 lissajous(float t) {
    float x = sin(freqX1 * t + phase);
    float y = cos(freqY1 * t + phase);

    float nx = 1.0;
    float ny = 1.0;

    if (freqX2 != 0.0) {
        x += sin(freqX2 * t + offsetX2 + phase);
        nx = 2.0;
    }
    if (freqY2 != 0.0) {
        y += cos(freqY2 * t + offsetY2 + phase);
        ny = 2.0;
    }

    float aspect = resolution.x / resolution.y;
    return vec2((x / nx) * amplitude * aspect, (y / ny) * amplitude);
}

void main() {
    vec2 uv = (fragTexCoord * resolution * 2.0 - resolution) / min(resolution.x, resolution.y);

    vec3 result = vec3(0.0);
    int totalSegments = layers;
    float fTotal = float(totalSegments);

    for (int i = 0; i < totalSegments; i++) {
        float fi = float(i);
        // Even spacing along one lap — consecutive segments are adjacent on
        // the curve, so the strobe creates a traveling bright arc.
        float t_i = TWO_PI * fi / fTotal;

        vec2 P = lissajous(t_i);
        vec2 Q = lissajous(t_i + orbitOffset);

        // Reciprocal glow: wide 1/|d| falloff for hot vibrant halos
        float d = sdSegment(uv, P, Q) - lineThickness;
        float glow = GLOW_WIDTH / (GLOW_WIDTH + abs(d));

        // Sequential strobe — skips segments not on stride boundary.
        // When strobeSpeed is 0, strobe is fully off (no boost applied).
        float strobeEnv = 0.0;
        if (strobeSpeed > 0.0 && i % strobeStride == 0) {
            float sc = fi / fTotal;
            strobeEnv = exp(-strobeDecay * fract(strobeTime + sc));
        }

        // FFT band-averaging lookup — layers subdivide baseFreq..maxFreq in log space
        float t0 = float(i) / float(totalSegments);
        float t1 = float(i + 1) / float(totalSegments);
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

        // Color from gradient LUT by normalized position
        vec3 color = texture(gradientLUT, vec2(t0, 0.5)).rgb;

        // baseBright: always-on ember. mag: always-on FFT. strobe: additive sweep.
        float brightness = baseBright + mag + strobeEnv * strobeBoost;
        result += color * glow * glowIntensity * brightness;
    }

    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
