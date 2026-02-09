// Nebula: Parallax kaliset fractal gas with per-semitone star twinkling.
// Kaliset field from CBS "Simplicity Galaxy"; color from gradient LUT.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float time;
uniform float baseFreq;
uniform int numOctaves;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float driftSpeed;
uniform float frontScale;
uniform float midScale;
uniform float backScale;
uniform int frontIter;
uniform int midIter;
uniform int backIter;
uniform float starDensity;
uniform float starSharpness;
uniform float glowWidth;
uniform float glowIntensity;
uniform float brightness;
uniform sampler2D gradientLUT;

#define FOLD_OFFSET vec3(-0.5, -0.4, -1.5)

float field(vec3 p, float s, int iterations) {
    float strength = 7.0 + 0.03 * log(1.e-6 + fract(sin(time) * 4373.11));
    float accum = s / 4.0;
    float prev = 0.0;
    float tw = 0.0;
    for (int i = 0; i < iterations; i++) {
        float mag = dot(p, p);
        p = abs(p) / mag + FOLD_OFFSET;
        float w = exp(-float(i) / 7.0);
        accum += w * exp(-strength * pow(abs(mag - prev), 2.2));
        tw += w;
        prev = mag;
    }
    return max(0.0, 5.0 * accum / tw - 0.7);
}

vec3 nrand3(vec2 co) {
    vec3 a = fract(cos(co.x * 8.3e-3 + co.y) * vec3(1.3e5, 4.7e5, 2.9e5));
    vec3 b = fract(sin(co.x * 0.3e-3 + co.y) * vec3(8.1e5, 1.0e5, 0.1e5));
    return mix(a, b, 0.5);
}

