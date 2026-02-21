// Hex Rush: Polar-coordinate tunnel with hash-based procedural wall patterns, N-gon corrected
// radius, FFT-driven wall brightness, and gradient coloring. Walls rush toward center with
// guaranteed gaps per ring.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

uniform int sides;
uniform float centerSize;
uniform float wallThickness;
uniform float wallSpacing;
uniform float rotationAccum;
uniform float pulseAmount;
uniform float pulseAccum;
uniform sampler2D ringBuffer;
uniform int freqBins;
uniform float perspective;
uniform float bgContrast;
uniform float colorAccum;
uniform float wallGlow;
uniform float glowIntensity;
uniform float wallAccum;
uniform float wobbleTime;

const float PI = 3.14159265;
const float TAU = 6.28318530;

void main() {
    vec2 fc = fragTexCoord * resolution;

    // 1. Center UV relative to screen center with aspect correction
    vec2 uv = (fc - 0.5 * resolution) / resolution.y;

    // 2. Perspective distortion (uses wobbleTime, decoupled from wallSpeed)
    uv /= dot(uv, vec2(sin(wobbleTime * 0.34), sin(wobbleTime * 0.53))) * perspective + 1.0;

    // 3. Polar conversion with rotationAccum
    float r = length(uv);
    float theta = atan(uv.y, uv.x) + rotationAccum;

    // 4. Section index and odd/even
    float zone = floor(float(sides) * mod(theta + PI, TAU) / TAU);
    float odd = mod(zone, 2.0);

    // 5. N-gon corrected radius
    float slice = TAU / float(sides);
    float hexR = cos(floor(0.5 + theta / slice) * slice - theta) * r;

    // 6. Alternating background segments
    float colorT = fract(colorAccum);
    vec3 baseBg = texture(gradientLUT, vec2(colorT, 0.5)).rgb;
    vec3 bgColor = baseBg * (0.08 + bgContrast * 0.15 * odd);

    // 7. Wall depth and procedural generation
    float depth = hexR * 10.0 + wallAccum;
    float ringIndex = floor(depth / wallSpacing);
    float ringFrac = fract(depth / wallSpacing);
    float segIndex = zone;

    // Per-ring gapChance and patternSeed from ring history buffer
    vec2 ringData = texelFetch(ringBuffer, ivec2(int(ringIndex) & 255, 0), 0).rg;
    float ringGap = ringData.r;
    float ringSeed = ringData.g;

    float wallHash = fract(sin(ringIndex * 127.1 + segIndex * 311.7 + ringSeed) * 43758.5453);
    float hasWall = step(ringGap, wallHash);

    float gapSeg = floor(fract(sin(ringIndex * 269.3 + ringSeed) * 43758.5453) * float(sides));
    if (segIndex == gapSeg) hasWall = 0.0;

    // 8. Wall rendering with thickness and glow
    float wallDist = abs(ringFrac - 0.5) * 2.0 * wallSpacing;
    float wallCore = 1.0 - smoothstep(wallThickness * 0.5 - 0.002, wallThickness * 0.5, wallDist);
    float wallSoft = 1.0 - smoothstep(wallThickness * 0.5, wallThickness * 0.5 + wallGlow * 0.5, wallDist);
    float wallMask = max(wallCore, wallSoft);
    wallMask *= hasWall;

    // Walls stop at center polygon edge
    float pulseR = centerSize + sin(pulseAccum) * pulseAmount;
    wallMask *= smoothstep(pulseR, pulseR + 0.02, hexR);

    // 9. FFT brightness — radial mapping (low freq center, high freq edge)
    float freqRatio = maxFreq / baseFreq;
    float radialT = clamp(hexR / 0.9, 0.0, 1.0);
    float freqLo = baseFreq * pow(freqRatio, radialT);
    float freqHi = baseFreq * pow(freqRatio, min(radialT + 0.05, 1.0));
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
    float brightness = baseBright + pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

    // 10. Center polygon outline with pulse (pulseR computed above for wall clipping)
    float centerOutline = smoothstep(pulseR + 0.015, pulseR + 0.005, hexR)
                        * (1.0 - smoothstep(pulseR - 0.005, pulseR - 0.015, hexR));
    float centerFill = smoothstep(pulseR + 0.005, pulseR - 0.005, hexR);

    // 11. Gradient LUT coloring and compositing
    vec3 wallColor = texture(gradientLUT, vec2(colorT, 0.5)).rgb;

    vec3 centerBg = baseBg * 0.08;
    vec3 color = mix(bgColor, centerBg, centerFill);
    color = mix(color, wallColor * brightness * glowIntensity, wallMask);
    color = mix(color, wallColor, centerOutline);

    // Vignette
    color *= smoothstep(1.8, 0.5, length(uv));

    finalColor = vec4(color, 1.0);
}
