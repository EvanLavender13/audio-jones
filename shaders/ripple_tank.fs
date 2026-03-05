#version 330

// Ripple Tank: merged audio waveform + sine wave interference with trail persistence

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;        // ping-pong read buffer (auto-bound by DrawTextureRec)
uniform sampler2D waveformTexture; // waveform ring buffer
uniform sampler2D colorLUT;        // color mapping
uniform vec2 resolution;
uniform float aspect;

// Wave parameters
uniform int waveSource;            // 0=audio, 1=sine
uniform float waveScale;           // audio: delay scaling
uniform float waveFreq;            // sine: spatial frequency
uniform float time;                // sine: accumulated time
uniform float falloffStrength;     // falloff rate
uniform int falloffType;           // 0=none, 1=inverse, 2=inverse_square, 3=gaussian
uniform float visualGain;
uniform int contourCount;
uniform int visualMode;            // 0=raw, 1=absolute, 2=bands, 3=iso-lines
uniform int bufferSize;
uniform int writeIndex;
uniform float value;               // brightness from color config HSV

// Source positions
uniform vec2 sources[8];           // animated source positions (CPU-computed)
uniform float phases[8];           // per-source phase offsets
uniform int sourceCount;

// Boundary reflection
uniform int boundaries;            // bool as int
uniform float reflectionGain;

// Trail persistence
uniform int diffusionScale;        // spatial blur tap spacing (0=off)
uniform float decayFactor;         // exp(-0.693147 * dt / halfLife)

// Color parameters
uniform int colorMode;             // 0=intensity, 1=per_source, 2=chromatic
uniform float chromaSpread;        // wavelength spread for chromatic mode

// Distance-based falloff with selectable type
float falloff(float d, int type, float strength) {
    float safeDist = max(d, 0.001);
    if (type == 0) return 1.0;                                          // None
    if (type == 1) return 1.0 / (1.0 + safeDist * strength);            // Inverse
    if (type == 2) return 1.0 / (1.0 + safeDist * safeDist * strength);  // Inverse square
    return exp(-safeDist * safeDist * strength);                        // Gaussian
}

// Fetch waveform with linear interpolation at ring buffer offset
float fetchWaveform(float delay) {
    delay = min(delay, float(bufferSize) * 0.9);
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    float u = clamp(idx / float(bufferSize), 0.001, 0.999);
    float val = texture(waveformTexture, vec2(u, 0.5)).r;
    return val * 2.0 - 1.0;
}

// Compute wave contribution from a single source position
float waveFromSource(vec2 uv, vec2 src, float phase) {
    float dist = length(uv - src);
    float atten = falloff(dist, falloffType, falloffStrength);

    if (waveSource == 0) {
        // Audio: sample waveform ring buffer with distance-based delay
        float delay = dist * waveScale;
        float spreading = 1.0 / sqrt(dist + 0.01);
        return fetchWaveform(delay) * spreading * atten;
    } else {
        // Sine: parametric standing wave
        return sin(dist * waveFreq - time + phase) * atten;
    }
}

// Overload for chromatic mode with per-channel frequency scaling
float waveFromSource(vec2 uv, vec2 src, float phase, float freq) {
    float dist = length(uv - src);
    float atten = falloff(dist, falloffType, falloffStrength);

    if (waveSource == 0) {
        // Scale delay by freq/waveFreq ratio for per-channel chromatic separation
        float delay = dist * waveScale * (freq / waveFreq);
        float spreading = 1.0 / sqrt(dist + 0.01);
        return fetchWaveform(delay) * spreading * atten;
    } else {
        return sin(dist * freq - time + phase) * atten;
    }
}

// Apply visualization mode to wave value
float visualize(float wave, int mode, int bands) {
    if (mode == 1) return abs(wave);                                     // Absolute
    if (mode == 2) return floor(wave * float(bands) + 0.5) / float(bands); // Bands
    if (mode == 3) {                                                     // Iso-lines
        float contoured = fract(abs(wave) * float(bands));
        float lineWidth = 0.1;
        return wave * smoothstep(0.0, lineWidth, contoured)
                     * smoothstep(lineWidth * 2.0, lineWidth, contoured);
    }
    return wave;                                                         // Raw
}

