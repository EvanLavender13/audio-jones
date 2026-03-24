// Protean Clouds - volumetric raymarched clouds with gradient LUT coloring
// Based on "Protean clouds" by nimitz (@stormoid)
// https://www.shadertoy.com/view/3l23Rh
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT coloring, FFT brightness, parametric fog, AudioJones uniforms

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float flyPhase;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float morph;
uniform float colorBlend;
uniform float fogIntensity;
uniform float turbulence;
uniform float densityThreshold;
uniform int marchSteps;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform int octaves;
uniform float rollAngle;
uniform float driftAmplitude;
uniform float driftFreqX1, driftFreqY1;
uniform float driftFreqX2, driftFreqY2;
uniform float driftOffsetX2, driftOffsetY2;

#define MAX_STEPS 130
#define MAX_DIST 50.0
mat2 rot(in float a) { float c = cos(a), s = sin(a); return mat2(c, s, -s, c); }
const mat3 m3 = mat3(0.33338, 0.56034, -0.71817, -0.87887, 0.32651, -0.15323,
                     0.15162, 0.69596, 0.61339) * 1.93;
float mag2(vec2 p) { return dot(p, p); }
float linstep(in float mn, in float mx, in float x) {
    return clamp((x - mn) / (mx - mn), 0., 1.);
}

vec2 disp(float t) {
    vec2 d = vec2(sin(t * driftFreqX1), cos(t * driftFreqY1));
    if (driftFreqX2 > 0.0) d.x += sin(t * driftFreqX2 + driftOffsetX2);
    if (driftFreqY2 > 0.0) d.y += cos(t * driftFreqY2 + driftOffsetY2);
    return d * driftAmplitude;
}

float sampleFFTBand(float t) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t);
    float bw = 1.0 / 8.0;
    float freqHi = baseFreq * pow(maxFreq / baseFreq, min(t + bw, 1.0));
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
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return baseBright + mag;
}

vec2 map(vec3 p) {
    vec3 p2 = p;
    p2.xy -= disp(p.z).xy;
    p.xy *= rot(sin(p.z + time) * (0.1 + morph * 0.05) + time * 0.09);
    float cl = mag2(p2.xy);
    float d = 0.;
    p *= .61;
    float z = 1.;
    float trk = 1.;
    float dspAmp = turbulence;
    for (int i = 0; i < octaves; i++) {
        p += sin(p.zxy * 0.75 * trk + time * trk * .8) * dspAmp;
        d -= abs(dot(cos(p), sin(p.yzx)) * z);
        z *= 0.57;
        trk *= 1.4;
        p = p * m3;
    }
    d = abs(d + morph * 3.) + morph * 0.3 - 2.5;
    return vec2(d + cl * .2 + 0.25, cl);
}

vec4 render(in vec3 ro, in vec3 rd) {
    vec4 rez = vec4(0);
    float t = 1.5;
    float fogT = 0.;
    for (int i = 0; i < MAX_STEPS; i++) {
        if (i >= marchSteps) break;
        if (rez.a > 0.99) break;

        vec3 pos = ro + t * rd;
        vec2 mpv = map(pos);
        float den = clamp(mpv.x - densityThreshold, 0., 1.) * 1.12;
        float dn = clamp((mpv.x + 2.), 0., 3.);

        float lutIndex = mix(den, t / MAX_DIST, colorBlend);
        vec4 col = vec4(0);
        if (mpv.x > densityThreshold + 0.3) {
            float brightness = sampleFFTBand(t / MAX_DIST);
            col = vec4(texture(gradientLUT, vec2(lutIndex, 0.5)).rgb, 0.08);
            col *= den * den * den;
            col.rgb *= linstep(4., -2.5, mpv.x) * 2.3;
            float dif = clamp((den - map(pos + .8).x) / 9., 0.001, 1.);
            dif += clamp((den - map(pos + .35).x) / 2.5, 0.001, 1.);
            col.rgb *= den * brightness * (0.03 + 0.1 * dif);
        }

        float fogC = exp(t * 0.2 - 2.2);
        col.rgba += vec4(texture(gradientLUT, vec2(lutIndex, 0.5)).rgb * 0.1, 0.06)
                    * fogIntensity * clamp(fogC - fogT, 0., 1.);
        fogT = fogC;
        rez = rez + col * (1. - rez.a);
        t += clamp(0.5 - dn * dn * .05, 0.09, 0.3);
    }
    return clamp(rez, 0.0, 1.0);
}

void main() {
    vec2 p = (fragTexCoord * resolution - 0.5 * resolution) / resolution.y;

    vec3 ro = vec3(0, 0, flyPhase);
    ro += vec3(sin(time) * 0.5, 0., 0.);

    ro.xy += disp(ro.z);
    float tgtDst = 3.5;

    vec3 target = normalize(ro - vec3(disp(flyPhase + tgtDst), flyPhase + tgtDst));
    vec3 rightdir = normalize(cross(target, vec3(0, 1, 0)));
    vec3 updir = normalize(cross(rightdir, target));
    rightdir = normalize(cross(updir, target));
    p *= rot(rollAngle);
    vec3 rd = normalize((p.x * rightdir + p.y * updir) - target);

    vec4 scn = render(ro, rd);
    finalColor = vec4(scn.rgb, 1.0);
}
