// Based on "Refactoring" by OldEclipse
// https://www.shadertoy.com/view/WcycRw
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT audio reactivity, gradient LUT coloring
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float flyPhase;
uniform float gridPhase;
uniform int marchSteps;
uniform float stepSize;
uniform float voxelScale;
uniform float voxelVariation;
uniform float cellSize;
uniform float shellRadius;
uniform int surfaceShape;
uniform int domainFold;
uniform int boundaryWave;
uniform int surfaceCount;
uniform float highlightIntensity;
uniform float positionTint;
uniform float tonemapGain;
uniform vec2 pan;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform int colorFreqMap;
uniform sampler2D gradientLUT;

float wave(float x) {
    if (boundaryWave == 1) {
        return abs(fract(x * 0.159155) * 2.0 - 1.0) * 2.0 - 1.0;
    } else if (boundaryWave == 2) {
        return fract(x * 0.159155) * 2.0 - 1.0;
    }
    return sin(x);
}

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

void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;
    vec3 rd = normalize(vec3(uv, 1.0));

    vec3 color = vec3(0.0);
    float t = 0.1;

    for (int i = 0; i < marchSteps; i++) {
        // Position: ray origin at (pan.x, pan.y, flyPhase), march along rd
        vec3 p = vec3(pan, flyPhase) + t * rd;

        // Voxel quantization with boundary density variation
        float sinCount = step(0.0, wave(p.x + gridPhase))
                       + step(0.0, wave(p.y + 1.0 + gridPhase))
                       + step(0.0, wave(p.z + 2.0 + gridPhase));
        float s = voxelScale - voxelScale * 0.25 * voxelVariation * sinCount;
        p = round(p * s) / s;

        // Boundary highlight distance
        float boundaryDist = min(min(
            abs(wave(p.x + gridPhase)),
            abs(wave(p.y + 1.0 + gridPhase))),
            abs(wave(p.z + 2.0 + gridPhase))) + 0.01;

        // Depth-mapped FFT: one brightness per march step
        float depthBrightness = 0.0;
        if (colorFreqMap == 0) {
            float ft0 = float(i) / float(marchSteps - 1);
            float ft1 = float(i + 1) / float(marchSteps);
            depthBrightness = sampleFFTBand(ft0, ft1);
        }

        // Surface loop: each iteration folds p via mod, creating mirrored geometry
        float minDist = 1e9;
        for (int surf = 0; surf < surfaceCount; surf++) {
            if (domainFold == 1) {
                p = abs(mod(p, cellSize) - cellSize * 0.5);
            } else {
                p = mod(p, cellSize) - cellSize * 0.5;
            }
            float dist;
            if (surfaceShape == 1) {
                dist = abs(dot(sin(p), cos(p.yzx)));
            } else {
                dist = abs(length(p) - shellRadius);
            }
            dist = abs(dist) + 0.01;

            float band = 1.0 / float(surfaceCount);
            float bandBase = float(surf) * band;
            float gradientT = bandBase + band * fract(dot(p, vec3(1.0)) * positionTint);
            vec3 surfColor = texture(gradientLUT, vec2(gradientT, 0.5)).rgb;

            float brightness = depthBrightness;
            if (colorFreqMap == 1) {
                float bw = 1.0 / float(max(surfaceCount, 2));
                brightness = sampleFFTBand(gradientT, gradientT + bw);
            }

            color += (surfColor + highlightIntensity / boundaryDist) / dist * brightness;

            minDist = min(minDist, dist);
        }

        // March forward
        t += stepSize * minDist;
    }

    finalColor = vec4(tanh(color * tonemapGain), 1.0);
}
