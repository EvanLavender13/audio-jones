// Based on "Color Stretch" by KilledByAPixel (Frank Force)
// https://www.shadertoy.com/view/4lXcD7
// License: CC BY-NC-SA 3.0 Unported
// Modified: uniforms for grid/recursion, gradient LUT replaces HSV, FFT audio, spin

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform float zoomPhase;
uniform float zoomSpeed;
uniform float zoomScale;
uniform int glyphSize;
uniform int recursionCount;
uniform float curvature;
uniform float spinPhase;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float E = 2.718281828459;

// Hash from reference (kept verbatim)
float RandFloat(int i) { return fract(sin(float(i)) * 43758.5453); }

void main() {
    // Center UV, normalize by height (from reference mainImage)
    vec2 uv = fragTexCoord * resolution - resolution * 0.5;
    uv /= resolution.y;

    // Spin: 2x2 rotation by CPU-accumulated spinPhase (replaces no-op InitUV)
    float c = cos(spinPhase);
    float s = sin(spinPhase);
    uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

    // Per-pixel time with curvature warp (reference: time = iTime + curvature*pow(length(uv), 0.2))
    // zoomPhase accumulated on CPU; curvature offset scaled by zoomSpeed for rate consistency
    float timePercent = zoomPhase + curvature * pow(length(uv), 0.2) * zoomSpeed;
    int iterations = int(floor(timePercent));
    timePercent -= float(iterations);

    // Zoom math (kept verbatim: pow(e, -glyphSizeLog * timePercent) * zoomScale)
    float glyphSizeF = float(glyphSize);
    float glyphSizeLog = log(glyphSizeF);
    float zoom = pow(E, -glyphSizeLog * timePercent) * zoomScale;

    vec2 focusF = vec2(ivec2(glyphSize / 2));

    // Offset convergence loop (kept verbatim, 13 iterations sufficient for glyphSize up to 8)
    vec2 offset = vec2(0.0);
    float gsfi = 1.0 / glyphSizeF;
    for (int i = 0; i < 13; ++i) {
        offset += focusF * gsfi * pow(gsfi, float(i));
    }

    // Apply zoom & offset (kept verbatim)
    vec2 pos = uv * zoom + offset;

    // Pass 1: accumulate gradient position from cell hashes (replaces HSV accumulation)
    ivec2 glyphPosLast = ivec2(glyphSize / 2);
    ivec2 glyphPos = ivec2(glyphSize / 2);
    float gradPos = 0.5;

    for (int r = 0; r <= recursionCount + 1; ++r) {
        int seed = (iterations + r) + (glyphPosLast.y + glyphPos.y);
        float gradOffset = mix(-0.2, 0.2, RandFloat(seed));

        float fade;
        if (r > recursionCount) {
            fade = timePercent;
        } else {
            float rt = max(float(r) - timePercent, 0.0);
            fade = rt / float(recursionCount);
        }

        gradPos += gradOffset * fade;

        if (r > recursionCount) { break; }

        pos *= glyphSizeF;
        glyphPosLast = glyphPos;
        glyphPos = ivec2(pos);
        pos -= floor(pos);
    }

    // Final gradient position determines both color and FFT frequency
    float t = fract(gradPos);
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float halfBand = 0.5 / float(recursionCount);
    float tLo = max(t - halfBand, 0.0);
    float tHi = min(t + halfBand, 1.0);
    float freqLo = baseFreq * pow(maxFreq / baseFreq, tLo);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, tHi);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    finalColor = vec4(color * brightness, 1.0);
}
