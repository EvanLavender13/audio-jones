// Based on "VisceraPulsating" by hdrp0720
// https://www.shadertoy.com/view/tfVfRV
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced hardcoded coloring with gradient LUT + FFT audio reactivity, parameterized all constants
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float scale;
uniform float twistAngle;
uniform int iterations;
uniform float freqGrowth;
uniform float warpIntensity;
uniform float pulseAmplitude;
uniform float pulseFreq;
uniform float pulseRadial;
uniform float bumpDepth;
uniform float specPower;
uniform float specIntensity;
uniform float heightScale;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

mat2 rotate(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

float map(vec2 u, float t) {
    vec2 n = vec2(0.0);
    vec2 q = vec2(0.0);
    float d = dot(u, u);
    float s = scale;
    float o = 0.0;
    float j = 0.0;
    mat2 m = rotate(twistAngle);

    for (int i = 0; i < iterations; i++) {
        j += 1.0;
        u = m * u;
        n = m * n;
        q = u * s + t + sin(t * pulseFreq - d * pulseRadial) * pulseAmplitude + j + n;
        o += dot(cos(q) / s, vec2(2.0));
        n -= sin(q) * warpIntensity;
        s *= freqGrowth;
    }

    return o;
}

void main() {
    vec2 uv = (fragTexCoord - 0.5) * vec2(resolution.x / resolution.y, 1.0);

    float h = map(uv, time);

    vec2 e = vec2(2.0 / resolution.y, 0.0);
    float hx = map(uv + e.xy, time);
    float hy = map(uv + e.yx, time);

    vec3 normal = normalize(vec3(h - hx, h - hy, bumpDepth));

    vec3 lightPos = normalize(vec3(0.5, 0.5, 1.0));
    vec3 viewPos = vec3(0.0, 0.0, 1.0);

    float diff = max(dot(normal, lightPos), 0.0);

    vec3 reflectDir = reflect(-lightPos, normal);
    float spec = pow(max(dot(viewPos, reflectDir), 0.0), specPower);

    float t = clamp(h * heightScale / 6.0 + 0.5, 0.0, 1.0);
    vec3 lutColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);

    vec3 specColor = texture(gradientLUT, vec2(1.0, 0.5)).rgb;

    float lit = baseBright + mag;
    vec3 col = lutColor * lit * (0.5 + 0.5 * diff) + specColor * spec * specIntensity * lit;

    finalColor = vec4(col, 1.0);
}
