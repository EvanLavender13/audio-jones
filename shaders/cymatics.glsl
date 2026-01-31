#version 430

layout(local_size_x = 16, local_size_y = 16) in;

// Waveform ring buffer texture
layout(binding = 0) uniform sampler2D waveformBuffer;
// Trail map output
layout(rgba32f, binding = 1) uniform image2D trailMap;
// Color lookup table
layout(binding = 3) uniform sampler2D colorLUT;

uniform vec2 resolution;
uniform float waveScale;
uniform float falloff;
uniform float visualGain;
uniform int contourCount;
uniform int bufferSize;
uniform int writeIndex;
uniform float value;
uniform vec2 sources[8];  // Animated source positions (computed on CPU)
uniform int sourceCount;  // Active source count (1-8)
uniform bool boundaries;
uniform float reflectionGain;

// Fetch waveform with linear interpolation at ring buffer offset
float fetchWaveform(float delay) {
    // Clamp delay to avoid sampling stale wrapped data
    delay = min(delay, float(bufferSize) * 0.9);
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    // Normalize to [0,1] for texture() linear interpolation, with small margin to avoid edge artifacts
    float u = clamp(idx / float(bufferSize), 0.001, 0.999);
    float val = texture(waveformBuffer, vec2(u, 0.5)).r;
    // Convert from [0,1] storage back to [-1,1] waveform
    return val * 2.0 - 1.0;
}


void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= int(resolution.x) || pos.y >= int(resolution.y)) {
        return;
    }

    // Map pixel to normalized coordinates centered at (0,0)
    vec2 uv = (vec2(pos) + 0.5) / resolution;
    uv = uv * 2.0 - 1.0;  // Now in range [-1, 1]

    // Aspect ratio correction
    float aspect = resolution.x / resolution.y;
    uv.x *= aspect;

    // Sum interference from all sources with Gaussian falloff
    float totalWave = 0.0;
    for (int i = 0; i < sourceCount; i++) {
        vec2 sourcePos = sources[i];
        sourcePos.x *= aspect;

        // Real source
        float dist = length(uv - sourcePos);
        float delay = dist * waveScale;
        float attenuation = exp(-dist * dist * falloff);
        totalWave += fetchWaveform(delay) * attenuation;

        // Mirror sources (4 reflections across screen edges)
        if (boundaries) {
            vec2 mirrors[4];
            mirrors[0] = vec2(-2.0 * aspect - sourcePos.x, sourcePos.y);  // Left
            mirrors[1] = vec2( 2.0 * aspect - sourcePos.x, sourcePos.y);  // Right
            mirrors[2] = vec2(sourcePos.x, -2.0 - sourcePos.y);           // Bottom
            mirrors[3] = vec2(sourcePos.x,  2.0 - sourcePos.y);           // Top

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

    // Output compression via tanh
    float intensity = tanh(wave * visualGain);

    // Map [-1,1] to [0,1] for LUT sampling
    float t = intensity * 0.5 + 0.5;
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;

    // Brightness from intensity magnitude - silence is black
    float brightness = abs(intensity) * value;

    // Blend with existing trail data for persistence
    vec4 newColor = vec4(color * brightness, brightness);
    vec4 existing = imageLoad(trailMap, pos);
    vec4 blended = max(existing, newColor);  // Keep brighter of old/new
    imageStore(trailMap, pos, blended);
}
