// Wave Warp: Multi-mode wave-basis fBM domain warp
// Based on "Filaments" by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/4lcSWs
// License: CC BY-NC-SA 3.0 Unported
// Modified: Replaced hardcoded triangle wave with selectable wave basis
// (triangle, sine, sawtooth, square); parameterized octave count and initial
// scale; renamed uniforms for consistency with effect config.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float strength; // Displacement magnitude (0.0-1.0)
uniform float time;     // Crease rotation angle (accumulated on CPU)
uniform int waveType;   // 0=triangle, 1=sine, 2=sawtooth, 3=square
uniform int octaves;    // fBM loop iterations
uniform float scale;    // Initial noise frequency

float wave(float x) {
    if (waveType == 0) return abs(fract(x) - 0.5);           // triangle
    if (waveType == 1) return sin(x * 6.283185) * 0.5 + 0.5; // sine
    if (waveType == 2) return fract(x);                        // sawtooth
    return step(0.5, fract(x));                                // square
}

vec2 wave2(vec2 p) {
    return vec2(wave(p.x + wave(p.y * 2.0)), wave(p.y + wave(p.x * 2.0)));
}

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, s, -s, c);
}

float waveNoise(vec2 p) {
    float z  = 1.5;
    float z2 = 1.5;
    float rz = 0.0;
    vec2  bp = p * scale;
    for (int i = 0; i < octaves; i++) {
        vec2 dg = wave2(bp * 2.0) * 0.5;
        dg *= rot(time);
        p  += dg / z2;
        bp *= 1.5;
        z2 *= 0.6;
        z  *= 1.7;
        p  *= 1.2;
        p  *= mat2(0.970, 0.242, -0.242, 0.970);
        rz += wave(p.x + wave(p.y)) / z;
    }
    return rz;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p  = (uv - 0.5) * 2.0;

    float nz = clamp(waveNoise(p), 0.0, 1.0);
    p *= 1.0 + (nz - 0.5) * strength;

    uv = p * 0.5 + 0.5;
    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
