// Spectral Rings: Concentric ring pattern with FFT-reactive brightness,
// gradient LUT coloring, and value noise perturbation.
// Based on XorDev's work (shadertoy.com/view/sstyDX), CC BY-NC-SA 3.0.
// Modified for parameterized generator with FFT/LUT integration.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float time;
uniform float ringDensity;
uniform float ringWidth;
uniform int layers;
uniform float pulseAccum;
uniform float colorShiftAccum;
uniform float rotationAccum;
uniform float eccentricity;
uniform float skewAngle;
uniform float noiseAmount;
uniform float noiseScale;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define PI 3.14159265359

// --- Noise utilities (inline) ---

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// --- Main ---

void main() {
    // 1. Center coordinates (convention: origin at screen center)
    vec2 pos = fragTexCoord * resolution - resolution * 0.5;

    // 2. Apply eccentricity (parameterized from reference's mat2(2,1,-2,4))
    //    skewAngle rotates the stretch axis; eccentricity controls magnitude
    float cs = cos(skewAngle), sn = sin(skewAngle);
    mat2 eccMat = mat2(1.0 + eccentricity * cs, eccentricity * sn,
                       -eccentricity * sn, 1.0 - eccentricity * cs);
    pos = eccMat * pos;

    // 3. Apply rotation (CPU-accumulated from rotationSpeed)
    float rc = cos(rotationAccum), rs = sin(rotationAccum);
    pos = mat2(rc, rs, -rs, rc) * pos;

    // 4. Radial distance normalized to [0,1]
    float maxRadius = min(resolution.x, resolution.y) * 0.5;
    float dist = length(pos) / maxRadius;

    // 5. Ring band brightness
    //    fract() creates repeating bands; pulseAccum shifts them outward/inward
    //    smoothstep edges give sharp rings with anti-aliased transitions
    float band = fract(dist * ringDensity + pulseAccum);
    float aa = fwidth(dist * ringDensity);
    float ringBright = smoothstep(0.0, aa, band)
                     * (1.0 - smoothstep(ringWidth - aa, ringWidth, band));

    // 6. FFT mapping — quantize distance into layers frequency bands
    //    Each pixel's radial distance maps to a discrete frequency band
    //    BAND_SAMPLES sub-samples within each band for smooth FFT reading
    float fl = float(layers);
    float bandIdx = clamp(floor(dist * fl), 0.0, fl - 1.0);
    float t0 = bandIdx / fl;
    float t1 = (bandIdx + 1.0) / fl;
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
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
    float fftBright = baseBright + mag;

    // 7. Color sampling with noise perturbation
    //    LUT position scrolls independently of ring pulse via colorShiftAccum
    //    noise2D breaks uniform banding for organic variation
    float lutU = dist * ringDensity + colorShiftAccum;
    vec2 noisePos = (pos / maxRadius) * noiseScale + vec2(time * 0.1);
    lutU += noise2D(noisePos) * noiseAmount;
    vec3 color = texture(gradientLUT, vec2(fract(lutU), 0.5)).rgb;

    // 8. Composite + fifth-root contrast (from reference's pow(O, 0.2))
    vec3 result = color * ringBright * fftBright;
    finalColor = vec4(pow(result, vec3(0.2)), 1.0);
}
