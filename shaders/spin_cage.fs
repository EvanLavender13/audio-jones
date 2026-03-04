// Based on "cube!" by catson
// https://www.shadertoy.com/view/3XcBDl
// License: CC BY-NC-SA 3.0 Unported
// Modified: All 5 platonic solids, CPU-side projection, FFT reactivity, gradient color

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec4 edges[30];      // Projected edge endpoints (a.x, a.y, b.x, b.y)
uniform float edgeT[30];     // Per-edge t value (0-1) for color + FFT
uniform int edgeCount;

uniform float lineWidth;
uniform float glowIntensity;

uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Line segment distance (verbatim from catson reference)
float lineDist(vec2 p, vec2 a, vec2 b) {
    vec2 ba = b - a;
    vec2 pa = p - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba);
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;

    vec3 result = vec3(0.0);
    float pixelWidth = lineWidth / resolution.y;

    for (int i = 0; i < edgeCount; i++) {
        vec2 a = edges[i].xy;
        vec2 b = edges[i].zw;
        float dist = lineDist(uv, a, b);

        // Glow falloff (Filaments-style inverse distance)
        float glow = pixelWidth / (pixelWidth + dist);

        // Color from gradient LUT
        float t = edgeT[i];
        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // FFT brightness: log-space frequency from t
        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float fftMag = 0.0;
        if (bin <= 1.0) {
            fftMag = texture(fftTexture, vec2(bin, 0.5)).r;
        }
        float brightness = baseBright + pow(clamp(fftMag * gain, 0.0, 1.0), curve);

        result += color * glow * glowIntensity * brightness;
    }

    result = tanh(result);  // Soft clamp to prevent blowout
    finalColor = vec4(result, 1.0);
}
