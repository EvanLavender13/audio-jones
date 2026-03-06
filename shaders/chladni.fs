#version 330

// Chladni: FFT-driven resonant plate eigenmode visualization
// Generates (n,m) mode pairs mathematically, maps each to its resonant
// frequency bin in the FFT, weights by energy.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // ping-pong read buffer
uniform sampler2D fftTexture;    // FFT magnitudes
uniform sampler2D gradientLUT;   // color mapping
uniform vec2 resolution;
uniform float plateSize;
uniform float coherence;
uniform float visualGain;
uniform float nodalEmphasis;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform int diffusionScale;
uniform float decayFactor;
uniform int plateShape;
uniform int fullscreen;

const float PI = 3.14159265;

// Cephes DR1, DR2: squares of first two zeros of J0
const float DR1 = 5.7831859629;
const float DR2 = 30.4712623437;
const float SQ2OPI = 0.797884560802865;

float besselJ0(float x) {
    x = abs(x);
    if (x <= 5.0) {
        float z = x * x;
        if (x < 1.0e-5) return 1.0 - z * 0.25;

        // RP[0..3] rational numerator
        float rp = -4.79443220978e9;
        rp = rp * z + 1.95617491947e12;
        rp = rp * z - 2.49248344361e14;
        rp = rp * z + 9.70862251047e15;

        // RQ[0..7] rational denominator (monic: leading coeff = 1)
        float rq = z + 4.99563147153e2;
        rq = rq * z + 1.73785401676e5;
        rq = rq * z + 4.84409658340e7;
        rq = rq * z + 1.11855537045e10;
        rq = rq * z + 2.11277520115e12;
        rq = rq * z + 3.10518229857e14;
        rq = rq * z + 3.18121955943e16;
        rq = rq * z + 1.71086294081e18;

        return (z - DR1) * (z - DR2) * rp / rq;
    }

    // Hankel asymptotic for x > 5
    float w = 5.0 / x;
    float q = w * w;

    // PP[0..6] / PQ[0..6]
    float pp = 7.96936729297e-4;
    pp = pp * q + 8.28352392107e-2;
    pp = pp * q + 1.23953371646e0;
    pp = pp * q + 5.44725003059e0;
    pp = pp * q + 8.74716500200e0;
    pp = pp * q + 5.30324038235e0;
    pp = pp * q + 9.99999999999e-1;

    float pq = 9.24408810559e-4;
    pq = pq * q + 8.56288474354e-2;
    pq = pq * q + 1.25352743901e0;
    pq = pq * q + 5.47097740330e0;
    pq = pq * q + 8.76190883237e0;
    pq = pq * q + 5.30605288235e0;
    pq = pq * q + 1.00000000000e0;

    // QP[0..7] / QQ[0..6] (QQ is monic)
    float qp = -1.13663838898e-2;
    qp = qp * q - 1.28252718671e0;
    qp = qp * q - 1.95539544258e1;
    qp = qp * q - 9.32060152124e1;
    qp = qp * q - 1.77681167980e2;
    qp = qp * q - 1.47077505155e2;
    qp = qp * q - 5.14105326767e1;
    qp = qp * q - 6.05014350601e0;

    float qq = q + 6.43178256118e1;
    qq = qq * q + 8.56430025977e2;
    qq = qq * q + 3.88240183605e3;
    qq = qq * q + 7.24046774196e3;
    qq = qq * q + 5.93072701187e3;
    qq = qq * q + 2.06209331660e3;
    qq = qq * q + 2.42005740240e2;

    float xn = x - PI * 0.25;
    float p = pp / pq;
    float qs = qp / qq;
    return (p * cos(xn) - w * qs * sin(xn)) * SQ2OPI / sqrt(x);
}

// Cephes Z1, Z2: squares of first two zeros of J1
const float J1_Z1 = 14.6819706421;  // 3.8317^2
const float J1_Z2 = 49.2184563217;  // 7.0156^2