void main() {
    vec2 uv = 2.0 * fragTexCoord - 1.0;
    vec2 uvs = uv * resolution / max(resolution.x, resolution.y);

    int totalSemitones = numOctaves * 12;
    int quarter = max(totalSemitones / 4, 1);

    // 4 broad FFT bands
    float freqs[4];
    for (int band = 0; band < 4; band++) {
        float bandSum = 0.0;
        int bandStart = band * quarter;
        int bandEnd = (band == 3) ? totalSemitones : (band + 1) * quarter;
        float bandCount = float(bandEnd - bandStart);
        for (int s = bandStart; s < bandEnd; s++) {
            float freq = baseFreq * pow(2.0, float(s) / 12.0);
            float bin = freq / (sampleRate * 0.5);
            if (bin <= 1.0)
                bandSum += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        freqs[band] = pow(clamp(bandSum * gain / bandCount, 0.0, 1.0), curve);
    }

    // Drift — driftSpeed accumulated into time on CPU, no jumps on slider change
    vec3 drift = vec3(sin(time / 16.0), sin(time / 12.0), sin(time / 128.0));

    // --- Layer 1: Foreground (z=0) ---
    vec3 p = vec3(uvs / frontScale, 0.0) + vec3(1.0, -1.3, 0.0);
    p += 0.3 * drift;
    float t = field(p, length(p), frontIter);

    // --- Layer 2: Mid (z=2.5) ---
    vec3 pm = vec3(uvs / midScale, 2.5) + vec3(1.5, -0.8, 0.0);
    pm += 0.2 * drift;
    float tm = field(pm, length(pm), midIter);

    // --- Layer 3: Background (z=5) ---
    vec3 p2 = vec3(uvs / (backScale + sin(time * 0.11) * 0.2 + 0.2 +
                           sin(time * 0.15) * 0.3 + 0.4), 5.0) +
              vec3(2.0, -1.3, -1.0);
    p2 += 0.12 * drift;
    float t2 = field(p2, length(p2), backIter);

    // --- Gas coloring via gradient LUT, tone-mapped so gas never out-competes stars ---
    vec3 lutFront = texture(gradientLUT, vec2(clamp(t * 0.3 + 0.1, 0.0, 1.0), 0.5)).rgb;
    vec3 frontColor = 0.3 * t * t * t * lutFront;

    vec3 lutMid = texture(gradientLUT, vec2(clamp(tm * 0.3 + 0.4, 0.0, 1.0), 0.5)).rgb;
    vec3 midColor = 0.4 * tm * tm * lutMid;

    vec3 lutBack = texture(gradientLUT, vec2(clamp(t2 * 0.3 + 0.7, 0.0, 1.0), 0.5)).rgb;
    vec3 backColor = 0.5 * t2 * t2 * lutBack;

    // Reinhard tone-map gas to soft-cap brightness
    vec3 gasColor = frontColor + midColor + backColor;
    gasColor = gasColor / (1.0 + gasColor);

    // --- Per-semitone stars: glowing points with layer parallax ---
    vec3 starColor = vec3(0.0);

    // Layer 1 stars (foreground) — scale centered uvs, constant offset seeds layer
    vec2 sUV1 = (uvs / frontScale + 0.3 * drift.xy) * 2.0 * starDensity + vec2(137.0, -89.0);
    vec2 sCell1 = floor(sUV1);
    vec2 sFrac1 = fract(sUV1);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            vec2 nb = vec2(float(dx), float(dy));
            vec3 rnd = nrand3(sCell1 + nb);
            if (rnd.y < 1.0 - 1.0 / starSharpness) continue;
            vec2 sp = nb + vec2(rnd.x, fract(rnd.y * 17.3)) - sFrac1;
            float d2 = dot(sp, sp);
            float semi = floor(rnd.z * float(totalSemitones));
            float sBin = baseFreq * pow(2.0, semi / 12.0) / (sampleRate * 0.5);
            float sMag = (sBin <= 1.0) ? texture(fftTexture, vec2(sBin, 0.5)).r : 0.0;
            // Audio-reactive twinkle: silent stars shimmer slowly, loud stars pulse faster
            float phase = fract(rnd.x * 31.7 + rnd.z * 17.3) * 6.2832;
            float twinkle = 0.7 + 0.3 * sin(time * (1.0 + sMag * 8.0) + phase);
            float react = baseBright + sMag * sMag * gain;
            float glow = react * twinkle * exp(-d2 / (2.0 * glowWidth * glowWidth)) * glowIntensity;
            // White-hot core fading to LUT color at edges
            vec3 tint = texture(gradientLUT, vec2(fract(semi / 12.0), 0.5)).rgb;
            float core = exp(-d2 / (0.5 * glowWidth * glowWidth));
            starColor += glow * mix(tint, vec3(1.0), core);
        }
    }

    // Layer 2 stars (mid)
    vec2 sUV2 = (uvs / midScale + 0.2 * drift.xy) * 2.0 * starDensity + vec2(251.0, -173.0);
    vec2 sCell2 = floor(sUV2);
    vec2 sFrac2 = fract(sUV2);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            vec2 nb = vec2(float(dx), float(dy));
            vec3 rnd = nrand3(sCell2 + nb);
            if (rnd.y < 1.0 - 1.0 / starSharpness) continue;
            vec2 sp = nb + vec2(rnd.x, fract(rnd.y * 17.3)) - sFrac2;
            float d2 = dot(sp, sp);
            float semi = floor(rnd.z * float(totalSemitones));
            float sBin = baseFreq * pow(2.0, semi / 12.0) / (sampleRate * 0.5);
            float sMag = (sBin <= 1.0) ? texture(fftTexture, vec2(sBin, 0.5)).r : 0.0;
            // Audio-reactive twinkle: silent stars shimmer slowly, loud stars pulse faster
            float phase = fract(rnd.x * 31.7 + rnd.z * 17.3) * 6.2832;
            float twinkle = 0.7 + 0.3 * sin(time * (1.0 + sMag * 8.0) + phase);
            float react = baseBright + sMag * sMag * gain;
            float glow = react * twinkle * exp(-d2 / (2.0 * glowWidth * glowWidth)) * glowIntensity;
            // White-hot core fading to LUT color at edges
            vec3 tint = texture(gradientLUT, vec2(fract(semi / 12.0), 0.5)).rgb;
            float core = exp(-d2 / (0.5 * glowWidth * glowWidth));
            starColor += glow * mix(tint, vec3(1.0), core);
        }
    }

    // Layer 3 stars (background)
    vec2 sUV3 = (uvs / backScale + 0.12 * drift.xy) * 2.0 * starDensity + vec2(419.0, -307.0);
    vec2 sCell3 = floor(sUV3);
    vec2 sFrac3 = fract(sUV3);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            vec2 nb = vec2(float(dx), float(dy));
            vec3 rnd = nrand3(sCell3 + nb);
            if (rnd.y < 1.0 - 1.0 / starSharpness) continue;
            vec2 sp = nb + vec2(rnd.x, fract(rnd.y * 17.3)) - sFrac3;
            float d2 = dot(sp, sp);
            float semi = floor(rnd.z * float(totalSemitones));
            float sBin = baseFreq * pow(2.0, semi / 12.0) / (sampleRate * 0.5);
            float sMag = (sBin <= 1.0) ? texture(fftTexture, vec2(sBin, 0.5)).r : 0.0;
            // Audio-reactive twinkle: silent stars shimmer slowly, loud stars pulse faster
            float phase = fract(rnd.x * 31.7 + rnd.z * 17.3) * 6.2832;
            float twinkle = 0.7 + 0.3 * sin(time * (1.0 + sMag * 8.0) + phase);
            float react = baseBright + sMag * sMag * gain;
            float glow = react * twinkle * exp(-d2 / (2.0 * glowWidth * glowWidth)) * glowIntensity;
            // White-hot core fading to LUT color at edges
            vec3 tint = texture(gradientLUT, vec2(fract(semi / 12.0), 0.5)).rgb;
            float core = exp(-d2 / (0.5 * glowWidth * glowWidth));
            starColor += glow * mix(tint, vec3(1.0), core);
        }
    }

    // --- Final ---
    vec3 result = (gasColor + starColor) * brightness;

    finalColor = vec4(result, 1.0);
}
