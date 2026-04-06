// Based on "Spiral of Spirals 2" by KilledByAPixel (Frank Force)
// https://www.shadertoy.com/view/lsdBzX
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced HSV coloring with gradient LUT, removed mouse interaction,
// added FFT audio reactivity, configurable zoom and animation speed

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float zoom;
uniform float timeAccum;
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float PI = 3.14159265359;

void main() {
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.x;
    uv *= zoom;

    float a = atan(uv.y, uv.x);
    float d = length(uv);

    // Archimedean spiral mapping
    float i = d;
    float p = a / (2.0 * PI) + 0.5;
    i -= p;
    a += 2.0 * PI * floor(i);

    float t = timeAccum;

    // Gradient LUT index (repurposed hue computation)
    float h = 0.5 * a;
    h *= t;
    h = 0.5 * (sin(h) + 1.0);
    h = pow(h, 3.0);
    h += 4.222 * t + 0.4;

    // Nested spiral structure
    a *= (floor(i) + p);

    // Brightness pattern
    float v = a;
    v *= t;
    v = sin(v);
    v = 0.5 * (v + 1.0);
    v = pow(v, 4.0);
    v *= pow(sin(fract(i) * PI), 0.4);
    v *= min(d, 1.0);

    // FFT brightness (radial frequency mapping)
    float t_fft = clamp(2.0 * d / zoom, 0.0, 1.0);
    float freq = baseFreq * pow(maxFreq / baseFreq, t_fft);
    float bin = freq / (sampleRate * 0.5);
    float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    // Final composition
    vec3 color = texture(gradientLUT, vec2(fract(h), 0.5)).rgb;
    finalColor = vec4(color * v * brightness * glowIntensity, 1.0);
}
