// Based on "Speak [470]" by Xor
// https://www.shadertoy.com/view/33tSzl
// License: CC BY-NC-SA 3.0
// Modified: ported to GLSL 330; parameterized march steps, turbulence,
// sphere radius, camera distance, outline spread; replaced cos() coloring
// with gradient LUT; replaced audio sphere displacement with per-step FFT
// glow modulation.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform int marchSteps;
uniform int turbulenceOctaves;
uniform int turbulenceMode;
uniform int distMode;
uniform float turbulenceGrowth;
uniform float sphereRadius;
uniform float ringThickness;
uniform float cameraDistance;
uniform vec3 phase;
uniform float outlineSpread;
uniform float colorStretch;
uniform float colorPhase;
uniform float brightness;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    // Ray direction - centered NDC perspective
    vec3 rayDir = normalize(vec3(fragCoord * 2.0, 0.0) - vec3(resolution.x, resolution.y, resolution.y));

    float z = 0.0;
    float d;
    vec3 color = vec3(0.0);
    float stepCount = float(max(marchSteps - 1, 1));

    for (int i = 0; i < marchSteps; i++) {
        // Sample point along ray
        vec3 p = z * rayDir;

        // Per-step rotation axis - outlineSpread creates the wireframe look
        vec3 a = normalize(cos(phase + time + outlineSpread * float(i)));

        // Camera offset
        p.z += cameraDistance;

        // Rodrigues rotation
        a = a * dot(a, p) - cross(a, p);

        // Turbulence - SUBTRACTION and .zxy swizzle (differs from Muons)
        // Turbulence phase uses NEGATIVE outlineSpread (opposite to rotation axis)
        float freq = 0.6;
        for (int oct = 0; oct < turbulenceOctaves; oct++) {
            vec3 v = a * freq + time - outlineSpread * float(i);
            vec3 w;
            switch (turbulenceMode) {
            case 1:  w = sin(v); break;
            case 2:  w = fract(v) * 2.0 - 1.0; break;
            case 3:  w = abs(sin(v)); break;
            case 4:  w = abs(fract(v) * 2.0 - 1.0) * 2.0 - 1.0; break;
            case 5:  w = sin(v) * abs(sin(v)); break;
            case 6:  w = step(0.5, fract(v)) * 2.0 - 1.0; break;
            case 7:  w = floor(v + 0.5); break;
            default: w = cos(v); break;
            }
            a -= w.zxy / freq;
            freq *= turbulenceGrowth;
        }

        // Distance function
        vec3 aa = abs(a);
        switch (distMode) {
        case 1:  d = abs(sin(length(a))); break;
        case 2:  d = abs(a.x-a.y) + abs(a.y-a.z) + abs(a.z-a.x); break;
        case 3:  d = min(length(a.xy), min(length(a.yz), length(a.xz))); break;
        case 4:  d = abs(a.x*a.y + a.y*a.z); break;
        case 5:  d = abs(max(aa.x, max(aa.y, aa.z)) - min(aa.x, min(aa.y, aa.z))); break;
        case 6:  d = abs(length(a.xy) - abs(a.z)); break;
        case 7:  d = abs(a.x * a.y * a.z); break;
        case 8:  d = abs(dot(sin(a), cos(a.yzx))); break;
        default: d = abs(length(a) - sphereRadius); break;
        }
        d *= ringThickness;
        z += d;

        // Per-step FFT - map step index to frequency band (BAND_SAMPLES pattern)
        float t0 = float(i) / stepCount;
        float t1 = float(i + 1) / stepCount;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        for (int bs = 0; bs < 4; bs++) {
            float bin = mix(binLo, binHi, (float(bs) + 0.5) / 4.0);
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        energy = pow(clamp(energy / 4.0 * gain, 0.0, 1.0), curve);
        float fftBrightness = baseBright + energy;

        // Gradient LUT coloring with 1/d glow attenuation
        vec3 stepColor = textureLod(gradientLUT, vec2(fract(z * colorStretch + colorPhase), 0.5), 0.0).rgb;
        color += stepColor * fftBrightness / max(d, 0.001);
    }

    // Brightness and tanh tonemap
    color = tanh(color * brightness / 1000.0);

    finalColor = vec4(color, 1.0);
}