void main() {
    // Map to centered, aspect-corrected coordinates [-1,1] (y) [-aspect,aspect] (x)
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= aspect;

    // =========================================
    // STEP 1: Sum interference from all sources
    // =========================================

    float totalWave = 0.0;
    for (int i = 0; i < sourceCount; i++) {
        vec2 sourcePos = sources[i];
        sourcePos.x *= aspect;
        totalWave += waveFromSource(uv, sourcePos, phases[i]);

        // Mirror sources for boundary reflections (4 per source)
        if (boundaries != 0) {
            vec2 mirrors[4];
            mirrors[0] = vec2(-2.0 * aspect - sourcePos.x, sourcePos.y);
            mirrors[1] = vec2( 2.0 * aspect - sourcePos.x, sourcePos.y);
            mirrors[2] = vec2(sourcePos.x, -2.0 - sourcePos.y);
            mirrors[3] = vec2(sourcePos.x,  2.0 - sourcePos.y);

            for (int m = 0; m < 4; m++) {
                totalWave += waveFromSource(uv, mirrors[m], phases[i]) * reflectionGain;
            }
        }
    }
    totalWave /= float(sourceCount);

    // =========================================
    // STEP 2: Apply visual mode
    // =========================================

    float visualWave = visualize(totalWave, visualMode, contourCount);

    // =========================================
    // STEP 3: Apply color mode
    // =========================================

    vec3 color;
    float brightness;

    if (colorMode == 2) {
        // Chromatic: separate RGB wavelengths
        vec3 chromaScale = vec3(1.0 - chromaSpread, 1.0, 1.0 + chromaSpread);
        vec3 chromaWave = vec3(0.0);

        for (int c = 0; c < 3; c++) {
            for (int i = 0; i < sourceCount; i++) {
                vec2 sourcePos = sources[i];
                sourcePos.x *= aspect;
                chromaWave[c] += waveFromSource(uv, sourcePos, phases[i], waveFreq * chromaScale[c]);

                if (boundaries != 0) {
                    vec2 mirrors[4];
                    mirrors[0] = vec2(-2.0 * aspect - sourcePos.x, sourcePos.y);
                    mirrors[1] = vec2( 2.0 * aspect - sourcePos.x, sourcePos.y);
                    mirrors[2] = vec2(sourcePos.x, -2.0 - sourcePos.y);
                    mirrors[3] = vec2(sourcePos.x,  2.0 - sourcePos.y);

                    for (int m = 0; m < 4; m++) {
                        chromaWave[c] += waveFromSource(uv, mirrors[m], phases[i], waveFreq * chromaScale[c]) * reflectionGain;
                    }
                }
            }
            chromaWave[c] /= float(sourceCount);

            // Apply visual mode to each channel
            chromaWave[c] = visualize(chromaWave[c], visualMode, contourCount);
        }

        color = tanh(chromaWave * visualGain);
        brightness = 1.0; // Color IS the brightness for chromatic

    } else if (colorMode == 1) {
        // PerSource: proximity-weighted LUT shift
        float shift = 0.0;
        float totalWeight = 0.0;

        for (int i = 0; i < sourceCount; i++) {
            vec2 sourcePos = sources[i];
            sourcePos.x *= aspect;
            float dist = length(uv - sourcePos);
            float weight = 1.0 / (dist + 0.1);

            // Source position: 0 for first, 1 for last -> map to [-0.5, +0.5]
            float srcShift = float(i) / float(max(sourceCount - 1, 1)) - 0.5;
            shift += srcShift * weight;
            totalWeight += weight;
        }
        shift /= totalWeight;

        float t = abs(visualWave) * 0.5 + 0.5 + shift;
        t = clamp(t, 0.0, 1.0);
        color = texture(colorLUT, vec2(t, 0.5)).rgb;
        brightness = abs(tanh(visualWave * visualGain)) * value;

    } else {
        // Intensity (default): map visualized wave to LUT
        float t;
        if (visualMode == 0) {
            // Raw: wave spans [-1,1], map to [0,1]
            t = tanh(visualWave) * 0.5 + 0.5;
        } else {
            // Other modes: already in useful range
            t = tanh(visualWave);
        }
        color = texture(colorLUT, vec2(t, 0.5)).rgb;
        brightness = abs(tanh(visualWave * visualGain)) * value;
    }

    vec4 newColor = vec4(color * brightness, brightness);

    // =========================================
    // STEP 4: Trail persistence
    // =========================================

    // Diffuse + decay previous frame, keep brighter of old/new
    vec4 existing;
    if (diffusionScale == 0) {
        existing = texture(texture0, fragTexCoord);
    } else {
        // Separable 5-tap Gaussian approximation via cross sampling
        // Weights: [0.0625, 0.25, 0.375, 0.25, 0.0625]
        vec2 texel = vec2(float(diffusionScale)) / resolution;

        vec4 h = texture(texture0, fragTexCoord + vec2(-2.0 * texel.x, 0.0)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(-1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2( 1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord + vec2( 2.0 * texel.x, 0.0)) * 0.0625;

        vec4 v = texture(texture0, fragTexCoord + vec2(0.0, -2.0 * texel.y)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(0.0, -1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2(0.0,  1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord + vec2(0.0,  2.0 * texel.y)) * 0.0625;

        existing = (h + v) * 0.5;
    }
    existing *= decayFactor;
    finalColor = max(existing, newColor);
}
