// Based on "StarTrail" by hdrp0720
// https://www.shadertoy.com/view/wXcyz7
// License: CC BY-NC-SA 3.0 Unported
// Modified: collapsed 3-buffer to single-pass ping-pong, analytical orbits,
// per-star FFT reactivity, gradient LUT color

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float variationTime;
uniform sampler2D previousFrame;
uniform int starCount;
uniform int freqMapMode;
uniform float spreadRadius;
uniform float speedWaviness;
uniform float glowIntensity;
uniform float decayFactor;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float SPRITE_RADIUS = 0.0075;

float hash11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float smootherstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return x * x * x * (3.0 * x * (2.0 * x - 5.0) + 10.0);
}

#define TWO_PI 6.2831853

void main() {
    vec3 color = texture(previousFrame, fragTexCoord).rgb * decayFactor;

    vec2 posScreen = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;
    float pixelRadius = length(posScreen);

    vec3 accumColor = vec3(0.0);

    for (int i = 0; i < starCount; i++) {
        float starRadius = sqrt(hash11(float(i) * 12.345)) * spreadRadius;

        if (abs(pixelRadius - starRadius) > SPRITE_RADIUS) { continue; }

        float initialAngle = hash11(float(i) * 67.890) * TWO_PI;
        float variation = sin(starRadius * speedWaviness);
        float angle = initialAngle + time + variation * variationTime;
        vec2 starPos = starRadius * vec2(cos(angle), sin(angle));

        float dist = length(posScreen - starPos);
        if (dist >= SPRITE_RADIUS) { continue; }

        float t;
        if (freqMapMode == 0) {
            t = clamp(starRadius / spreadRadius, 0.0, 1.0);
        } else if (freqMapMode == 1) {
            t = hash11(float(i) * 45.678);
        } else {
            t = fract(initialAngle / TWO_PI);
        }

        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        vec3 starColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
        float intensity = smootherstep(SPRITE_RADIUS, 0.0, dist);

        accumColor += starColor * brightness * intensity * glowIntensity;
    }

    color += accumColor;
    finalColor = vec4(color, 1.0);
}
