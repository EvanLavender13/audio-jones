// Based on "Light Medley [353]" by diatribes (golfed by @bug)
// https://www.shadertoy.com/view/3cXBWB
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT-per-step coloring via gradient LUT

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float flyPhase;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float swirlAmount;
uniform float swirlRate;
uniform float swirlPhase;
uniform float twistRate;
uniform int slabMode;
uniform int latticeMode;
uniform int swirlPerm;
uniform int swirlFunc;
uniform float slabFreq;
uniform float latticeScale;
uniform vec2 pan;
uniform float glowIntensity;

#define MAX_STEPS 64
#define BAND_SAMPLES 4

void main() {
    // UV: centered, aspect-corrected, with Lissajous camera pan
    vec2 uv = (gl_FragCoord.xy * 2.0 - resolution) / resolution.y
             + pan;

    float d = 0.4;      // initial march distance
    vec3 c = vec3(0.0);  // color accumulator

    for (int i = 0; i < MAX_STEPS; i++) {
        // Build 3D sample point: XY from UV * depth, Z advances with depth + scaled time
        vec3 p = vec3(uv * d, d + flyPhase) - 3.0;

        // Coordinate swirl: each axis displaced by a function of another axis
        vec3 perm;
        if (swirlPerm == 1)      { perm = p.zxy; }
        else if (swirlPerm == 2) { perm = p.xzy; }
        else                     { perm = p.yzx; }
        vec3 swirlArg = swirlPhase + perm * swirlRate;
        vec3 swirlVal;
        if (swirlFunc == 1)      { swirlVal = sin(swirlArg); }
        else if (swirlFunc == 2) { swirlVal = abs(fract(swirlArg / 6.2832) - 0.5) * 4.0 - 1.0; }
        else if (swirlFunc == 3) { swirlVal = abs(sin(swirlArg)); }
        else                     { swirlVal = cos(swirlArg); }
        p += swirlVal * swirlAmount;

        // Z-dependent XY rotation (33 ~ pi/2 mod 2pi, 11 ~ 3pi/2 mod 2pi -> proper rotation matrix)
        p.xy *= mat2(cos(twistRate * p.z + vec4(0, 33, 11, 0)));

        // Slab shape
        float slab;
        if (slabMode == 1)      { slab = cos(p.y * slabFreq); }
        else if (slabMode == 2) { slab = cos(p.z * slabFreq); }
        else if (slabMode == 3) { slab = cos(length(p.xy) * slabFreq); }
        else if (slabMode == 4) { slab = cos(length(p.xz) * slabFreq); }
        else if (slabMode == 5) { slab = cos(length(p) * slabFreq); }
        else if (slabMode == 6) { slab = cos(dot(p, vec3(0.577)) * slabFreq); }
        else                    { slab = cos(p.x * slabFreq); }

        // Lattice type
        vec3 f = abs(fract(p * latticeScale) - 0.5);
        float lattice;
        if (latticeMode == 1)      { lattice = max(max(f.x, f.y), f.z); }
        else if (latticeMode == 2) { lattice = length(f.xy); }
        else                       { lattice = dot(f, vec3(0.25)); }

        float s = max(slab, lattice);
        d += s;

        // Per-step FFT band sampling
        float t0 = float(i) / float(MAX_STEPS);
        float t1 = float(i + 1) / float(MAX_STEPS);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float mag = 0.0;
        for (int b = 0; b < BAND_SAMPLES; b++) {
            float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0)
                mag += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        mag = pow(clamp(mag / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Gradient color from march depth position
        vec3 color = texture(gradientLUT, vec2(t0, 0.5)).rgb;

        // Accumulate: gradient color * FFT brightness * inverse distance * glow
        c += color * brightness / max(s, 0.01) * glowIntensity;
    }

    // tanh tonemap - empirical normalization constant
    #define TONEMAP_NORM 2e6
    finalColor = vec4(tanh(c * c / TONEMAP_NORM), 1.0);
}
