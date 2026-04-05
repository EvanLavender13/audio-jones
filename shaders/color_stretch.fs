// Based on "Color Stretch" by KilledByAPixel (Frank Force)
// https://www.shadertoy.com/view/4lXcD7
// License: CC BY-NC-SA 3.0 Unported
// Modified: uniforms for grid/recursion/focus, gradient LUT replaces HSV, FFT audio, spin

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
uniform vec2 focusOffset;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float E = 2.718281828459;

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

    // Focus position: center cell + clamped offset (research: clamp to [0, glyphSize-1])
    ivec2 focus = clamp(ivec2(glyphSize / 2) + ivec2(round(focusOffset)),
                        ivec2(0), ivec2(glyphSize - 1));
    vec2 focusF = vec2(focus);

    // Offset convergence loop (kept verbatim, 13 iterations sufficient for glyphSize up to 8)
    vec2 offset = vec2(0.0);
    float gsfi = 1.0 / glyphSizeF;
    for (int i = 0; i < 13; ++i) {
        offset += focusF * gsfi * pow(gsfi, float(i));
    }

    // Apply zoom & offset (kept verbatim)
    vec2 pos = uv * zoom + offset;

    // Fractal recursion loop (core loop structure from GetPixelFractal)
    vec3 color = vec3(0.0);

    for (int r = 0; r <= recursionCount + 1; ++r) {
        // Shared index t for gradient LUT and FFT (research: t = r / recursionCount)
        float t = float(r) / float(recursionCount);

        // FFT frequency lookup (standard generator pattern)
        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Gradient LUT color (replaces HSV accumulation)
        vec3 layerColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // GetRecursionFade logic (kept verbatim)
        float fade;
        if (r > recursionCount) {
            fade = timePercent;
        } else {
            float rt = max(float(r) - timePercent, 0.0);
            fade = rt / float(recursionCount);
        }

        color += layerColor * brightness * fade;

        if (r > recursionCount) { break; }

        // Subdivide space (core loop: multiply pos by glyphSize, subtract floor)
        pos *= glyphSizeF;
        pos -= floor(pos);
    }

    finalColor = vec4(color, 1.0);
}
