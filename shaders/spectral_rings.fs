// Spectral Rings — dense concentric rings with FFT-reactive brightness
// Based on "Rings [324 chars]" by XorDev (shadertoy.com/view/sstyDX), CC BY-NC-SA 3.0
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D noiseTex;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float time;
uniform float noiseScale;
uniform int quality;
uniform float pulseAccum;
uniform float colorShiftAccum;
uniform float rotationAccum;
uniform float eccentricity;
uniform float tiltAngle;
uniform vec2 centerOffset; // lissajous drift in normalized coords
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    vec2 r = resolution;
    vec2 I = fragTexCoord * r;
    vec2 i = vec2(1.0);
    vec2 d = vec2((I.x - 2.0 * I.y + r.y * 0.2) / 2000.0);
    vec2 p, l;
    float ns = noiseScale;
    float logRatio = log(maxFreq / baseFreq);

    // Elliptical stretch: eccentricity scales one axis, tiltAngle rotates it
    float ca = cos(tiltAngle), sa = sin(tiltAngle);
    mat2 rot = mat2(ca, sa, -sa, ca);
    mat2 invRot = mat2(ca, -sa, sa, ca);
    float stretch = 1.0 + eccentricity * 3.0;
    mat2 M = invRot * mat2(stretch, 0, 0, 1.0) * rot;

    // Transform screen center through M, then add lissajous in screen space
    vec2 transformedCenter = (r * 0.5) * M / r.y + centerOffset;

    // Iteration bound from quality
    float maxI = sqrt(2.0 * float(quality) + 1.0);

    vec3 O = vec3(0.0);

    for (; i.x < maxI; i += 1.0 / i) {
        l = length(p = (I + d * i) * M / r.y - transformedCenter) / vec2(3, 8);
        d *= -mat2(73, 67, -67, 73) / 99.0;

        // FFT brightness — band sampling around radial frequency
        float t0 = clamp(l.x - 0.02, 0.0, 1.0);
        float t1 = clamp(l.x + 0.02, 0.0, 1.0);
        float freqLo = baseFreq * exp(t0 * logRatio);
        float freqHi = baseFreq * exp(t1 * logRatio);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);
        float energy = 0.0;
        for (int s = 0; s < 4; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / 4.0);
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        float bright = baseBright + pow(clamp(energy * 0.25 * gain, 0.0, 1.0), curve);

        // Noise → gradient LUT color
        float nR = texture(noiseTex, (l + pulseAccum * 0.01) * ns).r;
        float nC = texture(noiseTex, p * mat2(cos(rotationAccum + vec4(0, 33, 11, 0))) * ns).r;
        vec3 radial = texture(gradientLUT, vec2(fract(nR + colorShiftAccum), 0.5)).rgb;
        vec3 cart = texture(gradientLUT, vec2(fract(nC + colorShiftAccum), 0.5)).rgb;

        // Reference per-channel pow (5,8,9) with clamped radial falloff
        vec3 contrib = radial * cart * bright / max(l.x, 0.15) * 0.4;
        O += pow(max(contrib, vec3(0.0)), vec3(5, 8, 9));
    }

    // Fifth root contrast (reference)
    finalColor = vec4(pow(max(O, vec3(0.0)), vec3(0.2)), 1.0);
}
