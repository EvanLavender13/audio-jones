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

float fftAt(float t) {
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float mag = 0.0;
    if (bin <= 1.0) { mag = texture(fftTexture, vec2(bin, 0.5)).r; }
    return pow(clamp(mag * gain, 0.0, 1.0), curve);
}

vec3 speciesColor(float v, float sliceOffset, float hue) {
    float t = sliceOffset + hue * (1.0 / 3.0);
    vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;
    float mag = fftAt(t);
    return col * v * (baseBright + mag);
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 s0 = texture(stateTex0, uv);
    vec4 s1 = texture(stateTex1, uv);
    float v0 = s0.y;
    float v1 = s0.w;
    float v2 = s1.y;
    float hue = s1.b;

    const float SLICE = 1.0 / 3.0;
    vec3 col0 = speciesColor(v0, 0.0,         hue);
    vec3 col1 = speciesColor(v1, SLICE,       hue);
    vec3 col2 = speciesColor(v2, 2.0 * SLICE, hue);

    vec3 col = brightness * (col0 + col1 + col2);
    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