float besselJ1(float x) {
    float sign = 1.0;
    if (x < 0.0) { x = -x; sign = -1.0; }
    if (x <= 5.0) {
        float z = x * x;
        if (x < 1.0e-5) return sign * x * 0.5;

        // RP[0..3]
        float rp = -8.99971225706e8;
        rp = rp * z + 4.52228297998e11;
        rp = rp * z - 7.27494245222e13;
        rp = rp * z + 3.68295732864e15;

        // RQ[0..7] (monic)
        float rq = z + 6.20836478118e2;
        rq = rq * z + 2.56987256758e5;
        rq = rq * z + 8.35146791432e7;
        rq = rq * z + 2.21511595480e10;
        rq = rq * z + 4.74914122080e12;
        rq = rq * z + 7.84369607876e14;
        rq = rq * z + 8.95222336185e16;
        rq = rq * z + 5.32278620333e18;

        return sign * x * (z - J1_Z1) * (z - J1_Z2) * rp / rq;
    }

    float w = 5.0 / x;
    float q = w * w;

    // PP[0..6]
    float pp = 7.62125616208e-4;
    pp = pp * q + 7.31397056941e-2;
    pp = pp * q + 1.12719608130e0;
    pp = pp * q + 5.11207951147e0;
    pp = pp * q + 8.42404590142e0;
    pp = pp * q + 5.21451598682e0;
    pp = pp * q + 1.00000000000e0;

    // PQ[0..6]
    float pq = 5.71323128073e-4;
    pq = pq * q + 6.88455908754e-2;
    pq = pq * q + 1.10514232634e0;
    pq = pq * q + 5.07386386129e0;
    pq = pq * q + 8.39985554328e0;
    pq = pq * q + 5.20982848682e0;
    pq = pq * q + 1.00000000000e0;

    // QP[0..7]
    float qp = 5.10862594750e-2;
    qp = qp * q + 4.98213872951e0;
    qp = qp * q + 7.58238284133e1;
    qp = qp * q + 3.66779609360e2;
    qp = qp * q + 7.10856304999e2;
    qp = qp * q + 5.97489612401e2;
    qp = qp * q + 2.11688757101e2;
    qp = qp * q + 2.52070205858e1;

    // QQ[0..6] (monic)
    float qq = q + 7.42373277036e1;
    qq = qq * q + 1.05644886038e3;
    qq = qq * q + 4.98641058338e3;
    qq = qq * q + 9.56231892405e3;
    qq = qq * q + 7.99704160447e3;
    qq = qq * q + 2.82619278518e3;
    qq = qq * q + 3.36093607811e2;

    float xn = x - 3.0 * PI * 0.25;
    float p = pp / pq;
    float qs = qp / qq;
    return sign * (p * cos(xn) - w * qs * sin(xn)) * SQ2OPI / sqrt(x);
}

float besselJ(int n, float x) {
    if (n == 0) return besselJ0(x);
    if (n == 1) return besselJ1(x);
    if (abs(x) < 1.0e-5) return 0.0; // J_n(0) = 0 for n > 0
    float jPrev = besselJ0(x);
    float jCurr = besselJ1(x);
    for (int i = 1; i < n; i++) {
        float jNext = (2.0 * float(i) / x) * jCurr - jPrev;
        jPrev = jCurr;
        jCurr = jNext;
    }
    return jCurr;
}

// 6 orders x 5 zeros = 30 entries, row-major [order][zeroIndex]
const float BESSEL_ZEROS[30] = float[30](
    2.4048, 5.5201, 8.6537, 11.7915, 14.9309,  // J0
    3.8317, 7.0156, 10.1735, 13.3237, 16.4706,  // J1
    5.1356, 8.4172, 11.6198, 14.7960, 17.9598,  // J2
    6.3802, 9.7610, 13.0152, 16.2235, 19.4094,  // J3
    7.5883, 11.0647, 14.3725, 17.6160, 20.8269,  // J4
    8.7715, 12.3386, 15.7002, 18.9801, 22.2178   // J5
);

// Asymptotic approximation for zeros beyond the table
// alpha_{n,m} ~ PI * (m + n/2 - 1/4), accurate < 1% for m >= 3
float besselZero(int n, int m) {
    if (n < 6 && m <= 5) return BESSEL_ZEROS[n * 5 + (m - 1)];
    return PI * (float(m) + float(n) * 0.5 - 0.25);
}

float chladniCircular(vec2 uv, int n, int m, float plateR) {
    float r = length(uv);
    float theta = atan(uv.y, uv.x);
    float alpha = besselZero(n, m);
    return besselJ(n, alpha * r / plateR) * cos(float(n) * theta);
}

// Rectangular plate vibration pattern for mode (n, m)
float chladni(vec2 uv, float n_val, float m_val, float L) {
    float nx = n_val * PI / L;
    float mx = m_val * PI / L;
    return cos(nx * uv.x) * cos(mx * uv.y) - cos(mx * uv.x) * cos(nx * uv.y);
}

