// Based on "Twisty cubes" by ChunderFPV
// https://www.shadertoy.com/view/lclXzH
// License: CC BY-NC-SA 3.0
// wireframe code modified from FabriceNeyret2: https://www.shadertoy.com/view/XfS3DK
// Modified: generic platonic solids via uniform arrays, Lissajous camera, FFT reactivity, gradient LUT coloring

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 vertices[20];
uniform int vertexCount;
uniform vec2 edgeIdx[30];
uniform int edgeCount;
uniform int layerCount;
uniform float scaleRatio;
uniform float twistAngle;
uniform float twistPhase;
uniform float twistPitch;
uniform float twistPitchPhase;
uniform float perspective;
uniform float scale;
uniform float cameraPitch;
uniform float cameraYaw;
uniform float lineWidth;
uniform float glowIntensity;
uniform float contrast;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define HALF_PI 1.5707963

mat2 rotMat(float angle) {
    return mat2(cos(angle + vec4(0, -HALF_PI, HALF_PI, 0)));
}

float lineDist(vec2 p, vec3 A, vec3 B) {
    vec2 a = A.xy;
    vec2 ba = B.xy - a;
    vec2 pa = p - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba) + 0.01 * mix(A.z, B.z, h);
}

vec3 camProject(vec3 p, mat2 pitchMat, mat2 yawMat, float persp) {
    p.zy *= pitchMat;
    p.zx *= yawMat;
    return vec3(p.xy / (p.z + persp), p.z);
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;

    float pixelWidth = lineWidth / resolution.y;

    vec3 c = vec3(0.0);

    for (int i = 0; i < layerCount; i++) {
        float t = float(i) / float(layerCount - 1);
        float layerScale = scale * pow(scaleRatio, float(i));
        float pitchAngle = cameraPitch + twistPitch * float(i) + twistPitchPhase;
        float yawAngle = cameraYaw + twistAngle * float(i) + twistPhase;
        mat2 pitchMat = rotMat(pitchAngle);
        mat2 yawMat = rotMat(yawAngle);

        // Per-layer color from gradient LUT
        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // Per-layer FFT brightness (band-averaged, log-space spread)
        float freqRatio = maxFreq / baseFreq;
        float bandW = 1.0 / 48.0;
        float ft0 = max(t - bandW * 0.5, 0.0);
        float ft1 = min(t + bandW * 0.5, 1.0);
        float freqLo = baseFreq * pow(freqRatio, ft0);
        float freqHi = baseFreq * pow(freqRatio, ft1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float fftEnergy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                fftEnergy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float brightness = baseBright + pow(clamp(fftEnergy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        // Per-edge line distance and glow
        for (int j = 0; j < edgeCount; j++) {
            vec3 vertA = vertices[int(edgeIdx[j].x)] * layerScale;
            vec3 vertB = vertices[int(edgeIdx[j].y)] * layerScale;

            vec3 projA = camProject(vertA, pitchMat, yawMat, perspective);
            vec3 projB = camProject(vertB, pitchMat, yawMat, perspective);

            float dist = lineDist(uv, projA, projB);
            float glow = pixelWidth / (pixelWidth + abs(dist));

            // Max blend (from reference)
            c = max(c, color * glow * glowIntensity * brightness);
        }
    }

    // Output: tanh(c*c*contrast) — squaring for contrast, tanh limits brightness
    finalColor = vec4(tanh(c * c * contrast), 1.0);
}
