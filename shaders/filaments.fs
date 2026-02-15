// Filaments: Tangled radial line segments driven by FFT energy.
// Rotating endpoint geometry, per-segment FFT brightness, additive glow.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform int filaments;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float radius;
uniform float spread;
uniform float stepAngle;
uniform float glowIntensity;
uniform float baseBright;
uniform float rotationAccum;

const float GLOW_WIDTH = 0.002;

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, s, -s, c);
}

// Point-to-segment distance (IQ sdSegment)
float segm(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 r = resolution;
    vec2 p = (fragTexCoord * r * 2.0 - r) / min(r.x, r.y);

    vec3 result = vec3(0.0);
    int totalFilaments = filaments;

    vec2 p1 = vec2(-radius, 0.0);

    for (int i = 0; i < totalFilaments; i++) {
        // Sign-preserving pow: avoid NaN from pow(negative, 1.5)
        float raPow = sign(rotationAccum) * pow(abs(rotationAccum), 1.5);
        p1 *= rot(stepAngle + raPow * 0.0007);

        vec2 p2 = p1 * rot(spread * float(i) - rotationAccum);

        // FFT band-averaging lookup
        float t0 = float(i) / float(totalFilaments);
        float t1 = float(i + 1) / float(totalFilaments);
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

        float dist = segm(p, p1, p2);

        float glow = GLOW_WIDTH / (GLOW_WIDTH + dist);

        float colorT = float(i) / float(totalFilaments);
        vec3 color = texture(gradientLUT, vec2(colorT, 0.5)).rgb;

        result += color * glow * glowIntensity * (baseBright + mag);
    }

    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
