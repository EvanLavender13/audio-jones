#version 430

layout(local_size_x = 16, local_size_y = 16) in;

// Waveform ring buffer texture
layout(binding = 0) uniform sampler2D waveformBuffer;
// Trail map output
layout(rgba32f, binding = 1) uniform image2D trailMap;
// Color lookup table
layout(binding = 3) uniform sampler2D colorLUT;

uniform vec2 resolution;
uniform float waveSpeed;
uniform float falloff;
uniform float visualGain;
uniform int contourCount;
uniform int bufferSize;
uniform int writeIndex;
uniform float value;

// Virtual speaker positions (normalized, center-relative)
const vec2 sources[5] = vec2[](
    vec2(0.0, 0.0),     // Center
    vec2(-0.4, 0.0),    // Left
    vec2(0.4, 0.0),     // Right
    vec2(0.0, -0.4),    // Bottom
    vec2(0.0, 0.4)      // Top
);

// Fetch waveform at ring buffer offset
float fetchWaveform(float delay) {
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    float waveVal = texelFetch(waveformBuffer, ivec2(int(idx), 0), 0).r;
    // Convert from [0,1] packed format back to [-1,1]
    return waveVal * 2.0 - 1.0;
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

    // Sum interference from all sources
    float totalWave = 0.0;
    for (int i = 0; i < 5; i++) {
        vec2 sourcePos = sources[i];
        sourcePos.x *= aspect;  // Apply same aspect correction to source positions
        float dist = length(uv - sourcePos);
        float delay = dist * waveSpeed;
        float amplitude = 1.0 / (1.0 + dist * falloff);
        totalWave += fetchWaveform(delay) * amplitude;
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

    // Apply value (brightness from color config)
    float brightness = abs(intensity) * value;

    // Write to trail map
    vec4 trailColor = vec4(color * brightness, brightness);
    imageStore(trailMap, pos, trailColor);
}