void main() {
    float nyquist = sampleRate * 0.5;
    float aspect = resolution.x / resolution.y;
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= aspect;

    float totalPattern = 0.0;
    float totalWeight = 0.0;

    if (plateShape == 1) {
        // Circular plate modes using Bessel functions
        // Resonant frequency: proportional to alpha_{n,m}^2
        // Map lowest mode (0,1) with alpha=2.4048 to baseFreq
        float freqScale = baseFreq / 5.783;  // 2.4048^2

        for (int n = 0; n <= 5; n++) {
            // Determine max m from maxFreq using asymptotic formula
            // alpha_{n,m}^2 <= maxFreq / freqScale
            // PI^2 * (m + n/2 - 1/4)^2 <= maxFreq / freqScale
            float maxArg = sqrt(maxFreq / freqScale) / PI;
            int maxM = int(maxArg - float(n) * 0.5 + 0.25);
            maxM = min(maxM, 20); // GPU loop bound

            for (int m = 1; m <= maxM; m++) {
                float alpha = besselZero(n, m);
                float modeFreq = freqScale * alpha * alpha;
                if (modeFreq < baseFreq || modeFreq > maxFreq) continue;

                float bin = modeFreq / nyquist;
                if (bin > 1.0) continue;

                float mag = texture(fftTexture, vec2(bin, 0.5)).r;
                mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
                mag = pow(mag, mix(1.0, 4.0, coherence));
                if (mag < 0.001) continue;

                totalPattern += mag * chladniCircular(uv, n, m, plateSize);
                totalWeight += mag;
            }
        }
    } else {
        // Rectangular plate modes
        // Resonant frequency of mode (n,m) is proportional to n^2 + m^2.
        // Map so that the lowest mode (1,2) corresponds to baseFreq.
        float lowestModeSum = 5.0; // 1^2 + 2^2
        float freqScale = baseFreq / lowestModeSum;

        // Iterate n and m up to a limit where n^2+m^2 could still be in range
        int maxN = int(sqrt(maxFreq / freqScale)) + 1;
        maxN = min(maxN, 20); // GPU loop bound

        for (int n = 1; n <= maxN; n++) {
            for (int m = 1; m <= maxN; m++) {
                if (n == m) continue; // trivial mode, always zero
                if (n > m) continue;  // avoid duplicate (n,m) and (m,n) — symmetric

                float modeFreq = freqScale * float(n * n + m * m);
                if (modeFreq < baseFreq || modeFreq > maxFreq) continue;

                // Map resonant frequency to FFT bin and sample
                float bin = modeFreq / nyquist;
                if (bin > 1.0) continue;

                float mag = texture(fftTexture, vec2(bin, 0.5)).r;
                mag = pow(clamp(mag * gain, 0.0, 1.0), curve);

                // Coherence sharpens: high coherence suppresses weak modes
                mag = pow(mag, mix(1.0, 4.0, coherence));

                if (mag < 0.001) continue; // skip silent modes entirely

                totalPattern += mag * chladni(uv, float(n), float(m), plateSize);
                totalWeight += mag;
            }
        }
    }

    // Normalize by total weight so brightness doesn't scale with mode count
    if (totalWeight > 0.0) {
        totalPattern /= totalWeight;
    }

    // Nodal emphasis: blend between signed field and absolute value
    float pattern = mix(totalPattern, abs(totalPattern), nodalEmphasis);

    // Color mapping
    float compressed = tanh(pattern * visualGain);
    float t = compressed * 0.5 + 0.5;
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

    // emphasis=0: asymmetric — positive regions bright, negative dark
    // emphasis=1: symmetric — both peaks bright, nodal lines dark
    float signedBright = compressed * 0.5 + 0.5;
    float absBright = abs(compressed);
    float brightness = mix(signedBright, absBright, nodalEmphasis);
    vec4 newColor = vec4(color * brightness, brightness);

    // Fullscreen masking: clip to plate boundary when not fullscreen
    if (fullscreen == 0) {
        float mask;
        if (plateShape == 1) {
            // Circular boundary
            mask = smoothstep(plateSize + 0.02, plateSize - 0.02, length(uv));
        } else {
            // Rectangular boundary
            vec2 edge = smoothstep(vec2(plateSize + 0.02), vec2(plateSize - 0.02), abs(uv));
            mask = edge.x * edge.y;
        }
        newColor *= mask;
    }

    // Trail persistence: diffuse + decay previous frame, keep brighter of old/new
    vec4 existing;
    if (diffusionScale == 0) {
        existing = texture(texture0, fragTexCoord);
    } else {
        vec2 texel = vec2(float(diffusionScale)) / resolution;

        vec4 h = texture(texture0, fragTexCoord + vec2(-2.0 * texel.x, 0.0)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(-1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2( 1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord + vec2( 2.0 * texel.x, 0.0)) * 0.0625;

        vec4 v = texture(texture0, fragTexCoord + vec2(0.0, -2.0 * texel.y)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(0.0, -1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2(0.0,  1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord + vec2(0.0,  2.0 * texel.y)) * 0.0625;

        existing = (h + v) * 0.5;
    }
    existing *= decayFactor;
    finalColor = max(existing, newColor);
}
