// Signal Frames: Concentric SDF outlines (boxes + triangles) mapped to FFT semitones with
// orbital motion, Lorentzian glow, and sweep boost. Each layer = one semitone.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform int numOctaves;
uniform float baseFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float rotationAccum;
uniform float orbitRadius;
uniform float orbitAccum;
uniform float sizeMin;
uniform float sizeMax;
uniform float aspectRatio;
uniform float outlineThickness;
uniform float glowWidth;
uniform float glowIntensity;
uniform float sweepAccum;
uniform float sweepIntensity;
uniform sampler2D gradientLUT;

const float TAU = 6.28318530718;

float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdEquilateralTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r / k;
    if (p.x + k * p.y > 0.0) p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    vec2 uv0 = (fc - 0.5 * resolution) / min(resolution.x, resolution.y);

    vec3 total = vec3(0.0);

    int totalLayers = numOctaves * 12;

    for (int i = 0; i < totalLayers; i++) {
        float t = float(i) / float(totalLayers);

        // Per-layer UV: rotate then apply orbital offset
        float angle = t * rotationAccum + sin(orbitAccum);
        float ca = cos(angle), sa = sin(angle);
        vec2 uv = vec2(uv0.x * ca - uv0.y * sa, uv0.x * sa + uv0.y * ca);

        // Orbital offset driven by independent time uniform
        float orbitAngle = orbitAccum + t * TAU;
        uv += vec2(sin(orbitAngle), cos(orbitAngle)) * orbitRadius * t;

        // Per-layer size
        float size = mix(sizeMin, sizeMax, t);

        // SDF: even layers = box, odd layers = equilateral triangle
        float sdf;
        if (i % 2 == 0) {
            sdf = sdBox(uv, vec2(size * aspectRatio, size));
        } else {
            sdf = sdEquilateralTriangle(uv, size);
        }

        // Outline extraction
        float d = abs(sdf) - outlineThickness;

        // Lorentzian glow
        float gw2 = glowWidth * glowWidth;
        float glow = gw2 / (gw2 + d * d) * glowIntensity;

        // Sweep boost
        float sweepPhase = fract(sweepAccum + t);
        float sweepBoost = sweepIntensity / (sweepPhase + 0.0001);

        // FFT semitone lookup
        int semitone = i;
        float freq = baseFreq * pow(2.0, float(semitone) / 12.0);
        float bin = freq / (sampleRate * 0.5);
        float mag = 0.0;
        if (bin <= 1.0) {
            mag = texture(fftTexture, vec2(bin, 0.5)).r;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
        }

        // Color from gradient LUT by pitch class
        int pitchClass = semitone % 12;
        vec3 color = texture(gradientLUT, vec2(float(pitchClass) / 12.0, 0.5)).rgb;

        // Composite: glow * brightness * sweep
        total += color * glow * (baseBright + mag) * (1.0 + sweepBoost);
    }

    // Additive tonemap to prevent clipping
    total = 1.0 - exp(-total);

    finalColor = vec4(total, 1.0);
}
