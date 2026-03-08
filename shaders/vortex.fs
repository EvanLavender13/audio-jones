// Based on "Vortex [265]" by Xor
// https://www.shadertoy.com/view/wctXWN
// License: CC BY-NC-SA 3.0
// Modified: ported to GLSL 330; parameterized march steps, turbulence,
// sphere radius, surface detail, camera distance; replaced cos() coloring
// with gradient LUT; added FFT audio reactivity; added trail buffer with
// decay/blur; added brightness tonemap control.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

// Same uniform set as Muons minus mode/turbulenceMode/phase/drift/axisFeedback/colorMode/ringThickness
uniform vec2 resolution;
uniform float time;
uniform int marchSteps;           // original: 1e2
uniform int turbulenceOctaves;    // original: d<9 loop bound
uniform float turbulenceGrowth;   // original: 0.8
uniform float sphereRadius;       // original: 0.5
uniform float surfaceDetail;      // original: 4e1
uniform float cameraDistance;     // original: 6.
uniform float rotationAngle;     // CPU-accumulated Y-axis spin
uniform float colorPhase;        // CPU-accumulated color scroll
uniform float colorStretch;
uniform float brightness;         // folds original /7e3 divisor
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

void main() {
    // Screen coords — verbatim from reference: (I+I - res.xyy) normalized
    vec2 fragCoord = fragTexCoord * resolution;
    vec3 rayDir = normalize(vec3(fragCoord * 2.0, 0.0) - vec3(resolution.x, resolution.y, resolution.y));

    // Dithered ray start — verbatim from reference for banding suppression
    // Original: z = fract(dot(I,sin(I)))
    float z = fract(dot(fragCoord, sin(fragCoord)));
    float d;
    vec3 color = vec3(0.0);
    float stepCount = float(max(marchSteps - 1, 1));

    for (int i = 0; i < marchSteps; i++) {
        // Raymarch sample position — verbatim from reference
        // Original: vec3 p = z * normalize(vec3(I+I,0) - iResolution.xyy);
        vec3 p = z * rayDir;
        // Original: p.z += 6.  ->  p.z += cameraDistance
        p.z += cameraDistance;

        // Y-axis rotation — reveals different faces of the turbulence volume
        float cs = cos(rotationAngle);
        float sn = sin(rotationAngle);
        p.xz = mat2(cs, -sn, sn, cs) * p.xz;

        // Turbulence loop — geometric frequency scaling
        // Original: for(d=1.; d<9.; d/=.8) p += cos(p.yzx*d-iTime)/d;
        // Parameterized: loop turbulenceOctaves times, d /= turbulenceGrowth
        d = 1.0;
        for (int j = 0; j < turbulenceOctaves; j++) {
            p += cos(p.yzx * d - time) / d;
            d /= turbulenceGrowth;
        }

        // Hollow sphere distance in warped space
        // Original: z += d = .002+abs(length(p)-.5)/4e1
        // Parameterized: sphereRadius replaces 0.5, surfaceDetail replaces 4e1
        d = 0.002 + abs(length(p) - sphereRadius) / surfaceDetail;
        z += d;

        // Color: gradient LUT replaces sin(z+vec4(6,2,4,0))+1.5
        float lutCoord = fract(z * colorStretch + colorPhase);
        vec3 stepColor = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

        // Per-step FFT — map march step to frequency band in log space
        // Same pattern as Muons additive volume mode
        float t0 = float(i) / stepCount;
        float t1 = float(i + 1) / stepCount;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);
        const int BAND_SAMPLES = 4;
        float energy = 0.0;
        for (int bs = 0; bs < BAND_SAMPLES; bs++) {
            float bin = mix(binLo, binHi, (float(bs) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float audio = baseBright + energy;

        // Additive accumulation — glow inversely proportional to step distance
        // Original: O += (coloring) / d
        color += stepColor * audio / d;
    }

    // Tanh tonemapping — original: O = tanh(O/7e3), brightness folds into divisor
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
