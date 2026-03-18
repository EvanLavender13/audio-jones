// Bit Crush: Iterative lattice walk producing crunchy grid patterns with FFT-driven brightness.
// A hash-perturbed ceil/floor walk quantizes space into jagged cells tinted by gradient LUT.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec2 center;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

uniform float scale;
uniform float cellSize;
uniform int iterations;
uniform float time;
uniform float glowIntensity;
uniform int walkMode;

float r(vec2 p, float t) {
    return cos(t * cos(p.x * p.y));
}

float rAsym(vec2 p, float t) {
    return cos(t * cos(dot(p, vec2(0.7, 1.3))));
}

void main() {
    // Center-relative, resolution-scaled coordinates
    vec2 p = (fragTexCoord - center) * resolution * scale;

    // Iterative lattice walk
    for (int i = 0; i < iterations; i++) {
        if (walkMode == 1) {
            // Rotating direction vector
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            vec2 dir = vec2(-cos(float(i)), sin(float(i)));
            p = ceil(p + r(ceilCell, time) + r(floorCell, time) * dir);
        } else if (walkMode == 2) {
            // Offset neighborhood query
            vec2 cellA = floor(p / cellSize) + vec2(-1.0, 0.0);
            vec2 cellB = floor(p / cellSize) + vec2(0.0, 1.0);
            p = ceil(p + r(cellA, time) + r(cellB, time) * vec2(-1.0, 1.0));
        } else if (walkMode == 3) {
            // Alternating snap function
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            vec2 stepped = p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0);
            int mode = i % 3;
            if (mode == 0) p = ceil(stepped);
            else if (mode == 1) p = floor(stepped);
            else p = round(stepped);
        } else if (walkMode == 4) {
            // Cross-coupled axes
            vec2 ceilCell = ceil(p / cellSize);
            float rx = r(ceilCell, time);
            float ry = r(ceilCell.yx, time);
            p = ceil(vec2(p.x + rx + ry * 0.5, p.y + ry - rx * 0.5));
        } else if (walkMode == 5) {
            // Asymmetric hash
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            p = ceil(p + rAsym(ceilCell, time) + rAsym(floorCell, time) * vec2(-1.0, 1.0));
        } else {
            // Mode 0 - Original
            vec2 ceilCell = ceil(p / cellSize);
            vec2 floorCell = floor(p / cellSize);
            p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
        }
    }

    // Color via gradient LUT
    float t = fract(dot(p, vec2(0.1, 0.13)));
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

    // FFT brightness modulation per cell - band-averaged
    float freqRatio = maxFreq / baseFreq;
    float bandW = 1.0 / 48.0;
    float ft0 = max(t - bandW * 0.5, 0.0);
    float ft1 = min(t + bandW * 0.5, 1.0);
    float freqLo = baseFreq * pow(freqRatio, ft0);
    float freqHi = baseFreq * pow(freqRatio, ft1);
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
    energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + energy * glowIntensity;

    finalColor = vec4(color * brightness, 1.0);
}
