// shaders/moire_generator.fs
#version 330

// Moire Generator: Procedural multi-layer grating interference with stripe, circle, and grid modes

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D gradientLUT;  // 1D gradient for color mapping

// Global parameters
uniform int patternMode;        // 0=stripes, 1=circles, 2=grid
uniform int layerCount;         // Active layers (2-4)
uniform int sharpMode;          // 0=smooth sinusoidal, 1=sharp step
uniform float colorIntensity;   // Blend between grayscale and LUT color (0-1)
uniform float globalBrightness; // Output intensity multiplier
uniform float time;             // Warp animation clock

// Per-layer parameters accessed via "layer0.frequency", "layer1.angle", etc.
struct MoireLayer {
    float frequency;
    float angle;
    float warpAmount;
    float scale;
    float phase;
};

uniform MoireLayer layer0;
uniform MoireLayer layer1;
uniform MoireLayer layer2;
uniform MoireLayer layer3;

#define PI 3.14159265359

mat2 rotate2d(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

// Smooth sinusoidal grating: produces soft analog moire
float gratingSmooth(float coord, float freq, float phase) {
    return 0.5 + 0.5 * cos(2.0 * PI * coord * freq + phase);
}

// Sharp step grating: square-wave with richer harmonics
float gratingSharp(float coord, float freq, float phase) {
    return step(0.5, fract(coord * freq + phase / (2.0 * PI)));
}

// Route to smooth or sharp grating based on mode toggle
float grating(float coord, float freq, float phase) {
    if (sharpMode != 0) return gratingSharp(coord, freq, phase);
    return gratingSmooth(coord, freq, phase);
}

// Evaluate grating based on pattern mode
float evaluateGrating(vec2 uv, float freq, float phase) {
    if (patternMode == 1) return grating(length(uv - 0.5), freq, phase);
    if (patternMode == 2) return grating(uv.x, freq, phase) * grating(uv.y, freq, phase);
    return grating(uv.x, freq, phase);
}

// Apply per-layer UV transform: scale, rotate, warp (order matters)
vec2 transformUV(vec2 uv, float scale, float angle, float warpAmount, float phase) {
    // 1. Scale from center
    uv = (uv - 0.5) * scale + 0.5;

    // 2. Rotate around center
    uv = rotate2d(angle) * (uv - 0.5) + 0.5;

    // 3. Warp: sinusoidal X distortion animated by time
    uv.x += warpAmount * sin(uv.y * 5.0 + time * 2.0 + phase);

    return uv;
}

// Compute single layer's grating value from transformed UV
float computeLayer(MoireLayer l) {
    vec2 uv = fragTexCoord;
    uv = transformUV(uv, l.scale, l.angle, l.warpAmount, l.phase);
    return evaluateGrating(uv, l.frequency, l.phase);
}

void main() {
    int count = clamp(layerCount, 1, 4);
    float result;

    if (sharpMode != 0) {
        // Additive average for binary gratings — preserves spatial contrast
        result = computeLayer(layer0);
        if (count >= 2) result += computeLayer(layer1);
        if (count >= 3) result += computeLayer(layer2);
        if (count >= 4) result += computeLayer(layer3);
        result /= float(count);
    } else {
        // Multiplicative with pow normalization for smooth gratings
        result = computeLayer(layer0);
        if (count >= 2) result *= computeLayer(layer1);
        if (count >= 3) result *= computeLayer(layer2);
        if (count >= 4) result *= computeLayer(layer3);
        result = pow(result, 1.0 / float(count));
    }

    // Color: blend between grayscale and gradient LUT
    vec3 gray = vec3(result);
    vec3 lutColor = textureLod(gradientLUT, vec2(result, 0.5), 0.0).rgb;
    vec3 color = mix(gray, lutColor, colorIntensity);

    finalColor = vec4(color * globalBrightness, 1.0);
}
