#version 330

// Interference: multi-source wave interference patterns with falloff and color modes

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
uniform int colorMode;          // 0=intensity, 1=per_source
uniform bool chromatic;         // RGB wavelength separation
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

float visualize(float wave, int mode, int bands, float gain) {
    if (mode == 1) {
        // Absolute: sharper rings with dark nodes
        return abs(wave) * gain;
    }
    if (mode == 2) {
        // Contour: discrete bands
        return floor(wave * float(bands) + 0.5) / float(bands) * gain;
    }
    // Raw: smooth gradients
    return wave * gain;
}

void main()
{
    float aspect = resolution.x / resolution.y;

    // Convert fragTexCoord to centered UV with aspect correction
    vec2 uv = (fragTexCoord - 0.5) * vec2(aspect, 1.0) * 2.0;

    vec4 srcColor = texture(texture0, fragTexCoord);

    if (chromatic) {
        // Chromatic mode: compute separate wavelengths for R/G/B channels
        vec3 chromaScale = vec3(1.0 - chromaSpread, 1.0, 1.0 + chromaSpread);
        vec3 wave = vec3(0.0);

        for (int c = 0; c < 3; c++) {
            wave[c] = 0.0;
            for (int i = 0; i < sourceCount; i++) {
                float dist = length(uv - sources[i]);
                float w = sin(dist * waveFreq * chromaScale[c] - time * waveSpeed + phases[i]);
                wave[c] += w * falloff(dist, falloffType, falloffStrength);
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
                        float dist = length(uv - mirrors[m]);
                        float w = sin(dist * waveFreq * chromaScale[c] - time * waveSpeed + phases[i]);
                        wave[c] += w * falloff(dist, falloffType, falloffStrength) * reflectionGain;
                    }
                }
            }
        }

        wave = abs(wave); // Dark nodes where waves cancel
        wave = tanh(wave * visualGain); // Tonemap

        finalColor = vec4(srcColor.rgb + wave, 1.0);
        return;
    }

    if (colorMode == 1) {
        // PerSource: each source samples LUT centered at its uniform position
        vec3 color = vec3(0.0);

        for (int i = 0; i < sourceCount; i++) {
            float dist = length(uv - sources[i]);
            float wave = sin(dist * waveFreq - time * waveSpeed + phases[i]);
            float atten = falloff(dist, falloffType, falloffStrength);

            // Sample LUT centered at source's position, wave shifts within neighborhood
            float baseOffset = (float(i) + 0.5) / float(sourceCount);
            float neighborhoodSize = 0.5 / float(sourceCount);
            float lutPos = baseOffset + wave * neighborhoodSize;
            vec3 srcLutColor = texture(colorLUT, vec2(lutPos, 0.5)).rgb;

            color += srcLutColor * abs(wave) * atten;
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
                    float dist = length(uv - mirrors[m]);
                    float wave = sin(dist * waveFreq - time * waveSpeed + phases[i]);
                    float atten = falloff(dist, falloffType, falloffStrength) * reflectionGain;

                    float baseOffset = (float(i) + 0.5) / float(sourceCount);
                    float neighborhoodSize = 0.5 / float(sourceCount);
                    float lutPos = baseOffset + wave * neighborhoodSize;
                    vec3 srcLutColor = texture(colorLUT, vec2(lutPos, 0.5)).rgb;

                    color += srcLutColor * abs(wave) * atten;
                }
            }
        }

        color = tanh(color * visualGain);

        finalColor = vec4(srcColor.rgb + color, 1.0);
        return;
    }

    // Intensity mode (colorMode == 0): sum all waves, map to LUT
    float totalWave = 0.0;

    for (int i = 0; i < sourceCount; i++) {
        float dist = length(uv - sources[i]);
        float wave = sin(dist * waveFreq - time * waveSpeed + phases[i]);
        float atten = falloff(dist, falloffType, falloffStrength);
        totalWave += wave * atten;
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
                float dist = length(uv - mirrors[m]);
                float wave = sin(dist * waveFreq - time * waveSpeed + phases[i]);
                float atten = falloff(dist, falloffType, falloffStrength) * reflectionGain;
                totalWave += wave * atten;
            }
        }
    }

    // Apply visualization mode
    totalWave = visualize(totalWave, visualMode, contourCount, 1.0);

    // Tonemap
    totalWave = tanh(totalWave * visualGain);

    // Map [-1,1] to [0,1] for LUT sampling
    float t = totalWave * 0.5 + 0.5;
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
    float brightness = abs(totalWave);

    // Additive blend with source texture
    finalColor = vec4(srcColor.rgb + color * brightness, 1.0);
}
