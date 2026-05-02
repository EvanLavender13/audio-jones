// Based on "Random Volumetric V2" by Cotterzz
// https://www.shadertoy.com/view/3XGXRW
// License: CC BY-NC-SA 3.0 Unported

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float time;
uniform float cameraPhase;

uniform float seed;
uniform float cyclePhase;
uniform float iterMin;
uniform float iterMax;
uniform float zoom;
uniform float zoomPulse;

uniform float paletteRandomness;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define rot(a) mat2(cos(a + vec4(0,33,11,0)))

vec3 P(float z) {
    return vec3(tanh(cos(z * .15) * 1.) * 8.0,
                tanh(cos(z * .12) * 1.) * 8.0,
                z);
}

float hash1(vec2 x) {
    uvec2 t = floatBitsToUint(x);
    uint h = 0xc2b2ae3du * t.x + 0x165667b9u;
    h = (h << 17u | h >> 15u) * 0x27d4eb2fu;
    h += 0xc2b2ae3du * t.y;
    h = (h << 17u | h >> 15u) * 0x27d4eb2fu;
    h ^= h >> 15u;
    h *= 0x85ebca77u;
    h ^= h >> 13u;
    h *= 0xc2b2ae3du;
    h ^= h >> 16u;
    return uintBitsToFloat(h >> 9u | 0x3f800000u) - 1.0;
}

vec2 hash2(vec2 x) {
    float k = 6.283185307 * hash1(x);
    return vec2(cos(k), sin(k));
}

float simplex2(vec2 p) {
    const float K1 = 0.366025404;
    const float K2 = 0.211324865;
    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    float m = step(a.y, a.x);
    vec2 o = vec2(m, 1.0 - m);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;
    vec3 h = max(0.5 - vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0);
    vec3 n = h*h*h * vec3(dot(a, hash2(i + 0.0)),
                           dot(b, hash2(i + o)),
                           dot(c, hash2(i + 1.0)));
    return dot(n, vec3(32.99));
}

float pnoise(vec2 uv) {
    float f = 0.0;
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    f  = 0.5000 * simplex2(uv); uv = m * uv;
    f += 0.2500 * simplex2(uv); uv = m * uv;
    f += 0.1250 * simplex2(uv); uv = m * uv;
    f += 0.0625 * simplex2(uv);
    f = 0.5 + 0.5 * f;
    return f;
}

float rand(float n) {
    return fract(cos(n * 89.42) * 343.42);
}

float map(vec3 p) {
    return 1.5 - length(p - P(p.z));
}

float rMix(float a, float b, float s) {
    s = rand(s);
    return s>0.9?sin(a):s>0.8?sqrt(abs(a)):s>0.7?a+b:s>0.6?a-b:s>0.5?b-a:s>0.4?b/(a==0.?0.01:a):s>0.3?pnoise(vec2(a,b)):s>0.2?a/(b==0.?0.01:b):s>0.1?a*b:cos(a);
}

