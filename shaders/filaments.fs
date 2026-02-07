// Filaments: Tangled radial line segments driven by FFT semitone energy.
// Adapted from nimitz's "Filaments" (shadertoy.com/view/4lcSWs) — rotating
// endpoint geometry, triangle-wave noise, per-segment FFT warp, additive glow.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform int numOctaves;
uniform float gain;
uniform float curve;
uniform float radius;
uniform float spread;
uniform float stepAngle;
uniform float glowIntensity;
uniform float falloffExponent;
uniform float baseBright;
uniform float noiseStrength;
uniform float noiseTime;
uniform float rotationAccum;

const float EPSILON = 0.0001;

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, s, -s, c);
}

float tri(float x) { return abs(fract(x) - 0.5); }

vec2 tri2(vec2 p) {
    return vec2(tri(p.x + tri(p.y * 2.0)), tri(p.y + tri(p.x * 2.0)));
}

// Triangle-wave fBM noise scalar (nimitz) — does NOT modify caller's p
float triangleNoise(vec2 p) {
    float z = 1.5;
    float z2 = 1.5;
    float rz = 0.0;
    vec2 bp = p * 0.8;
    for (int i = 0; i <= 3; i++) {
        vec2 dg = tri2(bp * 2.0) * 0.5;
        dg *= rot(noiseTime);
        p += dg / z2;
        bp *= 1.5;
        z2 *= 0.6;
        z *= 1.7;
        p *= 1.2;
        p *= mat2(0.970, 0.242, -0.242, 0.970);
        rz += tri(p.x + tri(p.y)) / z;
    }
    return rz;
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

    float nz = clamp(triangleNoise(p), 0.0, 1.0);

    // Scale p by noise — organic spatial warping
    p *= 1.0 + (nz - 0.5) * noiseStrength;

    vec3 result = vec3(0.0);
    int totalFilaments = numOctaves * 12;

    vec2 p1 = vec2(-radius, 0.0);

    for (int i = 0; i < totalFilaments; i++) {
        p1 *= rot(stepAngle + pow(rotationAccum, 1.5) * 0.0007);

        vec2 p2 = p1 * rot(spread * float(i) - rotationAccum);

        // FFT semitone lookup
        float freq = baseFreq * pow(2.0, float(i) / 12.0);
        float bin = freq / (sampleRate * 0.5);
        float mag = 0.0;
        if (bin <= 1.0) {
            mag = texture(fftTexture, vec2(bin, 0.5)).r;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
        }

        float dist = segm(p, p1, p2);

        float glow = glowIntensity / (pow(dist, 1.0 / falloffExponent) + EPSILON);

        float pitchClass = fract(float(i) / 12.0);
        vec3 color = texture(gradientLUT, vec2(pitchClass, 0.5)).rgb;

        result += color * glow * (baseBright + mag);
    }

    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
