// Nebula: Parallax kaliset/FBM fractal gas with per-semitone star twinkling.
// Kaliset field from CBS "Simplicity Galaxy"; color from gradient LUT.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float time;
uniform float baseFreq;
uniform int starBins;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
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
uniform int noiseType;
uniform float dustScale;
uniform float dustStrength;
uniform float dustEdge;
uniform float spikeIntensity;
uniform float spikeSharpness;

#define FOLD_OFFSET vec3(-0.5, -0.4, -1.5)

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1, 0)), f.x),
        mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x),
        f.y
    );
}

float fbm(vec2 p, int octaves) {
    float v = 0.0, a = 0.5;
    mat2 rot = mat2(0.8, 0.6, -0.6, 0.8);
    for (int i = 0; i < octaves; i++) {
        v += a * noise(p);
        p = rot * p * 2.0;
        a *= 0.5;
    }
    return v;
}

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

// Per-layer gas evaluation: branches on noiseType
float evalLayer(vec3 p, float s, vec2 layerUV, int iterations) {
    if (noiseType == 1) {
        int oct = clamp(iterations, 2, 8);
        vec2 q = vec2(fbm(layerUV * 2.0 + time * 0.08, oct),
                      fbm(layerUV * 2.0 + vec2(5.2, 1.3), oct));
        vec2 rr = vec2(fbm(layerUV * 2.0 + 4.0 * q + vec2(1.7, 9.2) + 0.12 * time, oct),
                       fbm(layerUV * 2.0 + 4.0 * q + vec2(8.3, 2.8) + 0.1 * time, oct));
        return fbm(layerUV * 2.0 + 4.0 * rr, oct);
    }
    return field(p, s, iterations);
}

vec3 nrand3(vec2 co) {
    vec3 a = fract(cos(co.x * 8.3e-3 + co.y) * vec3(1.3e5, 4.7e5, 2.9e5));
    vec3 b = fract(sin(co.x * 0.3e-3 + co.y) * vec3(8.1e5, 1.0e5, 0.1e5));
    return mix(a, b, 0.5);
}

// Per-bin stars with FFT-reactive twinkle and white-hot cores
vec3 starLayer(vec2 starUV, int totalStarBins) {
    vec3 color = vec3(0.0);
    vec2 cell = floor(starUV);
    vec2 fr = fract(starUV);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            vec2 nb = vec2(float(dx), float(dy));
            vec3 rnd = nrand3(cell + nb);
            if (rnd.y < 1.0 - 1.0 / starSharpness) continue;
            vec2 sp = nb + vec2(rnd.x, fract(rnd.y * 17.3)) - fr;
            float d2 = dot(sp, sp);
            float semi = floor(rnd.z * float(totalStarBins));
            float sBin = baseFreq * pow(maxFreq / baseFreq, semi / float(totalStarBins)) / (sampleRate * 0.5);
            float sMag = (sBin <= 1.0) ? texture(fftTexture, vec2(sBin, 0.5)).r : 0.0;
            // Audio-reactive twinkle: silent stars shimmer slowly, loud stars pulse faster
            float phase = fract(rnd.x * 31.7 + rnd.z * 17.3) * 6.2832;
            float twinkle = 0.7 + 0.3 * sin(time * (1.0 + sMag * 8.0) + phase);
            float react = baseBright + sMag * sMag * gain;
            float glow = react * twinkle * exp(-d2 / (2.0 * glowWidth * glowWidth)) * glowIntensity;
            // White-hot core fading to LUT color at edges
            vec3 tint = texture(gradientLUT, vec2(semi / float(totalStarBins), 0.5)).rgb;
            float core = exp(-d2 / (0.5 * glowWidth * glowWidth));
            color += glow * mix(tint, vec3(1.0), core);
            // Diffraction spikes on brightest stars
            float h = rnd.y;
            if (h > 0.90) {
                float angle = atan(sp.y, sp.x);
                float flicker = 0.5 + 0.5 * sin(time * (2.0 + rnd.z * 4.0) + rnd.x * 31.4);
                float spike = pow(abs(cos(angle)), spikeSharpness)
                            + pow(abs(sin(angle)), spikeSharpness);
                spike *= exp(-d2 * 2.0) * spikeIntensity * flicker * react;
                color += spike * mix(tint, vec3(1.0), 0.5);
            }
        }
    }
    return color;
}

void main() {
    vec2 uv = 2.0 * fragTexCoord - 1.0;
    vec2 uvs = uv * resolution / max(resolution.x, resolution.y);

    int totalStarBins = starBins;

    // Drift — driftSpeed accumulated into time on CPU, no jumps on slider change
    vec3 drift = vec3(sin(time / 16.0), sin(time / 12.0), sin(time / 128.0));

    // --- Layer 1: Foreground (z=0) ---
    vec3 p = vec3(uvs / frontScale, 0.0) + vec3(1.0, -1.3, 0.0);
    p += 0.3 * drift;
    float t = evalLayer(p, length(p), uvs / frontScale + 0.3 * drift.xy, frontIter);

    // --- Layer 2: Mid (z=2.5) ---
    vec3 pm = vec3(uvs / midScale, 2.5) + vec3(1.5, -0.8, 0.0);
    pm += 0.2 * drift;
    float tm = evalLayer(pm, length(pm), uvs / midScale + 0.2 * drift.xy, midIter);

    // --- Layer 3: Background (z=5) ---
    float backScaleAnim = backScale + sin(time * 0.11) * 0.2 + 0.2 + sin(time * 0.15) * 0.3 + 0.4;
    vec3 p2 = vec3(uvs / backScaleAnim, 5.0) + vec3(2.0, -1.3, -1.0);
    p2 += 0.12 * drift;
    float t2 = evalLayer(p2, length(p2), uvs / backScaleAnim + 0.12 * drift.xy, backIter);

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

    // Dust lanes — darken gas along FBM-driven filaments
    float dust = fbm(uvs * dustScale + vec2(time * 0.05, -time * 0.03), 6);
    float dustLanes = smoothstep(0.45 - dustEdge, 0.45 + dustEdge, dust) * dustStrength;
    vec3 darkTint = texture(gradientLUT, vec2(0.02, 0.5)).rgb * 0.1;
    gasColor = mix(gasColor, darkTint, dustLanes * 0.7);

    // --- Per-semitone stars with layer parallax ---
    vec3 starColor = vec3(0.0);
    starColor += starLayer((uvs / frontScale + 0.3 * drift.xy) * 2.0 * starDensity + vec2(137.0, -89.0), totalStarBins);
    starColor += starLayer((uvs / midScale + 0.2 * drift.xy) * 2.0 * starDensity + vec2(251.0, -173.0), totalStarBins);
    starColor += starLayer((uvs / backScale + 0.12 * drift.xy) * 2.0 * starDensity + vec2(419.0, -307.0), totalStarBins);

    // --- Final ---
    vec3 result = (gasColor + starColor) * brightness;

    finalColor = vec4(result, 1.0);
}
