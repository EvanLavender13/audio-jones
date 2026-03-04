#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // waveform ring buffer
uniform sampler2D previousFrame;  // ping-pong read buffer (trail persistence)
uniform sampler2D colorLUT;       // color mapping
uniform vec2 resolution;
uniform float aspect;
uniform float waveScale;
uniform float falloff;
uniform float visualGain;
uniform int contourCount;
uniform int bufferSize;
uniform int writeIndex;
uniform float value;              // brightness from color config HSV
uniform vec2 sources[8];          // animated source positions (CPU-computed)
uniform int sourceCount;
uniform int boundaries;           // bool as int
uniform float reflectionGain;
uniform float decayFactor;        // exp(-0.693147 * dt / halfLife)

// Fetch waveform with linear interpolation at ring buffer offset
float fetchWaveform(float delay) {
    delay = min(delay, float(bufferSize) * 0.9);
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    float u = clamp(idx / float(bufferSize), 0.001, 0.999);
    float val = texture(texture0, vec2(u, 0.5)).r;
    return val * 2.0 - 1.0;
}

void main() {
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
        float attenuation = exp(-dist * dist * falloff);
        totalWave += fetchWaveform(delay) * attenuation;

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
                float mAtten = exp(-mDist * mDist * falloff) * reflectionGain;
                totalWave += fetchWaveform(mDelay) * mAtten;
            }
        }
    }

    // Optional contour banding
    float wave = totalWave;
    if (contourCount > 0) {
        wave = floor(totalWave * float(contourCount) + 0.5) / float(contourCount);
    }

    // tanh compression
    float intensity = tanh(wave * visualGain);

    // Color LUT mapping: [-1,1] → [0,1]
    float t = intensity * 0.5 + 0.5;
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
    float brightness = abs(intensity) * value;

    // New frame color
    vec4 newColor = vec4(color * brightness, brightness);

    // Trail persistence: decay previous frame, keep brighter of old/new
    vec4 existing = texture(previousFrame, fragTexCoord) * decayFactor;
    finalColor = max(existing, newColor);
}
