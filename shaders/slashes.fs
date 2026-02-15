// Slashes: Chaotic rectangular bars per semitone, re-rolled each tick via PCG3D hash.
// FFT magnitude gates visibility, sharp envelope gives staccato flash.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform int bars;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float tickAccum;
uniform float envelopeSharp;
uniform float maxBarLength;
uniform float barThickness;
uniform float thicknessVariation;
uniform float scatter;
uniform float glowSoftness;
uniform float baseBright;
uniform float rotationDepth;

const float TAU = 6.2831853;

// PCG3D hash (Jarzynski & Olano) — deterministic pseudo-random from 3D uint coords
uvec3 pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

// Normalize pcg3d output to [0,1] per channel
vec3 hash3(uvec3 seed) {
    uvec3 h = pcg3d(seed);
    return vec3(h) / float(0xFFFFFFFFu);
}

// 2D box signed distance (IQ)
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// Rodrigues rotation — rotate p around axis ax by angle t
vec3 erot(vec3 p, vec3 ax, float t) {
    return mix(dot(ax, p) * ax, p, cos(t)) + cross(ax, p) * sin(t);
}

// 2D rotation matrix
mat2 rot2(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, s, -s, c);
}

void main() {
    vec2 r = resolution;
    vec2 uv = (fragTexCoord * r * 2.0 - r) / min(r.x, r.y);

    float tickFloor = floor(tickAccum);
    uint ti = uint(tickFloor);
    float tickFract = fract(tickAccum);

    // Envelope: peaks at tick boundary (fract=0), decays within tick
    float env = exp(-pow(tickFract, envelopeSharp));

    vec3 result = vec3(0.0);
    int totalBars = bars;

    for (int i = 0; i < totalBars; i++) {
        // PCG3D hash seeded with (semitoneIndex, tickIndex, tickIndex)
        vec3 rnd = hash3(uvec3(uint(i), ti, ti));

        // FFT band-averaging lookup
        float t0 = float(i) / float(totalBars);
        float t1 = float(i + 1) / float(totalBars);
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

        // Bar dimensions driven by envelope and FFT magnitude
        float halfWidth = maxBarLength * env * mag;
        float halfThick = barThickness + rnd.x * thicknessVariation * barThickness;

        // Skip negligible bars
        if (halfWidth < 0.0001) continue;

        // Random position offset from center
        vec2 offset = (rnd.yz * 2.0 - 1.0) * scatter;

        // Random rotation angle
        float angle = rnd.x * TAU;

        // Transform UV: translate then rotate into bar's local space
        vec2 localUV = uv - offset;

        // Apply rotation: 2D or 3D foreshortened via erot
        if (rotationDepth > 0.0) {
            // 3D Rodrigues rotation with random axis for depth foreshortening
            vec3 rndAxis = hash3(uvec3(uint(i) + 97u, ti, ti + 31u));
            vec3 axis = normalize(rndAxis * 2.0 - 1.0);

            // Pure 2D rotation result
            vec2 uv2d = rot2(angle) * localUV;

            // 3D rotation: embed in XY plane, rotate around random axis, project back
            vec3 p3 = vec3(localUV, 0.0);
            vec3 rotated3d = erot(p3, axis, angle);
            vec2 uv3d = rotated3d.xy;

            // Blend between 2D and 3D based on rotationDepth
            localUV = mix(uv2d, uv3d, rotationDepth);
        } else {
            localUV = rot2(angle) * localUV;
        }

        // SDF distance to oriented rectangle
        float d = sdBox(localUV, vec2(halfWidth, halfThick));

        // Soft-edged glow mask
        float glow = smoothstep(glowSoftness, 0.0, d);

        // Color from gradient LUT by normalized bar index
        vec3 color = texture(gradientLUT, vec2(float(i) / float(totalBars), 0.5)).rgb;

        // Additive blend
        result += color * glow * (baseBright + mag);
    }

    // tanh tone mapping prevents blowout from additive accumulation
    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
