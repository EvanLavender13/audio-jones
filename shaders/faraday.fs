#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // ping-pong read buffer
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float time;
uniform int waveCount;           // int, 1-6
uniform float spatialScale;
uniform float visualGain;
uniform float rotationOffset;    // rotationAngle + rotationAccum
uniform int layers;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform int diffusionScale;
uniform float decayFactor;
uniform int waveSource;
uniform int waveShape;
uniform float waveFreq;

const float PI = 3.14159265;
const float TAU = 6.283185307;
const int BAND_SAMPLES = 4;

// Periodic waveform: 0=sine, 1=triangle, 2=sawtooth, 3=square
float wave(float x, int shape) {
    if (shape == 0) return sin(x);
    float p = fract(x / TAU);
    if (shape == 1) return (p < 0.5) ? p * 4.0 - 1.0 : 3.0 - p * 4.0;
    if (shape == 2) return p * 2.0 - 1.0;
    return (p < 0.5) ? 1.0 : -1.0;
}

// Evaluate lattice superposition for integer wave count N at position p
// with wave number k and rotation offset rot
float lattice(vec2 p, float k, int N, float rot) {
    float h = 0.0;
    for (int i = 0; i < N; i++) {
        float theta = float(i) * PI / float(N) + rot;
        vec2 dir = vec2(cos(theta), sin(theta));
        h += cos(dot(dir, p) * k);
    }
    return h / float(N);
}

// Parametric lattice: selectable wave shape replaces cos()
float latticeWave(vec2 p, float k, int N, float rot, int shape) {
    float h = 0.0;
    for (int i = 0; i < N; i++) {
        float theta = float(i) * PI / float(N) + rot;
        vec2 dir = vec2(cos(theta), sin(theta));
        h += wave(dot(dir, p) * k, shape);
    }
    return h / float(N);
}

void main() {
    float nyquist = sampleRate * 0.5;
    float aspect = resolution.x / resolution.y;
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= aspect;

    float totalHeight = 0.0;
    float totalWeight = 0.0;

    for (int i = 0; i < layers; i++) {
        float t0 = (layers > 1) ? float(i) / float(layers - 1) : 0.5;
        float freq = baseFreq * pow(maxFreq / baseFreq, t0);
        float k = freq * spatialScale;

        float mag;
        float layerHeight;

        if (waveSource == 0) {
            // === Audio mode: existing FFT-driven behavior (unchanged) ===
            float t1 = float(i + 1) / float(layers);
            float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
            float binLo = freq / nyquist;
            float binHi = freqHi / nyquist;

            float energy = 0.0;
            for (int s = 0; s < BAND_SAMPLES; s++) {
                float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
                if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
            mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
            if (mag < 0.001) continue;

            layerHeight = lattice(uv, k, waveCount, rotationOffset);
            layerHeight *= cos(time * freq * 0.5);
        } else {
            // === Parametric mode: uniform amplitude, selectable wave shape ===
            mag = 1.0;
            layerHeight = latticeWave(uv, k, waveCount, rotationOffset, waveShape);
            layerHeight *= wave(time * waveFreq * 0.5, waveShape);
        }

        totalHeight += mag * layerHeight;
        totalWeight += mag;
    }

    if (totalWeight > 0.0) totalHeight /= totalWeight;

    // Visualization: tanh compression + gradient LUT
    float compressed = tanh(totalHeight * visualGain);
    float t = abs(compressed);
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
    float brightness = abs(compressed);
    vec4 newColor = vec4(color * brightness, brightness);

    // Trail persistence: diffuse + decay, keep brighter of old/new
    // (Same 5-tap separable Gaussian as Chladni)
    vec4 existing;
    if (diffusionScale == 0) {
        existing = texture(texture0, fragTexCoord);
    } else {
        vec2 texel = vec2(float(diffusionScale)) / resolution;

        vec4 h = texture(texture0, fragTexCoord + vec2(-2.0 * texel.x, 0.0)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(-1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2( 1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord + vec2( 2.0 * texel.x, 0.0)) * 0.0625;

        vec4 v = texture(texture0, fragTexCoord + vec2(0.0, -2.0 * texel.y)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(0.0, -1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2(0.0,  1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord + vec2(0.0,  2.0 * texel.y)) * 0.0625;

        existing = (h + v) * 0.5;
    }
    existing *= decayFactor;
    finalColor = max(existing, newColor);
}
