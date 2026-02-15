// Spectral Arcs: Cosmic-style tilted concentric ring arcs driven by FFT semitone energy.
// Core visual technique from XorDev's "Cosmic" shader — perspective tilt, cos() multi-arc
// clipping, per-ring rotation via sin(i*i), inverse-distance glow. FFT magnitude modulates
// per-ring brightness so active notes flare.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform int rings;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float ringScale;
uniform float tilt;
uniform float tiltAngle;
uniform float arcWidth;
uniform float glowIntensity;
uniform float glowFalloff;
uniform float baseBright;
uniform float rotationAccum;

void main() {
    vec2 r = resolution;
    vec2 fc = fragTexCoord * r;

    vec3 result = vec3(0.0);

    // Rotate centered coords by tiltAngle, then apply perspective tilt.
    // tiltAngle controls which direction the disc tilts, tilt controls how much.
    vec2 centered = fc - r * 0.5;
    float ca = cos(tiltAngle), sa = sin(tiltAngle);
    vec2 rotated = vec2(centered.x * ca - centered.y * sa,
                        centered.x * sa + centered.y * ca);
    mat2 tiltMat = mat2(1.0, -tilt, 2.0 * tilt, 1.0 + tilt);
    vec2 p = rotated * tiltMat;

    // Perspective projection: depth varies with y for tilted-disc look
    float depth = r.y + r.y - p.y * tilt;
    vec2 uv = p / depth;

    int totalRings = rings;
    float ft = float(totalRings);

    for (int i = 0; i < totalRings; i++) {
        float fi = float(i) + 1.0;

        // FFT semitone lookup: index -> frequency -> normalized bin
        float freq = baseFreq * pow(maxFreq / baseFreq, float(i) / float(rings - 1));
        float bin = freq / (sampleRate * 0.5);
        float mag = 0.0;
        if (bin <= 1.0) {
            mag = texture(fftTexture, vec2(bin, 0.5)).r;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
        }

        // Ring glow: inverse distance to ring fi (Cosmic technique)
        float dist = length(uv) * ft * ringScale - fi;
        float glow = glowIntensity / (abs(dist) + glowFalloff / r.y);

        // Multi-segment arc clipping: inner rings 1 arc, middle 2, outer 3
        float segments = ceil(fi * 3.0 / ft);
        float a = atan(uv.y, uv.x) * segments
                + rotationAccum * sin(fi * fi)
                + fi * fi;
        float arcMask = clamp(cos(a), 0.0, arcWidth);

        // Color from gradient LUT by ring position
        float t = float(i) / float(rings);
        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // Accumulate: baseline glow + FFT-driven brightness boost
        result += glow * arcMask * color * (baseBright + mag);
    }

    finalColor = vec4(result, 1.0);
}
