// Based on "Inkelly" by leon
// https://www.shadertoy.com/view/4fl3Wl
// License: CC BY-NC-SA 3.0 Unported
// Modified: glow-on-black render replacing white-paper background; gradient LUT + FFT brightness (BAND_SAMPLES=4) replacing constant white line color
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // mask field (auto-bound)
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float sampleRate;
uniform float edgeFeather;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    vec2 uv = fragTexCoord;

    vec2 ep = 1.0 / resolution;
    float mr = smoothstep(0.0, edgeFeather, texture(texture0, uv).r);
    float mE = smoothstep(0.0, edgeFeather, texture(texture0, uv + vec2(ep.x, 0.0)).r);
    float mW = smoothstep(0.0, edgeFeather, texture(texture0, uv - vec2(ep.x, 0.0)).r);
    float mN = smoothstep(0.0, edgeFeather, texture(texture0, uv + vec2(0.0, ep.y)).r);
    float mS = smoothstep(0.0, edgeFeather, texture(texture0, uv - vec2(0.0, ep.y)).r);
    float edge = abs(mE - mr) + abs(mW - mr) + abs(mN - mr) + abs(mS - mr);
    edge = clamp(edge / 2.0, 0.0, 1.0);

    float t = texture(texture0, uv).r;
    vec3 lineColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float ts = t + (float(s) + 0.5) / float(BAND_SAMPLES) /
                   float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 result = lineColor * brightness * edge;
    finalColor = vec4(result, 1.0);
}
