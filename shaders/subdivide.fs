// Based on "Always Watching" by SnoopethDuckDuck
// https://www.shadertoy.com/view/ffB3D1
// License: CC BY-NC-SA 3.0 Unported
// Modified: removed faces/stripes/vignette, replaced cosine palette with
// gradient LUT, added FFT audio reactivity, exposed config uniforms
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
uniform float squish;
uniform float threshold;
uniform int maxIterations;
uniform float edgeDarken;
uniform float areaFade;
uniform float desatThreshold;
uniform float desatAmount;
uniform sampler2D gradientLUT;

#define C(u,a,b) cross(vec3(u-a,0), vec3(b-a,0)).z > 0.
#define S(a) smoothstep(2./R.y, -2./R.y, a)

float h11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float h12(vec2 p) {
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float catrom(float t) {
    float f = floor(t),
          x = t - f;
    float v0 = h11(f), v1 = h11(f+1.), v2 = h11(f+2.), v3 = h11(f+3.);
    float c2 = -.5 * v0    + 0.5*v2;
    float c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    float c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

float sdPoly(in vec2[4] v, in vec2 p) {
    float d = dot(p-v[0],p-v[0]);
    float s = 1.0;
    for (int i=0, j=3; i<4; j=i, i++) {
        vec2 e = v[j] - v[i];
        vec2 w =    p - v[i];
        vec2 b = w - e*clamp(dot(w,e)/dot(e,e), 0.0, 1.0);
        d = min(d, dot(b,b));
        bvec3 c = bvec3(p.y>=v[i].y,p.y<v[j].y,e.x*w.y>e.y*w.x);
        if (all(c) || all(not(c))) s*=-1.0;
    }
    return s*sqrt(d);
}

void main() {
    vec2 R = resolution;
    vec2 fc = fragTexCoord * R;
    vec2 u = (fc + fc - R) / R.y;

    // Initialize quad corners to screen bounds (aspect-corrected)
    vec2 tl = vec2(-1, 1) * R / R.y;
    vec2 tr = vec2( 1, 1) * R / R.y;
    vec2 bl = vec2(-1,-1) * R / R.y;
    vec2 br = vec2( 1,-1) * R / R.y;

    float t = time;  // CPU-accumulated with speed

    vec2 ID = vec2(0.0);
    float area;

    // BSP subdivision loop
    for (int i = 0; i < maxIterations; i++) {
        t += 7.3 * h12(ID);

        float k = float(i) + 1.0;
        float K = 1.0 / k;

        // Squish: replaces original's intro-animated `to` variable
        float to = squish * (10.0 - k) * cos(t*k + (u.x+u.y)*k/2.0 + h12(ID)*10.0);
        float mx1 = catrom(t);
        float mx2 = catrom(t + to);
        vec2 x1, x2;

        // Alternate horizontal and vertical cuts
        if (i % 2 == 0) {
            x1 = mix(tl, tr, mx1);
            x2 = mix(bl, br, mx2);
            if (C(u,x1,x2)) { tr = x1; br = x2; ID += vec2(K,0); }
            else             { tl = x1; bl = x2; ID -= vec2(K,0); }
        } else {
            x1 = mix(tl, bl, mx1);
            x2 = mix(tr, br, mx2);
            if (C(u,x1,x2)) { tl = x1; tr = x2; ID += vec2(0,K); }
            else             { bl = x1; br = x2; ID -= vec2(0,K); }
        }

        // Shoelace area
        area = tl.x*bl.y + bl.x*br.y + br.x*tr.y + tr.x*tl.y
             - tl.y*bl.x - bl.y*br.x - br.y*tr.x - tr.y*tl.x;

        // Probabilistic early exit
        if (h12(ID) < threshold) break;
    }

    // Per-cell hash for color and frequency assignment
    float h = h12(ID);

    // FFT brightness: cell hash maps to frequency band
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

    // Edge darkening (configurable intensity)
    float l = max(length(tl-br), length(tr-bl));
    col *= mix(1.0, 0.25 + 0.75 * S(sdPoly(vec2[](tl, bl, br, tr), u) + 0.0033), edgeDarken);
    col *= mix(1.0, 0.75 + 0.25 * S(sdPoly(vec2[](tl, bl, br, tr), u + 0.07*l)), edgeDarken);

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
