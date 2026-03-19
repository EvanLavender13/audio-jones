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
uniform sampler2D gradientLUT;

void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;
    vec3 rd = normalize(vec3(uv, 1.0));

    vec3 color = vec3(0.0);
    float t = 0.1;

    for (int i = 0; i < marchSteps; i++) {
        // Position: ray origin at (pan.x, pan.y, flyPhase), march along rd
        vec3 p = vec3(pan, flyPhase) + t * rd;

        // Voxel quantization with sin-boundary density variation
        float sinCount = step(0.0, sin(p.x + gridPhase))
                       + step(0.0, sin(p.y + 1.0 + gridPhase))
                       + step(0.0, sin(p.z + 2.0 + gridPhase));
        float s = voxelScale - voxelScale * 0.25 * voxelVariation * sinCount;
        p = round(p * s) / s;

        // Boundary highlight distance
        float boundaryDist = min(min(
            abs(sin(p.x + gridPhase)),
            abs(sin(p.y + 1.0 + gridPhase))),
            abs(sin(p.z + 2.0 + gridPhase))) + 0.01;

        // FFT brightness for this march step (depth-dependent audio)
        float t0 = float(i) / float(marchSteps - 1);
        float t1 = float(i + 1) / float(marchSteps);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
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
        float brightness = baseBright + mag;

        // Surface loop: each iteration folds p via mod, creating mirrored geometry
        float minDist = 1e9;
        for (int surf = 0; surf < surfaceCount; surf++) {
            p = mod(p, cellSize) - cellSize * 0.5;
            float dist = abs(length(p) - shellRadius) + 0.01;

            float gradientT = float(surf) / float(max(surfaceCount - 1, 1));
            vec3 surfColor = texture(gradientLUT, vec2(gradientT, 0.5)).rgb;

            color += (surfColor - p * positionTint + highlightIntensity / boundaryDist) / dist * brightness;

            minDist = min(minDist, dist);
        }

        // March forward
        t += stepSize * minDist;
    }

    finalColor = vec4(tanh(color * tonemapGain), 1.0);
}
