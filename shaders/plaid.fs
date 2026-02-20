#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float scale;
uniform int bandCount;       // N unique bands (3-8)
uniform float accentWidth;
uniform float threadDetail;
uniform int twillRepeat;
uniform float morphAmount;
uniform float time;          // accumulated morphSpeed * deltaTime
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define MAX_BANDS 16   // bandCount*2 after mirroring
#define BAND_SAMPLES 4
#define WEFT_PHASE_OFFSET 3.7 // separates weft morph from warp morph

void computeBandWidths(float phaseBase, out float widths[MAX_BANDS], out int totalBands) {
    totalBands = bandCount * 2;
    for (int i = 0; i < bandCount; i++) {
        // Each band gets a unique width via golden ratio quasi-random sequence
        // Range: accentWidth (thinnest) to 1.0 (widest), like real tartan threadcounts
        float baseWidth = accentWidth + fract(float(i) * 0.618) * (1.0 - accentWidth);
        // Morph: oscillate width with per-band phase and frequency variation
        float phase = float(i) * 1.618 + phaseBase;
        float freq = 0.7 + 0.6 * fract(float(i + 3) * 0.4137);
        widths[i] = baseWidth + sin(time * freq + phase) * morphAmount * baseWidth;
        widths[i] = max(widths[i], 0.01);  // clamp positive
    }
    // Mirror: bands go 0..N-1, N-1..0
    for (int i = 0; i < bandCount; i++) {
        widths[bandCount + i] = widths[bandCount - 1 - i];
    }
}

int getBandIndex(float pos, float widths[MAX_BANDS], int count) {
    float total = 0.0;
    for (int i = 0; i < count; i++) total += widths[i];
    float localPos = mod(pos, total);
    float cursor = 0.0;
    for (int i = 0; i < count; i++) {
        if (localPos < cursor + widths[i]) return i;
        cursor += widths[i];
    }
    return 0;
}

float bandBrightness(int bandIdx, int totalBands) {
    float t0 = float(bandIdx) / float(totalBands);
    float t1 = float(bandIdx + 1) / float(totalBands);
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return baseBright + energy;
}

void main() {
    // Center UV, aspect-correct, apply scale
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;
    uv *= scale;

    // Separate band widths for warp (vertical) and weft (horizontal)
    // Different morph phases mean the two axes breathe independently
    float warpWidths[MAX_BANDS];
    float weftWidths[MAX_BANDS];
    int totalBands;
    computeBandWidths(0.0, warpWidths, totalBands);
    int weftBands;
    computeBandWidths(WEFT_PHASE_OFFSET, weftWidths, weftBands);

    // Cumulative totals differ per axis
    float warpTotal = 0.0;
    float weftTotal = 0.0;
    for (int i = 0; i < totalBands; i++) {
        warpTotal += warpWidths[i];
        weftTotal += weftWidths[i];
    }

    // Warp (vertical bands) and weft (horizontal bands)
    float warpPos = mod(uv.x * warpTotal, warpTotal);
    float weftPos = mod(uv.y * weftTotal, weftTotal);

    int warpIdx = getBandIndex(warpPos, warpWidths, totalBands);
    int weftIdx = getBandIndex(weftPos, weftWidths, totalBands);

    // Map post-mirror index back to pre-mirror for symmetric LUT colors
    // Bands go 0..N-1, N-1..0 so mirrored half maps back: idx >= N → (2N-1-idx)
    int warpOrig = warpIdx < bandCount ? warpIdx : (totalBands - 1 - warpIdx);
    int weftOrig = weftIdx < bandCount ? weftIdx : (totalBands - 1 - weftIdx);
    vec3 warpColor = texture(gradientLUT, vec2((float(warpOrig) + 0.5) / float(bandCount), 0.5)).rgb;
    vec3 weftColor = texture(gradientLUT, vec2((float(weftOrig) + 0.5) / float(bandCount), 0.5)).rgb;

    // Twill: floor-based thread interleave (hard, no AA)
    vec2 threadUV = uv * threadDetail;
    float ix = mod(floor(threadUV.x), float(twillRepeat));
    float iy = mod(floor(threadUV.y), float(twillRepeat));
    bool warpOnTop = mod(ix + iy, float(twillRepeat)) < float(twillRepeat) * 0.5;

    // Blend: top thread 75%, bottom 25%
    vec3 topColor = warpOnTop ? warpColor : weftColor;
    vec3 botColor = warpOnTop ? weftColor : warpColor;
    vec3 color = mix(botColor, topColor, 0.75);

    // FFT brightness from the top band
    int topIdx = warpOnTop ? warpIdx : weftIdx;
    float bright = bandBrightness(topIdx, totalBands);

    color *= bright * glowIntensity;
    finalColor = vec4(color, 1.0);
}
