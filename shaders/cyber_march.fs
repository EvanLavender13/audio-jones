// Based on "Fractalic Explorer 2" by nayk
// https://www.shadertoy.com/view/7fS3Wh
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT audio reactivity, gradient LUT coloring
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float flyPhase;
uniform float morphPhase;
uniform int marchSteps;
uniform float stepSize;
uniform float fov;
uniform float domainSize;
uniform int foldIterations;
uniform float foldScale;
uniform float morphAmount;
uniform vec3 foldOffset;
uniform float initialScale;
uniform float colorSpread;
uniform float tonemapGain;
uniform float yaw;
uniform float pitch;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform int colorFreqMap;
uniform float baseBright;
uniform sampler2D gradientLUT;

float sampleFFTBand(float freqT0, float freqT1) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, freqT0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, freqT1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int b = 0; b < BAND_SAMPLES; b++) {
        float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return baseBright + mag;
}

mat2 rot(float a) { return mat2(cos(a), -sin(a), sin(a), cos(a)); }

void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;
    vec3 rd = normalize(vec3(uv, fov));

    // Camera rotation (yaw/pitch from CPU Lissajous)
    vec2 trd;
    float sp = sin(pitch), cp = cos(pitch);
    trd = mat2(cp, -sp, sp, cp) * vec2(rd.y, rd.z);
    rd.y = trd.x; rd.z = trd.y;
    float sy = sin(yaw), cy = cos(yaw);
    trd = mat2(cy, -sy, sy, cy) * vec2(rd.x, rd.z);
    rd.x = trd.x; rd.z = trd.y;

    vec3 ro = vec3(0.0, 0.0, flyPhase);

    vec3 color = vec3(0.0);
    vec2 t2;

    for (float i = 0., s, e, g = 0.; i++ < marchSteps;) {
        vec3 p = ro + g * rd;

        // Domain repetition
        p = mod(p - domainSize, domainSize * 2.0) - domainSize;

        // G(p.xz, 1.0)
        t2 = floor(vec2(p.x, p.z) * 1.0) / 1.0;
        p.x = t2.x; p.z = t2.y;
        // Q(p.xz)
        t2 = rot(round(atan(p.x, p.z) * 4.0) / 4.0) * vec2(p.x, p.z);
        p.x = t2.x; p.z = t2.y;
        // G(p.xy, 1.0)
        t2 = floor(vec2(p.x, p.y) * 1.0) / 1.0;
        p.x = t2.x; p.y = t2.y;
        // F(p.xz, 2.0)
        t2 = abs(rot(2.0) * vec2(p.x, p.z)); t2.y -= 0.5;
        p.x = t2.x; p.z = t2.y;
        // G(p.yz, 1.0)
        t2 = floor(vec2(p.y, p.z) * 1.0) / 1.0;
        p.y = t2.x; p.z = t2.y;
        // M(p.xz)
        t2 = abs(vec2(p.x, p.z)) - 0.5; t2 *= rot(0.785); t2 = abs(t2) - 0.2;
        p.x = t2.x; p.z = t2.y;
        // M(p.zy)
        t2 = abs(vec2(p.z, p.y)) - 0.5; t2 *= rot(0.785); t2 = abs(t2) - 0.2;
        p.z = t2.x; p.y = t2.y;
        // Q2(p.xy)
        t2 = abs(vec2(p.x, p.y) - 1.0) - abs(vec2(p.x, p.y) + 1.0) + vec2(p.x, p.y);
        p.x = t2.x; p.y = t2.y;

        // Fractal fold loop
        s = initialScale;
        for (int j = 0; j < foldIterations; j++) {
            // Q2(p.zy)
            t2 = abs(vec2(p.z, p.y) - 1.0) - abs(vec2(p.z, p.y) + 1.0) + vec2(p.z, p.y);
            p.z = t2.x; p.y = t2.y;

            p = 0.3 - abs(p);

            // Axis sort
            if (p.x < p.z) { p = p.zyx; }
            if (p.z < p.y) { p = p.xzy; }

            e = foldScale + morphAmount * sin(morphPhase);
            s *= e;
            p = abs(p) * e - foldOffset;
        }

        // Distance estimate and accumulation
        e = length(p.yzzz) / s;
        g += stepSize * e;

        // Gradient color from march distance
        float gradientT = fract(g * colorSpread);
        vec3 surfColor = texture(gradientLUT, vec2(gradientT, 0.5)).rgb;

        // FFT brightness
        float brightness;
        if (colorFreqMap == 0) {
            float ft0 = (i - 1.0) / float(marchSteps - 1);
            float ft1 = i / float(marchSteps);
            brightness = sampleFFTBand(ft0, ft1);
        } else {
            float bw = 0.1;
            brightness = sampleFFTBand(gradientT, gradientT + bw);
        }

        color += surfColor / max(e, 0.001) * brightness;
    }

    finalColor = vec4(tanh(color * tonemapGain), 1.0);
}
