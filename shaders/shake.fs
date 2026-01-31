// shaders/shake.fs
#version 330

uniform sampler2D inputTex;
uniform vec2 resolution;
uniform float time;

uniform float intensity;    // 0.0 - 0.2
uniform int samples;        // 1 - 16
uniform float rate;         // Hz
uniform bool gaussian;      // distribution type

in vec2 fragTexCoord;
out vec4 fragColor;

// Hash function for deterministic pseudo-random values
float hash(float n) { return fract(sin(n) * 43758.5453123); }

void main() {
    vec2 uv = fragTexCoord;

    if (samples < 1 || intensity <= 0.0) {
        fragColor = texture(inputTex, uv);
        return;
    }

    // Discrete time step for coherent shake (same offset for all pixels)
    float frameIndex = floor(time * rate);

    vec3 result = vec3(0.0);

    for (int i = 0; i < samples; i++) {
        // Unique seed per sample, but SAME for all pixels in this frame
        float sampleSeed = frameIndex * 16.0 + float(i);

        vec2 jitter;

        if (gaussian) {
            // Box-Muller transform for Gaussian distribution
            float u1 = hash(sampleSeed);
            float u2 = hash(sampleSeed + 0.5);
            float r = sqrt(-2.0 * log(max(u1, 0.0001)));
            float theta = 6.28318 * u2;
            jitter = vec2(cos(theta), sin(theta)) * r * 0.5 * intensity;
        } else {
            // Uniform distribution in [-1, 1]
            float rx = hash(sampleSeed);
            float ry = hash(sampleSeed + 0.5);
            jitter = (vec2(rx, ry) * 2.0 - 1.0) * intensity;
        }

        result += texture(inputTex, uv + jitter).rgb;
    }

    fragColor = vec4(result / float(samples), 1.0);
}
