// shaders/scan_bars.fs
#version 330

// Scan Bars: scrolling colored bars with tan() convergence distortion
// Supports linear (angled), spoke (radial fan), and ring (concentric) layouts

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D gradientLUT; // 1D color gradient lookup
uniform vec2 resolution;

// Layout
uniform int mode;              // 0=Linear, 1=Spokes, 2=Rings
uniform float angle;           // Bar orientation (linear mode, radians)

// Bar shape
uniform float barDensity;      // Number of bars across viewport
uniform float sharpness;       // Bar edge hardness (smoothstep width)

// Convergence distortion
uniform float convergence;     // Strength of tan() bunching
uniform float convergenceFreq; // Spatial frequency of convergence warping
uniform float convergenceOffset; // Focal point offset from center

// Animation (phase accumulators driven on CPU)
uniform float scrollPhase;     // Bar position scroll
uniform float colorPhase;      // LUT index drift

// Color chaos
uniform float chaosFreq;       // Spatial frequency for color indexing
uniform float chaosIntensity;  // How wildly adjacent bars jump across palette

// Time quantization
uniform float snapAmount;      // 0 = smooth, higher = lurching motion

#define TAU 6.28318530718
#define TAN_CLAMP 10.0

// Clamp tan() to prevent NaN/Inf near asymptotes
float safeTan(float x) {
    return clamp(tan(x), -TAN_CLAMP, TAN_CLAMP);
}

mat2 rotate2D(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

// Quantize phase for snap/lurch effect
float snapPhase(float phase, float snap) {
    if (snap <= 0.0) return phase;
    return floor(phase) + pow(fract(phase), snap);
}

void main() {
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    // Apply snap quantization to scroll and color phases
    float scroll = snapPhase(scrollPhase, snapAmount);
    float colPhase = snapPhase(colorPhase, snapAmount);

    // =========================================
    // STEP 1: Compute bar coordinate per mode
    // =========================================

    float coord = 0.0;

    if (mode == 1) {
        // Spoke mode: angular position around center
        // atan() returns [-PI, PI], normalize to [0, 1]
        coord = atan(uv.y, uv.x) / TAU + 0.5;

        // Convergence bunches spokes toward angular focal point
        coord += convergence * safeTan(abs(coord - (0.5 + convergenceOffset)) * convergenceFreq);

        // fract() handles the atan() discontinuity at +/-PI seam
        coord = fract(coord);

    } else if (mode == 2) {
        // Ring mode: radial distance from center
        // Clamp minimum radius to avoid degenerate bunching at origin
        coord = max(length(uv), 0.001);

        // Convergence bunches rings toward a specific radius
        coord += convergence * safeTan(abs(coord - (0.5 + convergenceOffset)) * convergenceFreq);

    } else {
        // Linear mode: rotate UV then use x-coordinate
        vec2 rotated = rotate2D(angle) * uv;

        // Convergence bunches bars toward a focal point
        coord = rotated.x + convergence * safeTan(abs(rotated.x - convergenceOffset) * convergenceFreq);
    }

    // =========================================
    // STEP 2: Bar mask from repeating stripe
    // =========================================

    float d = fract(barDensity * coord + scroll);
    float mask = smoothstep(0.5 - sharpness, 0.5, d)
               * smoothstep(0.5 + sharpness, 0.5, d);

    // =========================================
    // STEP 3: Color via LUT
    // =========================================

    float chaos = safeTan(coord * chaosFreq + colPhase);
    float t = fract(chaos * chaosIntensity);
    vec4 color = texture(gradientLUT, vec2(t, 0.5));

    // =========================================
    // STEP 4: Final output
    // =========================================

    finalColor = vec4(color.rgb * mask, 1.0);
}
