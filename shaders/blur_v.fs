#version 330

// Vertical pass of separable Gaussian blur + decay
// Combines blur completion with physarum-style evaporation

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;      // Screen resolution for texel size calculation
uniform float halfLife;       // Trail persistence in seconds
uniform float deltaTime;      // Time between frames in seconds
uniform float blurScale;      // Sampling distance multiplier (1.0 = normal, higher = wider bloom)

out vec4 finalColor;

void main()
{
    vec2 texelSize = (1.0 / resolution) * blurScale;

    // 5-tap Gaussian kernel [1, 4, 6, 4, 1] / 16
    vec3 result = vec3(0.0);

    result += texture(texture0, fragTexCoord + vec2(0.0, -2.0 * texelSize.y)).rgb * 0.0625;  // 1/16
    result += texture(texture0, fragTexCoord + vec2(0.0, -1.0 * texelSize.y)).rgb * 0.25;    // 4/16
    result += texture(texture0, fragTexCoord).rgb * 0.375;                                    // 6/16
    result += texture(texture0, fragTexCoord + vec2(0.0,  1.0 * texelSize.y)).rgb * 0.25;    // 4/16
    result += texture(texture0, fragTexCoord + vec2(0.0,  2.0 * texelSize.y)).rgb * 0.0625;  // 1/16

    // Framerate-independent exponential decay (evaporation)
    // Only apply decay when deltaTime > 0 (first pass only in multi-pass bloom)
    float safeHalfLife = max(halfLife, 0.001);
    float decayMultiplier = exp(-0.693147 * deltaTime / safeHalfLife);

    vec3 fadedColor = result * decayMultiplier;

    // Subtract minimum to overcome 8-bit quantization (only on decay pass)
    if (deltaTime > 0.0) {
        fadedColor = max(fadedColor - vec3(0.008), vec3(0.0));
    }

    finalColor = vec4(fadedColor, 1.0);
}