vec3 hsl2rgb(in vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

vec3 fhexRGB(float fh) {
    if(isinf(fh) || fh > 100000.) { fh = 0.; }
    fh = abs(fh * 10000000.);
    float r = fract(fh / 65536.);
    float g = fract(fh / 256.);
    float b = fract(fh / 16777216.);
    return hsl2rgb(vec3(r,g,b));
}

int PALETTE = 0;

vec3 addColor(float num, float seedV, float alt) {
    if(isinf(num)) { num = alt * seedV; }
    if(PALETTE == 7) {
        return fhexRGB(num);
    } else if(PALETTE > 2 || (PALETTE == 1 && rand(seedV+19.) > 0.3)) {
        float sat = 1.;
        if(num < 0.) { sat = 1. - (1./(abs(num)+1.)); }
        float light = 1.0 - (1./(abs(num)+1.));
        vec3 col = hsl2rgb(vec3(fract(abs(num)), sat, light));
        if(PALETTE == 1) { col *= 2.; }
        return col;
    } else {
        vec3 col = vec3(fract(abs(num)), 1./num, 1.-fract(abs(num)));
        if(rand(seedV*2.) > 0.5) { col = col.gbr; }
        if(rand(seedV*3.) > 0.5) { col = col.gbr; }
        if(PALETTE == 1) { col += (1.+cos(rand(num)+vec3(4,2,1))) / 2.; }
        return col;
    }
}

vec3 sanitize(vec3 dc) {
    dc.r = min(1., abs(dc.r));
    dc.g = min(1., abs(dc.g));
    dc.b = min(1., abs(dc.b));
    if(!(dc.r >= 0.) && !(dc.r < 0.)) { return vec3(1,0,0); }
    else if(!(dc.g >= 0.) && !(dc.g < 0.)) { return vec3(1,0,0); }
    else if(!(dc.b >= 0.) && !(dc.b < 0.)) { return vec3(1,0,0); }
    return dc;
}

void main() {
    vec3 r = vec3(resolution, 0.);
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 u = (fragCoord - r.xy/2.) / r.y;

    float s = .002, d = 0.;
    vec3 p = P(cameraPhase), ro = p,
         Z = normalize(P(cameraPhase + 1.) - p),
         X = normalize(vec3(Z.z, 0, -Z.x)),
         D = vec3(rot(tanh(sin(p.z*.03)*8.)*3.0) * u, 1)
             * mat3(-X, cross(X, Z), Z);
    for(int rm = 0; rm < 80 && s > .001; rm++) {
        p = ro + D * d;
        s = map(p) * .8;
        d += s;
    }
    p = ro + D * d;

    vec2 uv = fragCoord / resolution.y;
    uv.x -= 0.5 * resolution.x / resolution.y;
    uv.y -= 0.5;
    float zoomVal = zoom + (zoomPulse * (sin(time) + 1.));
    vec2 guv = uv * zoomVal;
    float x = guv.x;
    float y = guv.y;

    float effSeed = seed + floor(cyclePhase);
    PALETTE = int(floor(8.0 * rand(effSeed + 66.0)));

    const int v = 24;
    float values[v];
    values[0]  = 1.0;
    values[1]  = p.x;
    values[2]  = x;
    values[3]  = y;
    values[4]  = x*x;
    values[5]  = y*y;
    values[6]  = x*x*x;
    values[7]  = y*y*y;
    values[8]  = x*x*x*x;
    values[9]  = y*y*y*y;
    values[10] = x*y*x;
    values[11] = y*y*x;
    values[12] = sin(y);
    values[13] = cos(y);
    values[14] = sin(x);
    values[15] = cos(x);
    values[16] = sin(y)*sin(y);
    values[17] = cos(y)*cos(y);
    values[16] = sin(x)*sin(x);
    values[17] = cos(x)*cos(x);
    values[18] = p.y;
    values[19] = distance(vec2(x,y), vec2(0));
    values[20] = p.z;
    values[21] = atan(x, y) * 4.;
    values[22] = pnoise(vec2(x,y) / 2.);
    values[23] = pnoise(vec2(y,x) * 10.);

    int iterMaxI = int(iterMax);
    int iterMinI = int(iterMin);
    int iterations = min(iterMaxI,
                         iterMinI + int(floor(rand(effSeed * 6.6) * float(iterMaxI - iterMinI))));

    float total = 0.;
    float sub = 0.;
    vec3 stochCol = vec3(0);
    float cn = 1.;

    for(int i = 0; i < iterations; i++) {
        float NEWVALUE  = values[int(floor(float(v) * rand(effSeed + float(i))))]
                          * (sin(time * rand(effSeed + float(i))) * rand(effSeed + float(i)));
        float NEWVALUE2 = values[int(floor(float(v) * rand(effSeed + float(i+5))))]
                          * (sin(time * rand(effSeed + float(i))) * rand(effSeed + float(i+5)));

        if(rand(effSeed + float(i+3)) > rand(effSeed)) {
            sub = sub == 0. ? rMix(NEWVALUE, NEWVALUE2, effSeed + float(i+4))
                            : rMix(sub, rMix(NEWVALUE, NEWVALUE2, effSeed + float(i+4)),
                                   effSeed + float(i));
        } else {
            sub = sub == 0. ? NEWVALUE
                            : rMix(sub, NEWVALUE, effSeed + float(i));
        }
        if(rand(effSeed + float(i)) > rand(effSeed) / 2.) {
            total = total == 0. ? sub : rMix(total, sub, effSeed + float(i*2));
            sub = 0.;
            if(rand(effSeed + float(i+30)) > rand(effSeed)) {
                stochCol += addColor(total, effSeed + float(i), values[21]);
                cn += 1.;
            }
        }
    }
    total = sub == 0. ? total : rMix(total, sub, effSeed);
    stochCol += addColor(total, effSeed, values[21]);
    stochCol /= cn;

    if(PALETTE < 3) { stochCol /= (3. * (0.5 + rand(effSeed + 13.))); }
    if(PALETTE == 4) { stochCol = pow(stochCol, 1./stochCol) * 1.5; }
    if(PALETTE == 2 || PALETTE == 5) { stochCol = hsl2rgb(stochCol); }
    if(PALETTE == 6) {
        stochCol = hsl2rgb(hsl2rgb(stochCol));
        if(rand(effSeed + 17.) > 0.5) { stochCol = stochCol.gbr; }
        if(rand(effSeed + 19.) > 0.5) { stochCol = stochCol.gbr; }
    }

    stochCol = sanitize(stochCol);

    float t = fract(abs(total));
    vec3 lutCol = texture(gradientLUT, vec2(t, 0.5)).rgb;
    vec3 col = lutCol + paletteRandomness * (stochCol - 0.5);

    float t0 = max(t - 0.125, 0.0);
    float t1 = min(t + 0.125, 1.0);
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int sIdx = 0; sIdx < BAND_SAMPLES; sIdx++) {
        float bin = mix(binLo, binHi, (float(sIdx) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    col = clamp(col * (baseBright + mag), 0.0, 1.0);

    finalColor = vec4(col, 1.0);
}
