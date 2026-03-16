// Based on "Arvore 3b - sinapses" by Elsio
// https://www.shadertoy.com/view/XfXcR7
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized all constants; replaced raw color accumulation with
// gradient LUT; added FFT audio reactivity per fold depth; added trail buffer
// with decay/blur; added forward camera drift.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float animPhase;        // CPU-accumulated: phase += animSpeed * deltaTime
uniform int marchSteps;
uniform int foldIterations;
uniform float fov;
uniform float foldOffset;
uniform float yFold;
uniform float branchThickness;
uniform float camZ;             // CPU-accumulated: camZ += driftSpeed * deltaTime, fmod 100
uniform float synapseIntensity;
uniform float synapseBounceFreq;
uniform float synapsePulseFreq;
uniform float colorPhase;
uniform float colorStretch;
uniform float brightness;
uniform sampler2D gradientLUT;
uniform sampler2D previousFrame;
uniform float decayFactor;
uniform float trailBlur;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

mat2 rot(float a) {
    vec4 v = cos(a + vec4(0.0, 11.0, 33.0, 0.0));
    return mat2(v.x, v.y, v.z, v.w);
}

void main() {
    // FFT precomputation — energy per fold depth
    const int MAX_FOLDS = 12;
    float fftBands[MAX_FOLDS];
    for (int k = 0; k < MAX_FOLDS; k++) {
        if (k >= foldIterations) { fftBands[k] = 0.0; continue; }
        float tFreq = float(k) / float(max(foldIterations - 1, 1));
        float freqLo = baseFreq * pow(maxFreq / baseFreq, tFreq);
        float tNext = float(k + 1) / float(foldIterations);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, tNext);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);
        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int bs = 0; bs < BAND_SAMPLES; bs++) {
            float bin = mix(binLo, binHi, (float(bs) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        fftBands[k] = baseBright + energy;
    }

    // Screen coord setup
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 u = (fragCoord - resolution * 0.5) / resolution.y;

    float i, d, s, a, c, cyl;
    float j;
    vec3 color = vec3(0.0);

    for (i = 0.0; i < float(marchSteps); i += 1.0) {
        // Ray setup — reference: vec3 p = vec3(d * u * 1.8, d * 1.8) + vec3(-.1, .7, -.2)
        vec3 p = vec3(d * u * fov, d * fov) + vec3(-0.1, 0.7, camZ);

        // Init fold state — reference: j = s = a = 8.45
        j = 8.45;
        s = 8.45;
        a = 8.45;

        // Inner fold loop — reference: while(j++ < 16.)
        int minFoldIdx = 0;  // Track which fold depth produced the minimum distance
        float jEnd = 8.45 + float(foldIterations);
        int foldIdx = 0;
        while (j < jEnd) {
            j += 1.0;

            // Sphere inversion — reference: c = dot(p,p), p /= c + .005, a /= c
            c = dot(p, p);
            p /= c + 0.005;
            a /= c;

            // Abs-fold with rotation — reference: p.xz = abs(rot(sin(t - 1./c) / a - j) * p.xz) - .5
            p.xz = abs(rot(sin(animPhase - 1.0 / c) / a - j) * p.xz) - foldOffset;

            // Y-fold — reference: p.y = 1.78 - p.y
            p.y = yFold - p.y;

            // Cylinder distance — reference: cyl = length(p.xz) * 2.5 - .06 / c
            cyl = length(p.xz) * 2.5 - branchThickness / c;
            cyl = max(cyl, p.y) / a;

            // Track which fold depth gave the closest approach
            float prevS = s;
            s = min(s, cyl) * 0.9;
            if (s < prevS) minFoldIdx = foldIdx;

            foldIdx++;
        }

        // Advance ray
        d += abs(s) + 1e-6;

        // Glow accumulation — reference: s < .001 ? o += .000001 / d*i : o
        // NOTE: reference formula is (0.000001 / d) * i — multiply by i, not divide
        if (s < 0.001) {
            float lutCoord = fract(d * colorStretch + colorPhase);
            vec3 stepColor = texture(gradientLUT, vec2(lutCoord, 0.5)).rgb;
            // Per-fold FFT brightness — use the fold depth that produced the hit
            float foldBright = fftBands[minFoldIdx];
            color += stepColor * foldBright * 0.000001 / d * i;
        }

        // Synapse glow — reference: bouncing sphere intersection
        if (cyl > length(p - vec3(0.0, sin(synapseBounceFreq * animPhase), 0.0))
                  - 0.5 - 0.5 * cos(synapsePulseFreq * animPhase)) {
            float lutCoord = fract(d * colorStretch + colorPhase + 0.5);
            vec3 synapseColor = texture(gradientLUT, vec2(lutCoord, 0.5)).rgb;
            if (s < 0.001) {
                // Reference: o += .000005 / d * i — multiply by i
                color += synapseColor * synapseIntensity * 0.000005 / d * i;
            }
            if (s < 0.02) {
                color += synapseColor * synapseIntensity * 0.0002 / d;
            }
        }
    }

    // Tanh tonemapping
    color = tanh(color * brightness / 7000.0);

    // Trail buffer with controllable blur — verbatim from Muons
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec3 raw = texelFetch(previousFrame, coord, 0).rgb;
    vec3 blurred  = 0.25   * raw;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2(-1, 0), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 1, 0), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 0,-1), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 0, 1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2(-1,-1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2( 1,-1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2(-1, 1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2( 1, 1), 0).rgb;
    vec3 prev = mix(raw, blurred, trailBlur);
    if (any(isnan(prev))) prev = vec3(0.0);
    finalColor = vec4(max(color, prev * decayFactor), 1.0);
}
