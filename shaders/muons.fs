// Raymarched volumetric particle trails — thin luminous filaments spiraling
// through 3D space like charged particles in a bubble chamber.
//
// Based on "Muons [315]" by Xor
// https://www.shadertoy.com/view/W3G3zh
// License: CC BY-NC-SA 3.0
//
// Modified: ported to GLSL 330; parameterized march steps, turbulence octaves,
// ring thickness, camera distance; replaced cos() coloring with gradient LUT;
// added 6 alternative shell distance modes; added FFT audio reactivity;
// added trail buffer with decay/blur; added brightness tonemap control.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform int marchSteps;
uniform int turbulenceOctaves;
uniform float turbulenceStrength;
uniform float ringThickness;
uniform float cameraDistance;
uniform float colorSpeed;
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
uniform int mode;
uniform int turbulenceMode;
uniform vec3 phase;
uniform float drift;

const float PHI = 1.6180339887;

void main() {
    // Screen coords centered at origin, matching original's (I+I - res.xyy) pattern
    vec2 fragCoord = fragTexCoord * resolution;
    vec3 rayDir = normalize(vec3(fragCoord * 2.0, 0.0) - vec3(resolution.x, resolution.y, resolution.y));

    float z = 0.0;
    float s = 0.0;
    float d = 0.0;
    vec3 color = vec3(0.0);
    float closestHit = 1e6;
    int winnerStep = 0;
    float winnerGlow = 0.0;

    for (int i = 0; i < marchSteps; i++) {
        // Sample point along ray, camera offset in z only
        vec3 p = z * rayDir;
        p.z += cameraDistance;

        // Time-varying rotation axis — s from previous step breaks periodicity
        vec3 a = normalize(cos(phase + time * (1.0 + drift * vec3(1.0, PHI, PHI * PHI)) - s));

        // Rodrigues rotation of sample point around axis
        a = a * dot(a, p) - cross(a, p);

        // FBM turbulence: layered sine displacement with .yzx swizzle
        // Original: for(d=1.;d++<9.;) a+=sin(a*d+t).yzx/d
        d = 1.0;
        for (int j = 1; j < turbulenceOctaves; j++) {
            d += 1.0;
            switch (turbulenceMode) {
            case 1: // Fract Fold — sharp sawtooth, crystalline/faceted filaments
                a += (fract(a * d + time * PHI) * 2.0 - 1.0).yzx / d * turbulenceStrength;
                break;
            case 2: // Abs-Sin Fold — sharp angular turns, always-positive displacement
                a += abs(sin(a * d + time * PHI)).yzx / d * turbulenceStrength;
                break;
            case 3: // Triangle Wave — smooth zigzag between sin and fract character
                a += (abs(fract(a * d + time * PHI) * 2.0 - 1.0) * 2.0 - 1.0).yzx / d * turbulenceStrength;
                break;
            case 4: // Squared Sin — soft peaks with flat valleys, organic feel
                a += (sin(a * d + time * PHI) * abs(sin(a * d + time * PHI))).yzx / d * turbulenceStrength;
                break;
            case 5: // Square Wave — hard binary on/off, blocky digital geometry
                a += (step(0.5, fract(a * d + time * PHI)) * 2.0 - 1.0).yzx / d * turbulenceStrength;
                break;
            case 6: // Quantized — grid-locked staircase structures
                a += (floor(a * d + time * PHI + 0.5)).yzx / d * turbulenceStrength;
                break;
            default: // 0: Sine (original) — smooth swirling folds
                a += sin(a * d + time * PHI).yzx / d * turbulenceStrength;
                break;
            }
        }

        // Shell distance in warped space
        s = length(a);
        switch (mode) {
        case 1:  // L1 Norm
            d = ringThickness * (abs(a.x - a.y) + abs(a.y - a.z) + abs(a.z - a.x));
            break;
        case 2:  // Axis Distance
            d = ringThickness * min(length(a.xy), min(length(a.yz), length(a.xz)));
            break;
        case 3:  // Dot Product
            d = ringThickness * abs(a.x*a.y + a.y*a.z);
            break;
        case 4:  // Chebyshev Spread
            {
                vec3 aa = abs(a);
                d = ringThickness * abs(max(aa.x, max(aa.y, aa.z)) - min(aa.x, min(aa.y, aa.z)));
            }
            break;
        case 5:  // Cone Metric
            d = ringThickness * abs(length(a.xy) - abs(a.z));
            break;
        case 6:  // Triple Product
            d = ringThickness * abs(a.x * a.y * a.z);
            break;
        default: // 0: Sine Shells (original)
            d = ringThickness * abs(sin(s));
            break;
        }

        // Adaptive step — smaller near shells for sharp crossings
        z += d;

        // Track closest shell crossing
        float proximity = d * s;
        if (proximity < closestHit) {
            closestHit = proximity;
            winnerStep = i;
            winnerGlow = 1.0 / max(proximity, 1e-6);
        }
    }

    float stepCount = float(max(marchSteps - 1, 1));

    // Color from winner's step position in LUT
    float lutCoord = fract(float(winnerStep) / stepCount + time * colorSpeed);
    vec3 sampleColor = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

    // FFT from winner's frequency band (multi-sample, same as motherboard)
    float t0 = float(winnerStep) / stepCount;
    float t1 = float(winnerStep + 1) / stepCount;
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

    // Final pixel — glow * color * audio
    color = vec3(winnerGlow * audio) * sampleColor;

    // Soft HDR rolloff
    color = tanh(min(color * brightness / 3000.0, 20.0));

    // Trail buffer with controllable blur to suppress single-pixel speckle
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
