// Based on "Circles spinning" by evewas
// https://www.shadertoy.com/view/scXXz4
// License: CC BY-NC-SA 3.0 Unported
// Modified: procedural tree, FFT brightness, gradient LUT, seed-based lines
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float levelPhase[5];
uniform float variationPhase[5];
uniform int branches;
uniform int depth;
uniform float rootRadius;
uniform float radiusDecay;
uniform float radiusVariation;
uniform float strokeWidth;
uniform float strokeTaper;
uniform int lineMode;
uniform float lineSeed;
uniform int lineCount;
uniform float lineBrightness;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform sampler2D gradientLUT;

const float TAU = 6.28318530718;
const int BAND_SAMPLES = 4;

float ringHash(int idx) {
    return fract(sin(float(idx) * 127.1) * 43758.5453);
}

// Compute ring world position and radius from flat index
// Returns vec3(center.x, center.y, radius)
vec3 computeRing(int idx) {
    if (idx == 0) { return vec3(0.0, 0.0, rootRadius); }

    int level = 0;
    int lStart = 0;
    int lSize = 1;
    for (int L = 1; L <= depth; L++) {
        lStart += lSize;
        lSize *= branches;
        if (idx < lStart + lSize) {
            level = L;
            break;
        }
    }
    int indexInLevel = idx - lStart;

    vec2 pos = vec2(0.0);
    float parentR = rootRadius;

    for (int K = 1; K <= level; K++) {
        int stride = 1;
        for (int p = 0; p < level - K; p++) { stride *= branches; }

        int ancestorInLevel = indexInLevel / stride;
        int siblingIdx = ancestorInLevel % branches;

        int flatK = 0;
        int cnt = 1;
        for (int i = 0; i < K; i++) { flatK += cnt; cnt *= branches; }
        flatK += ancestorInLevel;

        float varOff = (ringHash(flatK + 1024) - 0.5) * variationPhase[K];
        float rotOff = TAU * float(siblingIdx) / float(branches);
        float angle = levelPhase[K] + varOff + rotOff;

        pos += parentR * vec2(cos(angle), sin(angle));

        float baseR = rootRadius * pow(1.0 - radiusDecay, float(K));
        parentR = baseR * (1.0 + (ringHash(flatK) * 2.0 - 1.0) * radiusVariation);
    }

    return vec3(pos, parentR);
}

int computeLevel(int idx) {
    if (idx == 0) { return 0; }
    int lStart = 0;
    int lSize = 1;
    for (int L = 1; L <= depth; L++) {
        lStart += lSize;
        lSize *= branches;
        if (idx < lStart + lSize) { return L; }
    }
    return depth;
}

float cutoff(float sdf, float thickness) {
    float fade = 0.001;
    return 1.0 - smoothstep(clamp(sdf - thickness, 0.0, 1.0), -fade, fade);
}

float lineSDF(vec2 uv, vec2 a, vec2 b) {
    vec2 pa = uv - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba);
}

// FFT lookup with BAND_SAMPLES sub-sampling
float fftBrightness(float t0, float t1, float freqRatio) {
    float freqLo = baseFreq * pow(freqRatio, t0);
    float freqHi = baseFreq * pow(freqRatio, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return baseBright + mag;
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    vec2 uv = (fc - 0.5 * resolution) / min(resolution.x, resolution.y);

    // Total ring count
    int ringCount = 0;
    int cnt = 1;
    for (int L = 0; L <= depth; L++) { ringCount += cnt; cnt *= branches; }

    int leafStart = ringCount - cnt / branches;
    int leafCount = ringCount - leafStart;

    vec3 total = vec3(0.0);
    float freqRatio = maxFreq / baseFreq;
    float rcf = float(ringCount);

    // Ring SDFs
    for (int i = 0; i < ringCount; i++) {
        vec3 ring = computeRing(i);
        float d = abs(length(uv - ring.xy) - ring.z);

        float level = float(computeLevel(i));
        float taper = 1.0 - strokeTaper * level / float(depth);
        float sw = strokeWidth * taper;
        float mask = cutoff(d, sw);

        float t = float(i) / rcf;
        float t1 = float(i + 1) / rcf;
        float brightness = fftBrightness(t, t1, freqRatio);

        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
        total += color * mask * brightness;
    }

    // Lines
    if (lineBrightness > 0.0 && leafCount > 1) {
        if (lineMode == 0 && lineCount > 0) {
            // Seeded random pairs
            int seedInt = int(floor(lineSeed * 137.0));
            for (int i = 0; i < lineCount; i++) {
                int a = int(floor(ringHash(seedInt + i) * float(leafCount)));
                int b = int(floor(ringHash(seedInt + i + 500) * float(leafCount)));
                if (a == b) { continue; }

                vec3 ra = computeRing(leafStart + a);
                vec3 rb = computeRing(leafStart + b);

                float ld = lineSDF(uv, ra.xy, rb.xy);
                float line = cutoff(ld, strokeWidth * 0.5);

                float ta = float(leafStart + a) / rcf;
                float tb = float(leafStart + b) / rcf;
                float lt = (ta + tb) * 0.5;
                float lt1 = lt + 0.5 / rcf;
                float lBright = fftBrightness(lt, lt1, freqRatio);

                vec3 lColor = texture(gradientLUT, vec2(lt, 0.5)).rgb;
                total += lColor * line * lineBrightness * lBright;
            }
        } else if (lineMode == 1) {
            // Siblings: consecutive groups of `branches` share a parent
            for (int i = 0; i < leafCount; i++) {
                int groupA = i / branches;
                for (int j = i + 1; j < leafCount; j++) {
                    if (j / branches != groupA) { continue; }

                    vec3 ra = computeRing(leafStart + i);
                    vec3 rb = computeRing(leafStart + j);

                    float ld = lineSDF(uv, ra.xy, rb.xy);
                    float line = cutoff(ld, strokeWidth * 0.5);

                    float ta = float(leafStart + i) / rcf;
                    float tb = float(leafStart + j) / rcf;
                    float lt = (ta + tb) * 0.5;
                    float lt1 = lt + 0.5 / rcf;
                    float lBright = fftBrightness(lt, lt1, freqRatio);

                    vec3 lColor = texture(gradientLUT, vec2(lt, 0.5)).rgb;
                    total += lColor * line * lineBrightness * lBright;
                }
            }
        }
    }

    finalColor = vec4(total, 1.0);
}
