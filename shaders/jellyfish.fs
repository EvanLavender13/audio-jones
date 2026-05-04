// Based on "Bioluminescent Deep Ocean" by hagy
// https://www.shadertoy.com/view/7c2Xz3
// License: CC BY-NC-SA 3.0 Unported
// Modified: surface band removed, Reinhard tonemap removed,
// per-jellyfish FFT log-spaced band brightness, gradient-LUT seeded hues,
// dynamic jellyCount loop, hardcoded brightness constants exposed as uniforms.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float pulsePhase;
uniform float driftPhase;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

uniform int jellyCount;
uniform float sizeBase;
uniform float sizeVariance;
uniform float pulseDepth;
uniform float driftAmp;
uniform float tentacleWave;
uniform float bellGlow;
uniform float rimGlow;
uniform float tentacleGlow;
uniform float snowDensity;
uniform float snowBrightness;
uniform float causticIntensity;
uniform float backdropDepth;

float hash21(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

vec2 hash22(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

mat2 rot2D(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash21(i),                hash21(i + vec2(1.0, 0.0)), u.x),
        mix(hash21(i + vec2(0.0,1.0)),hash21(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    mat2 r = rot2D(0.5);
    for (int i = 0; i < 5; i++) {
        v += a * valueNoise(p);
        p = r * p * 2.0;
        a *= 0.5;
    }
    return v;
}

vec2 worley(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    float d1 = 1e10, d2 = 1e10;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 nb = vec2(float(x), float(y));
            vec2 df = nb + hash22(i + nb) - f;
            float d = dot(df, df);
            if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
        }
    }
    return sqrt(vec2(d1, d2));
}

float caustics(vec2 p, float t) {
    vec2 w1 = worley(p * 3.2 + vec2( 0.7, 0.3) + t * 0.15);
    vec2 w2 = worley(p * 2.1 + vec2( 1.4, 0.9) - t * 0.09);
    return pow(1.0 - w1.x, 4.0) * 0.55 + pow(1.0 - w2.x, 3.0) * 0.45;
}

float sdEllipse(vec2 p, vec2 ab) {
    float k = length(p / ab);
    return (k - 1.0) * min(ab.x, ab.y);
}

float sdSeg(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0));
}

float smax(float a, float b, float k) {
    float h = max(k - abs(a - b), 0.0) / k;
    return max(a, b) + h * h * k * 0.25;
}

vec2 jellyPath(float s, float dPhase) {
    float ax = (0.09 + hash21(vec2(s, 1.5)) * 0.07) * driftAmp;
    float ay = (0.06 + hash21(vec2(s, 1.6)) * 0.05) * driftAmp;
    float fx = 0.22 + hash21(vec2(s, 1.1)) * 0.16;
    float fy = 0.17 + hash21(vec2(s, 1.2)) * 0.12;
    float px = hash21(vec2(s, 1.3)) * 6.283;
    float py = hash21(vec2(s, 1.4)) * 6.283;
    return vec2(ax * sin(dPhase * fx + px), ay * sin(dPhase * fy + py));
}

vec3 jellyfish(vec2 uv, vec2 ctr, float s, vec3 hue, float phase, float t) {
    float sz   = sizeBase + hash21(vec2(s, 9.2)) * sizeVariance;
    float pSpd = 0.55 + hash21(vec2(s, 9.1)) * 0.35;

    float pulse = 1.0 + pulseDepth * sin(pulsePhase * pSpd * 3.14159 + phase);
    float pz    = sz * pulse;
    vec2  pos   = ctr + jellyPath(s, driftPhase);
    vec2  lp    = uv - pos;

    float bellBody = sdEllipse(lp, vec2(pz * 0.76, pz * 0.38));
    float bellD    = smax(bellBody, lp.y + pz * 0.07, pz * 0.10);

    float cavD = sdEllipse(lp - vec2(0.0, pz * 0.10), vec2(pz * 0.46, pz * 0.29));

    float tentD = 100.0;
    float waveScale = tentacleWave / 0.007;
    for (int i = 0; i < 8; i++) {
        float fi   = float(i);
        float ang  = (fi / 7.0 - 0.5) * 1.5;
        float tLen = (0.05 + hash21(vec2(s, fi + 20.0)) * 0.04) * 1.5;
        vec2  prev = pos + vec2(sin(ang) * pz * 0.65, -pz * 0.06);
        for (int j = 0; j < 4; j++) {
            float fj  = float(j);
            float wX  = (sin(t * 1.1 + fi * 1.4 + fj * 0.9 + s * 4.7) * 0.007
                       + sin(t * 2.3 + fi * 0.8 + fj * 1.5 + s * 2.1) * 0.004) * waveScale;
            vec2  nxt = prev + vec2(wX, -tLen * 0.25);
            float taper = mix(0.0045, 0.0012, fj / 3.0);
            tentD = min(tentD, sdSeg(uv, prev, nxt) - taper);
            prev  = nxt;
        }
    }

    float rimD       = abs(sdEllipse(lp + vec2(0.0, pz * 0.07), vec2(pz * 0.68, pz * 0.265)));
    float rimGlowVal = exp(-rimD * rimD * 5500.0) * rimGlow;

    float cavMask  = smoothstep(-pz * 0.02, pz * 0.04, cavD);
    float cavRim   = exp(-cavD  * cavD  * 3500.0) * 0.12;
    float interior = smoothstep(0.0, -pz * 0.42, bellD) * (0.04 + 0.10 * cavMask);

    float bellRim     = exp(-bellD * bellD * 300.0);
    float bellHaloVal = exp(-max(bellD, 0.0) * 14.0) * bellGlow;
    float tentGlowVal = exp(-max(tentD, 0.0) * 60.0) * tentacleGlow;

    float nVar = valueNoise(uv * 11.0 + vec2(s * 2.3)) * 0.3 + 0.7;

    vec3 glow  = hue * (bellRim + bellHaloVal + interior + cavRim + tentGlowVal) * nVar;
    glow      += mix(hue, vec3(1.0), 0.7) * rimGlowVal;
    return glow;
}

