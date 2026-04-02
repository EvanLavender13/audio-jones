// shaders/scan_bars.fs
#version 330

// Scan Bars: scrolling colored bars with tan() convergence distortion
// Supports linear (angled), spoke (radial fan), ring (concentric), and grid layouts

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D gradientLUT; // 1D color gradient lookup
uniform sampler2D fftTexture;  // FFT magnitude data
uniform vec2 resolution;

// Layout
uniform int mode;              // 0=Linear, 1=Spokes, 2=Rings, 3=Grid
uniform float angle;           // Bar orientation (linear/grid modes, radians)

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

// FFT audio reactivity
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

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

void main() {
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    // Snap quantization already applied on CPU before sending uniforms
    float scroll = scrollPhase;
    float colPhase = colorPhase;

    // =========================================
    // STEP 1: Compute bar coordinate per mode
    // =========================================

    float coord = 0.0;
    float mask = 0.0;  // Hoisted from STEP 2 for grid mode

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

    } else if (mode == 3) {
        // Grid mode: two orthogonal sets of linear bars
        vec2 gridUV = rotate2D(angle) * uv;

        float coordX = gridUV.x + convergence * safeTan(abs(gridUV.x - convergenceOffset) * convergenceFreq);
        float coordY = gridUV.y + convergence * safeTan(abs(gridUV.y - convergenceOffset) * convergenceFreq);

        float dX = fract(barDensity * coordX + scroll);
        float dY = fract(barDensity * coordY + scroll);
        float maskX = smoothstep(0.5 - sharpness, 0.5, dX)
                    * smoothstep(0.5 + sharpness, 0.5, dX);
        float maskY = smoothstep(0.5 - sharpness, 0.5, dY)
                    * smoothstep(0.5 + sharpness, 0.5, dY);

        mask = max(maskX, maskY);
        coord = (maskX >= maskY) ? coordX : coordY;

    } else {
        // Linear mode: rotate UV then use x-coordinate
        vec2 rotated = rotate2D(angle) * uv;

        // Convergence bunches bars toward a focal point
        coord = rotated.x + convergence * safeTan(abs(rotated.x - convergenceOffset) * convergenceFreq);
    }

    // =========================================
    // STEP 2: Bar mask from repeating stripe
    // =========================================

    if (mode != 3) {
        float barCoord = barDensity * coord + scroll;
        float d = fract(barCoord);
        mask = smoothstep(0.5 - sharpness, 0.5, d)
             * smoothstep(0.5 + sharpness, 0.5, d);
    }

    // =========================================
    // STEP 3: Color via LUT
    // =========================================

    float chaos = safeTan(coord * chaosFreq + colPhase);
    float t = fract(chaos * chaosIntensity);
    vec4 color = texture(gradientLUT, vec2(t, 0.5));

    // =========================================
    // STEP 4: Band-averaged FFT lookup
    // =========================================

    float freqRatio = maxFreq / baseFreq;
    float bandW = 1.0 / 48.0;
    float t0 = max(t - bandW * 0.5, 0.0);
    float t1 = min(t + bandW * 0.5, 1.0);
    float freqLo = baseFreq * pow(freqRatio, t0);
    float freqHi = baseFreq * pow(freqRatio, t1);
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
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

    // =========================================
    // STEP 5: Final output
    // =========================================

    float react = baseBright + mag;
    finalColor = vec4(color.rgb * mask * react, 1.0);
}
