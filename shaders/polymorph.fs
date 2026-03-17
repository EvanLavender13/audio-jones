// Based on "Arknights：Polyhedrons" by yli110
// https://www.shadertoy.com/view/sc2GzW
// License: CC BY-NC-SA 3.0 Unported
// Modified: All 5 platonic solids, CPU-side morph state machine, explicit edge
// capsules as uniforms, gradient LUT coloring, FFT reactivity

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 edgeA[30];
uniform vec3 edgeB[30];
uniform float edgeT[30];
uniform int edgeCount;
uniform float edgeThickness;
uniform float glowIntensity;
uniform vec3 cameraOrigin;
uniform float cameraFov;

uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define PIXEL (3.0 / min(resolution.x, resolution.y))

// sdCapsule — verbatim from reference
float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}

void main() {
    vec2 p = (2.0 * gl_FragCoord.xy - resolution) / min(resolution.x, resolution.y);

    // Camera setup — orbit camera from reference
    vec3 ta = vec3(0.0);
    vec3 ro = cameraOrigin;
    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
    vec3 vv = normalize(cross(uu, ww));
    vec3 rd = normalize(p.x * uu + p.y * vv + cameraFov * ww);

    // Ray march with per-edge color glow accumulation
    float t = 0.0;
    vec3 accumColor = vec3(0.0);

    for (int step = 0; step < 80; step++) {
        vec3 pos = ro + t * rd;

        // Find closest edge and minimum distance
        float minDist = 1e10;
        int closest = 0;
        for (int e = 0; e < edgeCount; e++) {
            float d = sdCapsule(pos, edgeA[e], edgeB[e], edgeThickness);
            if (d < minDist) {
                minDist = d;
                closest = e;
            }
        }

        // Glow contribution weighted by closest edge's color
        float g = PIXEL / (0.001 + minDist * minDist);
        vec3 color = texture(gradientLUT, vec2(edgeT[closest], 0.5)).rgb;

        // FFT brightness for the closest edge
        float freqRatio = maxFreq / baseFreq;
        float et = edgeT[closest];
        float bandW = 1.0 / 48.0;
        float ft0 = max(et - bandW * 0.5, 0.0);
        float ft1 = min(et + bandW * 0.5, 1.0);
        float freqLo = baseFreq * pow(freqRatio, ft0);
        float freqHi = baseFreq * pow(freqRatio, ft1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float fftEnergy = 0.0;
        for (int s = 0; s < 4; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / 4.0);
            if (bin <= 1.0) {
                fftEnergy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float brightness = baseBright + pow(clamp(fftEnergy / 4.0 * gain, 0.0, 1.0), curve);

        accumColor += color * g * glowIntensity * brightness;

        t += minDist;
        if (t > 100.0) break;
    }

    // Gamma + tonemap from reference
    accumColor = pow(accumColor, vec3(0.4545));
    finalColor = vec4(tanh(accumColor), 1.0);
}
