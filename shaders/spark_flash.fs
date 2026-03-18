// Based on "Extreme_spark" by Sheena
// https://www.shadertoy.com/view/7fs3Rr
// License: CC BY-NC-SA 3.0 Unported
// Modified: uniforms replace constants, BAND_SAMPLES FFT, gradientLUT color
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float time;
uniform int layers;
uniform float lifetime;
uniform float armSoftness;
uniform float starSoftness;
uniform float armBrightness;
uniform float starBrightness;
uniform float armReach;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform sampler2D gradientLUT;

#define PI 3.14159265359

float hash12(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;

    vec3 col = vec3(0.0);

    for (int i = 0; i < layers; i++) {
        float fi = float(i);
        float offset = fi * lifetime / float(layers);
        float phase = time + offset;

        float seed = floor(phase / lifetime);
        float lt = fract(phase / lifetime);

        // Random spark position
        float px = hash12(vec2(seed, fi));
        float py = hash12(vec2(seed, fi + 50.0));

        // Sine lifecycle for arm length and brightness base
        float armLen = sin(lt * PI) * armReach;
        float bright = sin(lt * PI);

        // FFT energy per spark - BAND_SAMPLES multi-sample averaging
        float t0 = float(i) / float(max(layers - 1, 1));
        float t1 = float(i + 1) / float(max(layers, 1));
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Smoothstep fade envelope on arm length
        float vFade = smoothstep(armLen, armLen * 0.55, abs(uv.y - py));
        float hFade = smoothstep(armLen, armLen * 0.55, abs(uv.x - px));

        // Inverse-square glow for arms - armSoftness 0.1-10 mapped to epsilon
        float armEps = armSoftness * 0.0001;
        float vGlow = 1.0 / (pow(abs(uv.x - px), 2.0) + armEps);
        float hGlow = 1.0 / (pow(abs(uv.y - py), 2.0) + armEps);

        // Inverse-distance star point - starSoftness 0.1-10 mapped to epsilon
        float starEps = starSoftness * 0.0001;
        vec2 d = uv - vec2(px, py);
        d.x *= aspect;
        float star = 1.0 / (dot(d, d) + starEps);

        // Gradient LUT color per spark
        float colorT = float(i) / float(max(layers - 1, 1));
        vec3 sparkColor = texture(gradientLUT, vec2(colorT, 0.5)).rgb;
        vec3 starColor = clamp(sparkColor * 1.4 + vec3(0.25), 0.0, 2.0);

        float armBright = armBrightness * 0.0001;
        float starBright = starBrightness * 0.0001;
        col += vGlow * vFade * sparkColor * brightness * bright * armBright;
        col += hGlow * hFade * sparkColor * brightness * bright * armBright;
        col += star * starColor * brightness * bright * starBright;
    }

    // Reinhard tonemapping
    col = col / (col + 1.0);
    finalColor = vec4(col, 1.0);
}
