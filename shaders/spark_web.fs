// Spark Web: Glowing line segments on a Lissajous curve with sequential strobe
// gating and per-segment FFT semitone brightness.
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
uniform float falloffExponent;

uniform float strobeTime;
uniform float strobeDecay;

uniform float baseFreq;
uniform int numOctaves;
uniform int segmentsPerOctave;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float TWO_PI = 6.28318530718;
const float EPSILON = 0.0001;

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

    return vec2((x / nx) * amplitude, (y / ny) * amplitude);
}

void main() {
    vec2 uv = (fragTexCoord * resolution * 2.0 - resolution) / min(resolution.x, resolution.y);

    vec3 result = vec3(0.0);
    int totalSegments = segmentsPerOctave * numOctaves;
    float fTotal = float(totalSegments);

    for (int i = 0; i < totalSegments; i++) {
        float fi = float(i);
        // Step by 1 radian per segment so the curve wraps many times.
        // Consecutive segments scatter across the figure.
        float t_i = fi;

        vec2 P = lissajous(t_i);
        vec2 Q = lissajous(t_i + orbitOffset);

        // lineThickness defines capsule surface; abs gives bright outline, not solid fill
        float dist = abs(sdSegment(uv, P, Q) - lineThickness);
        float glow = glowIntensity / (pow(dist, 1.0 / falloffExponent) + EPSILON);

        // Sequential strobe
        float sc = fi / fTotal;
        float strobeEnv = exp(-strobeDecay * fract(strobeTime + sc));

        // FFT semitone lookup
        int semitone = int(floor(fi * 12.0 / float(segmentsPerOctave)));
        float freq = baseFreq * pow(2.0, float(semitone) / 12.0);
        float bin = freq / (sampleRate * 0.5);
        float mag = 0.0;
        if (bin <= 1.0) {
            mag = texture(fftTexture, vec2(bin, 0.5)).r;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
        }

        // Color from gradient LUT by pitch class
        float semitoneF = fi * 12.0 / float(segmentsPerOctave);
        vec3 color = texture(gradientLUT, vec2(fract(semitoneF / 12.0), 0.5)).rgb;

        result += color * glow * strobeEnv * (baseBright + mag);
    }

    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