vec3 marineSnow(vec2 uv, float t) {
    vec3  col = vec3(0.0);
    float cs  = mix(0.18, 0.04, snowDensity);
    vec2  cb  = floor(uv / cs);

    for (int cy = -1; cy <= 1; cy++) {
        for (int cx = -1; cx <= 1; cx++) {
            vec2  ci  = cb + vec2(float(cx), float(cy));
            vec2  rng = hash22(ci);
            float spd = 0.018 + hash21(ci + vec2(3.17, 1.43)) * 0.012;

            vec2  pt = vec2(
                (ci.x + rng.x) * cs,
                (ci.y + fract(rng.y + t * spd / cs)) * cs
            );

            float twk = 0.45 + 0.55 * sin(t * (0.35 + rng.x * 0.7) + rng.y * 6.283);
            float d   = length(uv - pt);
            float brt = 0.55 + 0.45 * smoothstep(-0.45, 0.40, pt.y);

            col += vec3(0.62, 0.82, 1.0) * exp(-d * d * 55000.0) * twk * snowBrightness * brt;
        }
    }
    return col;
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;
    float t = time;

    float depth = clamp(uv.y + 0.5, 0.0, 1.0);

    vec3  abyssCol = vec3(0.005, 0.010, 0.040);
    vec3  deepCol  = vec3(0.012, 0.035, 0.130);
    vec3  surfCol  = vec3(0.030, 0.080, 0.250);
    float dAbove   = smoothstep(0.0, 0.30, depth);
    vec3  col      = mix(abyssCol, mix(deepCol, surfCol, pow(depth, 1.3)), dAbove);
    col += vec3(0.0, 0.007, 0.022) * fbm(uv * 2.8 + vec2(t * 0.07, t * 0.04));
    col *= backdropDepth;

    col += marineSnow(uv, t);

    for (int s = 0; s < jellyCount; s++) {
        float fs = float(s) + 1.0;
        vec2  ctr   = (hash22(vec2(fs, 7.7)) - 0.5) * vec2(0.8, 0.6);
        float phase = hash21(vec2(fs, 8.3)) * 6.283;
        float lutT  = hash21(vec2(fs, 5.5));
        vec3  hue   = texture(gradientLUT, vec2(lutT, 0.5)).rgb;

        float t0 = float(s) / float(jellyCount);
        float t1 = float(s + 1) / float(jellyCount);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int b = 0; b < BAND_SAMPLES; b++) {
            float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float fftBoost = baseBright + mag;

        col += jellyfish(uv, ctr, fs, hue, phase, t) * fftBoost;
    }

    float causticMask = smoothstep(0.12, 0.85, depth);
    float caus = caustics(uv * 2.3 + vec2(t * 0.07), t * 0.22);
    col += vec3(0.6, 0.8, 1.0) * caus * causticMask * causticIntensity;

    vec2  vUV = gl_FragCoord.xy / resolution;
    vec2  vq  = vUV * (1.0 - vUV);
    col *= mix(0.22, 1.0, pow(vq.x * vq.y * 15.0, 0.32));

    finalColor = vec4(col, 1.0);
}
