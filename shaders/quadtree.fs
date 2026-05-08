// Based on "Quadtree Random Points" by Bingle
// https://www.shadertoy.com/view/wc2XRh
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced parametric/mouse points with CPU-driven Lissajous source
// array, replaced grayscale outline with gradient LUT + FFT band lookup,
// exposed maxIterations, lineWidth, cellFillAmount, colorMode as uniforms
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec2 sources[8];
uniform int pointCount;
uniform int maxIterations;
uniform float lineWidth;
uniform float cellFillAmount;
uniform int colorMode; // 0=depth, 1=hash
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

bool inBox(vec2 lo, vec2 hi, vec2 p) {
    return lo.x <= p.x && lo.y <= p.y && hi.x > p.x && hi.y > p.y;
}

void main() {
    // toUv: center fragCoord, divide by resolution.y so cells are square
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 uv = 2.0 * (fragCoord - 0.5 * resolution) / resolution.y;

    vec2 lo = floor(uv);
    vec2 hi = ceil(uv);
    uv = mod(uv, vec2(1.0));

    int iters = 0;
    while (iters < maxIterations) {
        iters++;
        bool sub = false;
        for (int i = 0; i < pointCount; ++i) {
            if (inBox(lo, hi, sources[i])) {
                sub = true;
                break;
            }
        }
        if (sub) {
            vec2 center = (hi + lo) * 0.5;
            if (uv.x > 0.5) { lo.x = center.x; uv.x = uv.x * 2.0 - 1.0; }
            else            { hi.x = center.x; uv.x *= 2.0; }
            if (uv.y > 0.5) { lo.y = center.y; uv.y = uv.y * 2.0 - 1.0; }
            else            { hi.y = center.y; uv.y *= 2.0; }
        } else {
            break;
        }
    }

    // Outline: smoothstep on distance to cell edge in cell-local UV.
    // The pow(2.0, iters)/resolution.y factor converts pixels to cell-local
    // UV at the current depth so lineWidth stays a screen-pixel quantity.
    float edge = max(abs(uv.x - 0.5), abs(uv.y - 0.5)) * 2.0;
    float outlineThreshold = 1.0 - lineWidth * pow(2.0, float(iters)) / resolution.y;
    float outline = smoothstep(outlineThreshold, 1.0, edge);

    float coverage = outline + cellFillAmount * (1.0 - outline);

    float t;
    if (colorMode == 0) {
        t = float(iters) / float(maxIterations);
    } else {
        t = fract(sin(dot(lo, vec2(12.9898, 78.233))) * 43758.5453);
    }

    vec3 baseColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; ++s) {
        float ts = t + (float(s) + 0.5) / float(BAND_SAMPLES) / float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 col = baseColor * brightness * coverage;
    finalColor = vec4(col, 1.0);
}
