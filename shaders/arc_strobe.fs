// Arc Strobe: Glowing line segments on a Lissajous curve with sequential strobe
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

uniform float strobeSpeed;
uniform float strobeTime;
uniform float strobeDecay;
uniform float strobeBoost;
uniform int strobeStride;

uniform float baseFreq;
uniform int numOctaves;
uniform int segmentsPerOctave;
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
    int totalSegments = segmentsPerOctave * numOctaves;
    float fTotal = float(totalSegments);

    for (int i = 0; i < totalSegments; i++) {
        float fi = float(i);
        // Even spacing along one lap — consecutive segments are adjacent on
        // the curve, so the strobe creates a traveling bright arc.
        float t_i = TWO_PI * fi / fTotal;

        vec2 P = lissajous(t_i);
        vec2 Q = lissajous(t_i + orbitOffset);

        // Lorentzian glow: tight falloff, 1/d^2 tail prevents background haze
        float d = sdSegment(uv, P, Q) - lineThickness;
        float d2 = d * d;
        float glow = GLOW_WIDTH * GLOW_WIDTH / (GLOW_WIDTH * GLOW_WIDTH + d2);

        // Sequential strobe — skips segments not on stride boundary.
        // When strobeSpeed is 0, strobe is fully off (no boost applied).
        float strobeEnv = 0.0;
        if (strobeSpeed > 0.0 && i % strobeStride == 0) {
            float sc = fi / fTotal;
            strobeEnv = exp(-strobeDecay * fract(strobeTime + sc));
        }

        // FFT semitone lookup — contiguous octaves along the curve so
        // frequency bands light coherent arcs (bass = one region, treble = another)
        float semitoneF = fi * 12.0 / float(segmentsPerOctave);
        int semitone = int(floor(semitoneF));
        float freq = baseFreq * pow(2.0, float(semitone) / 12.0);
        float bin = freq / (sampleRate * 0.5);
        float mag = 0.0;
        if (bin <= 1.0) {
            mag = texture(fftTexture, vec2(bin, 0.5)).r;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
        }

        // Color from gradient LUT by pitch class
        vec3 color = texture(gradientLUT, vec2(fract(semitoneF / 12.0), 0.5)).rgb;

        // baseBright: always-on ember. mag: always-on FFT. strobe: additive sweep.
        float brightness = baseBright + mag + strobeEnv * strobeBoost;
        result += color * glow * glowIntensity * brightness;
    }

    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
