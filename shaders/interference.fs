#version 330

// Interference: multi-source wave interference patterns
// Visual modes control wave appearance, color modes control coloring

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;     // Source texture
uniform sampler2D colorLUT;     // 1D color gradient lookup
uniform vec2 resolution;
uniform float time;

// Wave parameters
uniform float waveFreq;         // Spatial frequency of waves
uniform float waveSpeed;        // Temporal animation speed
uniform int sourceCount;        // Number of active sources (1-8)
uniform vec2 sources[8];        // Source positions in centered UV space
uniform float phases[8];        // Per-source phase offsets

// Falloff parameters
uniform int falloffType;        // 0=none, 1=inverse, 2=inverse_square, 3=gaussian
uniform float falloffStrength;  // Falloff rate

// Visualization parameters
uniform int visualMode;         // 0=raw, 1=absolute, 2=contour
uniform int contourCount;       // Number of discrete bands for contour mode
uniform float visualGain;       // Output intensity multiplier

// Color parameters
uniform int colorMode;          // 0=intensity, 1=chromatic
uniform float chromaSpread;     // Wavelength spread for chromatic mode

// Boundary reflection
uniform bool boundaries;        // Enable mirror sources at screen edges
uniform float reflectionGain;   // Amplitude of reflected waves

out vec4 finalColor;

float falloff(float d, int type, float strength) {
    float safeDist = max(d, 0.001);
    if (type == 0) return 1.0;                                    // None
    if (type == 1) return 1.0 / (1.0 + safeDist * strength);      // Inverse
    if (type == 2) return 1.0 / (1.0 + safeDist * safeDist * strength); // Inverse square
    return exp(-safeDist * safeDist * strength);                  // Gaussian (default)
}

float visualize(float wave, int mode, int bands) {
    if (mode == 1) {
        // Absolute: sharper rings with dark nodes
        return abs(wave);
    }
    if (mode == 2) {
        // Contour: discrete bands
        return floor(wave * float(bands) + 0.5) / float(bands);
    }
    // Raw: smooth gradients
    return wave;
}

// Compute single wave contribution from a source position
float waveFromSource(vec2 uv, vec2 src, float phase, float freq) {
    float dist = length(uv - src);
    float wave = sin(dist * freq - time * waveSpeed + phase);
    return wave * falloff(dist, falloffType, falloffStrength);
}

void main()
{
    float aspect = resolution.x / resolution.y;
    vec2 uv = (fragTexCoord - 0.5) * vec2(aspect, 1.0) * 2.0;
    vec4 srcColor = texture(texture0, fragTexCoord);

    // =========================================
    // STEP 1: Compute interference pattern
    // =========================================

    float totalWave = 0.0;

    // Sum waves from all sources
    for (int i = 0; i < sourceCount; i++) {
        totalWave += waveFromSource(uv, sources[i], phases[i], waveFreq);
    }

    // Add mirror sources if boundaries enabled
    if (boundaries) {
        for (int i = 0; i < sourceCount; i++) {
            vec2 src = sources[i];
            vec2 mirrors[4];
            mirrors[0] = vec2(-2.0 * aspect - src.x, src.y);  // Left
            mirrors[1] = vec2( 2.0 * aspect - src.x, src.y);  // Right
            mirrors[2] = vec2(src.x, -2.0 - src.y);           // Bottom
            mirrors[3] = vec2(src.x,  2.0 - src.y);           // Top

            for (int m = 0; m < 4; m++) {
                totalWave += waveFromSource(uv, mirrors[m], phases[i], waveFreq) * reflectionGain;
            }
        }
    }

    // =========================================
    // STEP 2: Apply visual mode
    // =========================================

    float visualWave = visualize(totalWave, visualMode, contourCount);
    visualWave = tanh(visualWave * visualGain);

    // Brightness: raw mode has no dark nodes, absolute/contour do
    float brightness = (visualMode == 0) ? 1.0 : abs(visualWave);

    // =========================================
    // STEP 3: Apply color mode
    // =========================================

    vec3 color;

    if (colorMode == 1) {
        // Chromatic: separate RGB wavelengths
        vec3 chromaScale = vec3(1.0 - chromaSpread, 1.0, 1.0 + chromaSpread);
        vec3 chromaWave = vec3(0.0);

        for (int c = 0; c < 3; c++) {
            for (int i = 0; i < sourceCount; i++) {
                chromaWave[c] += waveFromSource(uv, sources[i], phases[i], waveFreq * chromaScale[c]);
            }

            if (boundaries) {
                for (int i = 0; i < sourceCount; i++) {
                    vec2 src = sources[i];
                    vec2 mirrors[4];
                    mirrors[0] = vec2(-2.0 * aspect - src.x, src.y);
                    mirrors[1] = vec2( 2.0 * aspect - src.x, src.y);
                    mirrors[2] = vec2(src.x, -2.0 - src.y);
                    mirrors[3] = vec2(src.x,  2.0 - src.y);

                    for (int m = 0; m < 4; m++) {
                        chromaWave[c] += waveFromSource(uv, mirrors[m], phases[i], waveFreq * chromaScale[c]) * reflectionGain;
                    }
                }
            }

            // Apply visual mode to each channel
            chromaWave[c] = visualize(chromaWave[c], visualMode, contourCount);
        }

        chromaWave = tanh(chromaWave * visualGain);

        // Chromatic uses wave values directly as RGB
        if (visualMode == 0) {
            // Raw: map [-1,1] to [0,1] for color
            color = chromaWave * 0.5 + 0.5;
        } else {
            // Absolute/Contour: already positive from visualize
            color = abs(chromaWave);
        }
        brightness = 1.0; // Color IS the brightness for chromatic

    } else {
        // Intensity (default): sample LUT based on total wave
        float t = visualWave * 0.5 + 0.5;
        color = texture(colorLUT, vec2(t, 0.5)).rgb;
    }

    // =========================================
    // STEP 4: Final output
    // =========================================

    finalColor = vec4(srcColor.rgb + color * brightness, 1.0);
}
