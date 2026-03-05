#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;        // ping-pong read buffer (auto-bound by DrawTextureRec)
uniform sampler2D waveformTexture; // waveform ring buffer
uniform sampler2D colorLUT;       // color mapping
uniform vec2 resolution;
uniform float aspect;
uniform float waveScale;
uniform float falloff;
uniform float visualGain;
uniform int contourCount;
uniform int contourMode;          // 0=off, 1=bands, 2=iso-lines
uniform int bufferSize;
uniform int writeIndex;
uniform float value;              // brightness from color config HSV
uniform vec2 sources[8];          // animated source positions (CPU-computed)
uniform int sourceCount;
uniform int boundaries;           // bool as int
uniform float reflectionGain;
uniform int diffusionScale;       // spatial blur tap spacing (0=off)
uniform float decayFactor;        // exp(-0.693147 * dt / halfLife)

// Fetch waveform with linear interpolation at ring buffer offset
float fetchWaveform(float delay) {
    delay = min(delay, float(bufferSize) * 0.9);
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    float u = clamp(idx / float(bufferSize), 0.001, 0.999);
    float val = texture(waveformTexture, vec2(u, 0.5)).r;
    return val * 2.0 - 1.0;
}

void main() {
    const float EPSILON = 0.01; // softens 1/0 singularity; max multiplier = 10x

    // Map to centered, aspect-corrected coordinates [-1,1] (y) [-aspect,aspect] (x)
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= aspect;

    // Sum interference from all sources with Gaussian falloff
    float totalWave = 0.0;
    for (int i = 0; i < sourceCount; i++) {
        vec2 sourcePos = sources[i];
        sourcePos.x *= aspect;

        float dist = length(uv - sourcePos);
        float delay = dist * waveScale;
        float spreading = 1.0 / sqrt(dist + EPSILON);
        float envelope = exp(-dist * dist * falloff);
        totalWave += fetchWaveform(delay) * spreading * envelope;

        // Mirror sources for boundary reflections (4 per source)
        if (boundaries != 0) {
            vec2 mirrors[4];
            mirrors[0] = vec2(-2.0 * aspect - sourcePos.x, sourcePos.y);
            mirrors[1] = vec2( 2.0 * aspect - sourcePos.x, sourcePos.y);
            mirrors[2] = vec2(sourcePos.x, -2.0 - sourcePos.y);
            mirrors[3] = vec2(sourcePos.x,  2.0 - sourcePos.y);

            for (int m = 0; m < 4; m++) {
                float mDist = length(uv - mirrors[m]);
                float mDelay = mDist * waveScale;
                float mSpreading = 1.0 / sqrt(mDist + EPSILON);
                float mEnvelope = exp(-mDist * mDist * falloff);
                float mAtten = mSpreading * mEnvelope * reflectionGain;
                totalWave += fetchWaveform(mDelay) * mAtten;
            }
        }
    }
    totalWave /= float(sourceCount);

    // Contour modes
    float wave = totalWave;
    if (contourMode == 1) {
        // Bands: quantize to steps (existing posterize logic)
        wave = floor(totalWave * float(contourCount) + 0.5) / float(contourCount);
    } else if (contourMode == 2) {
        // Iso-amplitude lines: thin bright lines at each contour level
        float contoured = fract(abs(totalWave) * float(contourCount));
        float lineWidth = 0.1;
        wave = totalWave * smoothstep(0.0, lineWidth, contoured)
                         * smoothstep(lineWidth * 2.0, lineWidth, contoured);
    }

    // Color: full gradient always, gain-independent
    float t = tanh(wave) * 0.5 + 0.5;
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;

    // Brightness: wave amplitude determines visibility, gain controls reach
    float brightness = abs(tanh(wave * visualGain)) * value;
    vec4 newColor = vec4(color * brightness, brightness);

    // Trail persistence: diffuse + decay previous frame, keep brighter of old/new
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
