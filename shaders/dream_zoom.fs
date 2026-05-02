// Inspired by "Dream Zoom" by NR4 (Alexander Kraus)
// https://www.shadertoy.com/view/NfBXDG
// NR4's reference is GPL-3.0-or-later (incompatible with this project's
// CC BY-NC-SA 3.0 license). NO source code from NR4's shader is reproduced
// here. The technique - log-polar transport, Jacobi cn lattice tiling,
// complex-map iteration with orbit-trap coloring, Vogel-disk DOF, temporal
// AA - is reimplemented from public-domain mathematical sources:
//   - DLMF Chapter 22 (Jacobian elliptic functions)
//   - DLMF 22.20.1 (descending Landen / AGM iteration)
//   - DLMF 22.6.1 (addition theorem for complex argument)
//   - DLMF 19.2.8 (complete elliptic integral K)

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

// texture0 holds the previous frame (bound by the fullscreen-quad source);
// see byzantine_display.fs for the same convention.
uniform sampler2D texture0;
uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform float zoomPhase;
uniform float globalRotationPhase;
uniform float rotationPhase;
uniform float jacobiRepeats;
uniform float spiralWrap;
uniform float formulaMix;
uniform int   iterations;
uniform float coordinateScale;
uniform vec2  offset;
uniform float cmapScale;
uniform float cmapOffset;
uniform vec2  trapOffset;
uniform vec2  origin;
uniform vec2  constantOffset;
uniform int   sampleCount;
uniform float taaMix;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float CN_PERIOD_CALIB = 1.18034;
const float CN_WRAP_DISTANCE = 3.7;
const float PI = 3.14159265358979;

// Jacobi sn, cn, dn for real argument u and parameter m = k^2.
// Descending Landen / AGM iteration. DLMF 22.20.1.
void jacobi_sncndn(float u, float m, out float sn, out float cn, out float dn) {
    const int N = 4;
    float emc = 1.0 - m;
    float a = 1.0;
    float em[4];
    float en[4];
    float c = 1.0;

    for (int i = 0; i < N; i++) {
        em[i] = a;
        emc = sqrt(emc);
        en[i] = emc;
        c = 0.5 * (a + emc);
        emc = a * emc;
        a = c;
    }

    u = c * u;
    sn = sin(u);
    cn = cos(u);
    dn = 1.0;

    if (sn != 0.0) {
        float ac = cn / sn;
        c = ac * c;
        for (int i = N - 1; i >= 0; i--) {
            float b = em[i];
            ac = c * ac;
            c = dn * c;
            dn = (en[i] + ac) / (b + ac);
            ac = c / b;
        }
        float a2 = inversesqrt(c * c + 1.0);
        sn = (sn < 0.0) ? -a2 : a2;
        cn = c * sn;
    }
}

// Complex Jacobi cn via the addition theorem. DLMF 22.6.1.
vec2 cn_complex(vec2 z, float m) {
    float snu, cnu, dnu;
    float snv, cnv, dnv;
    jacobi_sncndn(z.x, m, snu, cnu, dnu);
    jacobi_sncndn(z.y, 1.0 - m, snv, cnv, dnv);
    float invDenom = 1.0 / (1.0 - dnu * dnu * snv * snv);
    return invDenom * vec2(cnu * cnv, -snu * dnu * snv * dnv);
}

// Per-iteration map. The mod(abs(z.y))/sign(z.y) folding is load-bearing -
// folds the imaginary axis differently than plain cos/sin would.
vec2 formula(vec2 z) {
    z -= 0.01 * origin;
    float ay = mod(abs(z.y), 2.0 * PI);
    vec2 expBranch =
        min(exp(z.x), 2.0e4) * vec2(cos(ay), sign(z.y) * sin(ay));
    vec2 sqBranch = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y);
    return mix(expBranch, sqBranch, formulaMix) + 0.01 * constantOffset + z;
}

// uv is already centered (output of spiralize); rot and scale are uniforms-only
// values hoisted into main() so they aren't recomputed per Vogel sample.
vec4 pixelOf(vec2 uv, mat2 rot, float scale) {
    uv -= offset * 0.001;
    // exp(mix(log(1e-4), log(1.0), c)) = pow(1e-4, 1.0 - c), since log(1.0)=0.
    vec2 z = scale * rot * uv;

    float tm = 1e9;
    for (int i = 0; i < iterations; i++) {
        if (dot(z, z) > 1e10) { break; }
        z = formula(z);
        tm = min(tm, length(z / (z - trapOffset)) * cmapScale * 0.01);
    }
    float t = fract(cmapOffset - log(mix(exp(0.001), exp(1.0), tm)));

    // BAND_SAMPLES standard - 4 adjacent FFT bins around t, log-spaced.
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float ts = t + (float(s) + 0.5)
                       / float(BAND_SAMPLES)
                       / float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float bright = baseBright + mag;

    return vec4(texture(gradientLUT, vec2(t, 0.5)).rgb * bright, 1.0);
}

vec4 spiralize(vec2 fragCoord, mat2 grot, mat2 rot, float scale) {
    vec2 uv0 = (fragCoord - 0.5 * resolution) / resolution.y;
    vec2 z = grot * uv0;

    z = vec2(log(length(z) + 1e-20), atan(z.y, z.x))
        * CN_PERIOD_CALIB * 0.5 * jacobiRepeats;
    z.x -= mod(zoomPhase / spiralWrap, 1.0) * CN_WRAP_DISTANCE;
    z = mat2(1.0, 1.0, -1.0, 1.0) * z;
    z = cn_complex(z, 0.5);

    return pixelOf(z, rot, scale);
}

void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 uv = (fragCoord - 0.5 * resolution) / resolution.y;

    // Hoisted uniforms-only values - reused across every Vogel sample.
    vec2 gcs = vec2(cos(globalRotationPhase), sin(globalRotationPhase));
    mat2 grot = mat2(gcs.x, gcs.y, -gcs.y, gcs.x);
    vec2 rcs = vec2(cos(rotationPhase), sin(rotationPhase));
    mat2 rot = mat2(rcs.x, rcs.y, -rcs.y, rcs.x);
    float scale = pow(1e-4, 1.0 - coordinateScale);

    // Vogel-disk multi-sample for DOF blur.
    int sc = max(sampleCount, 1);
    vec4 col = vec4(0.0);
    const float gold = 2.4;
    for (int i = 0; i < sc; i++) {
        float fi = float(i) + 0.5;
        float x = fi / float(sc);
        float p = gold * fi;
        vec2 offsetPx = (0.5 / resolution.y) * sqrt(x) * vec2(cos(p), sin(p));
        vec2 sampleFrag = (uv - offsetPx) * resolution.y + 0.5 * resolution;
        col += spiralize(sampleFrag, grot, rot, scale);
    }
    col /= float(sc);

    vec4 prev = texture(texture0, fragTexCoord);
    col = mix(col, prev, taaMix);

    finalColor = vec4(col.rgb, 1.0);
}
