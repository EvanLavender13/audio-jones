// Based on "Convex Polygon Subdivision" by SnoopethDuckDuck
// https://www.shadertoy.com/view/NfBGzd
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced palette with gradient LUT, added FFT audio reactivity,
// exposed config uniforms, removed intro animation and vignette

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float threshold;
uniform int maxIterations;
uniform float edgeDarken;
uniform float areaFade;
uniform float desatThreshold;
uniform float desatAmount;
uniform sampler2D gradientLUT;

#define C(a,b) ((a).x*(b).y-(a).y*(b).x)
#define S(a) smoothstep(2./R.y, -2./R.y, a)

const int maxN = 15;

vec2 center(vec2[maxN] P, int N) {
    vec2 bl = vec2(1e5), tr = vec2(-1e5);
    for (int j = 0; j < N; j++) {
        bl = min(bl, P[j]);
        tr = max(tr, P[j]);
    }
    return (bl + tr) / 2.0;
}

float h11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float h12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 h21(float p) {
    vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float catrom1(float t) {
    float f = floor(t),
          x = t - f;
    float v0 = h11(f), v1 = h11(f+1.), v2 = h11(f+2.), v3 = h11(f+3.);
    float c2 = -.5 * v0    + 0.5*v2;
    float c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    float c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

vec2 catrom2(float t) {
    float f = floor(t),
          x = t - f;
    vec2 v0 = h21(f), v1 = h21(f+1.), v2 = h21(f+2.), v3 = h21(f+3.);
    vec2 c2 = -.5 * v0    + 0.5*v2;
    vec2 c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    vec2 c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

float sdPoly(in vec2[maxN] v, int N, in vec2 p) {
    float d = dot(p-v[0], p-v[0]);
    float s = 1.0;
    for (int i = 0, j = N-1; i < N; j = i, i++) {
        vec2 e = v[j] - v[i];
        vec2 w = p - v[i];
        vec2 b = w - e * clamp(dot(w,e)/dot(e,e), 0.0, 1.0);
        d = min(d, dot(b,b));
        bvec3 c = bvec3(p.y>=v[i].y, p.y<v[j].y, e.x*w.y>e.y*w.x);
        if (all(c) || all(not(c))) { s *= -1.0; }
    }
    return s * sqrt(d);
}

void main() {
    vec2 R = resolution;
    vec2 fc = fragTexCoord * R;
    vec2 u = (fc + fc - R) / R.y;

    vec2 e;
    vec2[maxN] P = vec2[](
        vec2(-1, 1) * R / R.y,
        vec2( 1, 1) * R / R.y,
        vec2( 1,-1) * R / R.y,
        vec2(-1,-1) * R / R.y,
        e, e, e, e, e, e, e, e, e, e, e);
    int N = 4;
    float id = 0.0;

    float t = time;

    for (int i = 0; i < maxIterations; i++) {
        vec2 c = center(P, N);

        vec2 cr = catrom2(t);
        float d = sdPoly(P, N, c);
        vec2 p0 = c + (cr - 0.5) * d;

        float a = 2.0 * 6.28 * catrom1(t / 5.0);
        vec2 p1 = p0 + 2.0 * vec2(cos(a), sin(a));

        // First line-polygon intersection
        int i0; vec2 q0;
        for (int j = 0; j < N; j++) {
            vec2 a = P[j], b = P[j + 1];
            float s = C(p1 - p0, p0 - a) / C(p1 - p0, b - a);
            if (s >= 0.0 && s < 1.0) { i0 = j; q0 = mix(a, b, s); break; }
        }

        // Second line-polygon intersection
        int i1; vec2 q1;
        for (int j = i0 + 1; j < N; j++) {
            vec2 a = P[j], b = P[(j + 1) % N];
            float s = C(p1 - p0, p0 - a) / C(p1 - p0, b - a);
            if (s >= 0.0 && s < 1.0) { i1 = j; q1 = mix(a, b, s); break; }
        }

        // Ensure p1 is closer to q0 than q1
        int ti; vec2 tq;
        if (dot(p1 - q0, p1 - q0) > dot(p1 - q1, p1 - q1)) {
            ti = i0; i0 = i1; i1 = ti;
            tq = q0; q0 = q1; q1 = tq;
        }

        // Split polygon based on which side of the line u falls on
        vec2[maxN] Q;
        if (C(u - p0, p1 - p0) > 0.0) {
            int M = i0 < i1 ? (i1 - i0) + 2 : N - (i0 - i1) + 2;
            Q[0] = q0;
            for (int j = 1; j < M - 1; j++) { Q[j] = P[(i0 + j) % N]; }
            Q[M - 1] = q1;
            N = M;
            id += 1.0 / float(i + 1);
        } else {
            int M = i1 < i0 ? (i0 - i1) + 2 : N - (i1 - i0) + 2;
            Q[0] = q1;
            for (int j = 1; j < M - 1; j++) { Q[j] = P[(i1 + j) % N]; }
            Q[M - 1] = q0;
            N = M;
            id -= 1.0 / float(i + 1);
        }
        P = Q;

        t += 1e2 * (h11(id) - 0.5);

        // Procedural hash replaces noise texture; threshold replaces hardcoded 0.1
        float tx = h12(u * 0.11);
        if (h11(id + floor(t / 1.5 - tx / 8.0)) < threshold) { break; }
    }

    // Bounding box for area fade and edge diagonal
    vec2 bl = vec2(1e5);
    vec2 tr = vec2(-1e5);
    for (int j = 0; j < N; j++) {
        bl = min(bl, P[j]);
        tr = max(tr, P[j]);
    }
    float area = (tr.x - bl.x) * (tr.y - bl.y);

    float h = h11(id);

    // FFT brightness
    float freqRatio = maxFreq / baseFreq;
    float bandWidth = 0.05;
    float t0 = max(0.0, h - bandWidth);
    float t1 = min(1.0, h + bandWidth);
    float freqLo = baseFreq * pow(freqRatio, t0);
    float freqHi = baseFreq * pow(freqRatio, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float brightness = baseBright + pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

    // Gradient LUT coloring
    vec3 col = texture(gradientLUT, vec2(h, 0.5)).rgb * brightness;

    // Edge darkening (two-pass sdPoly, matching subdivide.fs style)
    float l = length(tr - bl);
    col *= mix(1.0, 0.25 + 0.75 * S(sdPoly(P, N, u) + 0.0033), edgeDarken);
    col *= mix(1.0, 0.75 + 0.25 * S(sdPoly(P, N, u + 0.07 * l)), edgeDarken);

    // Desaturation gate
    if (h > desatThreshold) {
        float lum = dot(col, vec3(0.299, 0.587, 0.114));
        col = mix(col, vec3(lum), desatAmount);
    }

    col = clamp(col, 0.0, 1.0);

    // Area fade: tiny cells dissolve to black
    vec3 bg = vec3(0.0);
    col = mix(bg, col, exp(-areaFade / area));

    finalColor = vec4(col, 1.0);
}
