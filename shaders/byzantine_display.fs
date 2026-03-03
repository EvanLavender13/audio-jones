// Based on "Byzantine Buffering" by paniq
// https://www.shadertoy.com/view/7tBGz1
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced RGB offset-zoom colorization with gradient LUT,
//           added FFT audio reactivity, removed barrel distortion

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Sim buffer (current read)
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform vec2 resolution;
uniform int frameCount;
uniform int cycleLength;
uniform float zoomAmount;
uniform vec2 center;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    // Counter-zoom: smooth compensation for discrete reseed
    // scale goes from 1.0 (just after reseed) to 1/zoomAmount (just before next)
    float m = float(frameCount % cycleLength) / float(cycleLength);
    float scale = pow(zoomAmount, -m);

    // Apply zoom with aspect-correct isotropic scaling
    vec2 centered = fragTexCoord - center;
    centered.x *= resolution.x / resolution.y;
    centered *= scale;
    centered.x /= resolution.x / resolution.y;
    vec2 uv = centered + center;

    // Sample simulation field [-1, 1] -> t [0, 1]
    float field = texture(texture0, uv).r;
    float t = field * 0.5 + 0.5;

    // Gradient LUT color
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

    // FFT audio reactivity: map t to frequency, look up energy
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) {
        energy = texture(fftTexture, vec2(bin, 0.5)).r;
        energy = pow(clamp(energy * gain, 0.0, 1.0), curve);
    }
    float brightness = baseBright + energy;

    finalColor = vec4(color * brightness, 1.0);
}
