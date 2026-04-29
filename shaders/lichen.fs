// Lichen color output - per-species LUT slice with per-band FFT brightness
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // unused (raylib auto-binds quad source)
uniform sampler2D stateTex0;      // species 0 (rg) + species 1 (ba)
uniform sampler2D stateTex1;      // species 2 (rg)
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform vec2 resolution;
uniform float brightness;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

float bandEnergy(float t0, float t1) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    return pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 s0 = texture(stateTex0, uv);
    vec4 s1 = texture(stateTex1, uv);
    float v0 = s0.y;
    float v1 = s0.w;
    float v2 = s1.y;

    const float SLICE = 1.0 / 3.0;

    float mag0 = bandEnergy(0.0,         SLICE);
    float mag1 = bandEnergy(SLICE,       2.0 * SLICE);
    float mag2 = bandEnergy(2.0 * SLICE, 1.0);

    vec3 col0 = texture(gradientLUT, vec2(0.0         + v0 * SLICE, 0.5)).rgb * v0 * (baseBright + mag0);
    vec3 col1 = texture(gradientLUT, vec2(SLICE       + v1 * SLICE, 0.5)).rgb * v1 * (baseBright + mag1);
    vec3 col2 = texture(gradientLUT, vec2(2.0 * SLICE + v2 * SLICE, 0.5)).rgb * v2 * (baseBright + mag2);

    vec3 col = brightness * (col0 + col1 + col2);
    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
