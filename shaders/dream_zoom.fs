// Inspired by "Dream Zoom" by NR4 (Alexander Kraus)
// https://www.shadertoy.com/view/NfBXDG
// NR4's reference is GPL-3.0-or-later (incompatible with this project's
// CC BY-NC-SA 3.0 license). NO source code from NR4's shader is reproduced
// here. The technique - log-polar transport, Jacobi cn lattice tiling,
// complex-map iteration with orbit-trap coloring - is reimplemented from
// public-domain mathematical sources:
//   - DLMF Chapter 22 (Jacobian elliptic functions)
//   - DLMF 22.20.1 (descending Landen / AGM iteration)
//   - DLMF 22.6.1 (addition theorem for complex argument)
//   - DLMF 19.2.8 (complete elliptic integral K)

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform int   variant;              // 0 = DREAM, 1 = JACOBI
uniform float zoomPhase;            // CPU-accumulated, signed
uniform float globalRotationPhase;  // CPU-accumulated, [-PI, PI]
uniform float rotationPhase;        // CPU-accumulated, [-PI, PI]
uniform float jacobiRepeats;
uniform float formulaMix;
uniform int   iterations;
uniform float cmapScale;
uniform float cmapOffset;
uniform vec2  trapOffset;
uniform vec2  origin;
uniform vec2  constantOffset;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Calibration constants - MUST stay coupled (see Notes).
//   CN_PERIOD_CALIB = 2*K(0.5)/pi, with K the complete elliptic integral
//   of the first kind. Aligns one log-radius cycle to one real-period of
//   cn(z, 0.5). Sources: DLMF 19.2.8, Wikipedia "Jacobi elliptic functions".
const float CN_PERIOD_CALIB = 1.18034;
//   3.7 = integer multiple of cn period along the real axis at m = 0.5.
const float CN_WRAP_DISTANCE = 3.7;

// Jacobi sn, cn, dn for real argument u and parameter m = k^2.
// Descending Landen / AGM iteration. 4 iterations is sufficient for shader
// precision in our parameter range. Source: DLMF 22.20.1.
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

// Complex Jacobi cn via the addition theorem. Source: DLMF 22.6.1.
//   cn(x + iy | m) = ( cn(x|m)*cn(y|1-m)
//                    - i sn(x|m)*dn(x|m)*sn(y|1-m)*dn(y|1-m) )
//                    / ( 1 - dn(x|m)^2 * sn(y|1-m)^2 )
vec2 cn_complex(vec2 z, float m) {
    float snu, cnu, dnu;
    float snv, cnv, dnv;
    jacobi_sncndn(z.x, m, snu, cnu, dnu);
    jacobi_sncndn(z.y, 1.0 - m, snv, cnv, dnv);
    float invDenom = 1.0 / (1.0 - dnu * dnu * snv * snv);
    return invDenom * vec2(cnu * cnv, -snu * dnu * snv * dnv);
}

void main() {
    // ----- 1. Per-pixel coordinate transport -----

    // Centered, isotropic UV in [-aspect, aspect] x [-1, 1].
    vec2 uv = (fragTexCoord - 0.5) * resolution / resolution.y;

    // Global rotation of the sampling plane (slow drift).
    float gphi = globalRotationPhase;
    mat2 grot = mat2(cos(gphi), -sin(gphi), sin(gphi), cos(gphi));
    uv = grot * uv;

    // Cartesian -> log-polar:  clog(z) = (log|z|, atan(z.y, z.x))
    vec2 z = vec2(log(length(uv) + 1e-20), atan(uv.y, uv.x));

    // Pre-scale to align one log-radius cycle with one cn period.
    z *= CN_PERIOD_CALIB * 0.5 * jacobiRepeats;

    // Continuous translation along real axis = continuous radial zoom.
    // mod() wrap is seamless because 3.7 is a cn period at m = 0.5.
    z.x -= mod(zoomPhase, 1.0) * CN_WRAP_DISTANCE;

    // 45-degree shear places the doubly-periodic cn lattice diagonally so
    // successive zoom steps don't alias into vertical bands.
    z = mat2(1.0, 1.0, -1.0, 1.0) * z;

    // Map through Jacobi cn at m = 0.5 (lemniscatic modulus).
    z = cn_complex(z, 0.5);

    // ----- 2. Variant pre-iteration affine -----

    float coordinateScale;
    vec2  preOffset;
    if (variant == 0) {        // VARIANT_DREAM (identity)
        coordinateScale = 1.0;
        preOffset = vec2(0.0);
    } else {                    // VARIANT_JACOBI
        coordinateScale = 1.063;
        preOffset = vec2(11.88, -23.33);
    }

    z -= preOffset * 0.001;
    z *= exp(mix(log(1e-4), log(1.0), coordinateScale));

    // ----- 3. Inner fractal rotation (rotationPhase) -----
    // Applied to z after cn-tiling and variant scale, before the iteration
    // loop. Symmetric with the global UV rotation in step 1.
    float rphi = rotationPhase;
    mat2 rrot = mat2(cos(rphi), -sin(rphi), sin(rphi), cos(rphi));
    z = rrot * z;

    // ----- 4. Per-pixel fractal iteration with orbit trap -----

    vec2 zf = z - 0.01 * origin;
    float tmin = 1e9;

    for (int i = 0; i < iterations; i++) {
        if (dot(zf, zf) > 1e10) { break; }

        // exp(zf) for complex zf, with x clamped to prevent overflow.
        //   exp(x + iy) = exp(x) * (cos(y), sin(y))
        vec2 expz = vec2(cos(zf.y), sin(zf.y)) * min(exp(zf.x), 2.0e4);

        // zf * zf for complex zf:  (a + bi)^2 = (a*a - b*b) + 2ab*i
        vec2 sqz = vec2(zf.x * zf.x - zf.y * zf.y, 2.0 * zf.x * zf.y);

        // f(z) = mix(exp(z), z*z, formulaMix) + 0.01*constantOffset + z
        zf = mix(expz, sqz, formulaMix) + 0.01 * constantOffset + zf;

        // Moebius-style orbit trap: |z / (z - trapOffset)|
        float t = length(zf / (zf - trapOffset));
        tmin = min(tmin, t * cmapScale * 0.01);
    }

    // ----- 5. Per-pixel depth coordinate -----
    // Equal-tmin pixels lie on the same depth ring of the fractal.
    float tLUT = fract(cmapOffset - log(mix(exp(0.001), exp(1.0), tmin)));

    // ----- 6. FFT-reactive depth modulation (BAND_SAMPLES standard) -----
    // tLUT drives both the gradient lookup AND the FFT band - depth rings
    // respond to specific frequency bands. Per memory/generator_patterns.md:
    // 4 adjacent bins around tLUT, log-spaced via baseFreq/maxFreq.
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float ts = tLUT + (float(s) + 0.5)
                          / float(BAND_SAMPLES)
                          / float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 col = texture(gradientLUT, vec2(tLUT, 0.5)).rgb * brightness;
    finalColor = vec4(col, 1.0);
}
