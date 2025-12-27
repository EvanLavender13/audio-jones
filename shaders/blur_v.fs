#version 330

// Vertical pass of separable Gaussian blur + decay
// Combines blur completion with physarum-style evaporation

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;      // Screen resolution for texel size calculation
uniform float halfLife;       // Trail persistence in seconds
uniform float deltaTime;      // Time between frames in seconds
uniform float blurScale;      // Sampling distance in pixels (0 = no blur, higher = wider bloom)

out vec4 finalColor;

void main()
{
    vec3 result;

    // No blur when scale is 0 - just sample center pixel
    if (blurScale < 0.01) {
        result = texture(texture0, fragTexCoord).rgb;
    } else {
        vec2 texelSize = (1.0 / resolution) * blurScale;

        // 5-tap Gaussian kernel [1, 4, 6, 4, 1] / 16
        result = vec3(0.0);

        // Clamp UVs to prevent wrapping at screen edges
        result += texture(texture0, clamp(fragTexCoord + vec2(0.0, -2.0 * texelSize.y), 0.0, 1.0)).rgb * 0.0625;  // 1/16
        result += texture(texture0, clamp(fragTexCoord + vec2(0.0, -1.0 * texelSize.y), 0.0, 1.0)).rgb * 0.25;    // 4/16
        result += texture(texture0, fragTexCoord).rgb * 0.375;                                                     // 6/16
        result += texture(texture0, clamp(fragTexCoord + vec2(0.0,  1.0 * texelSize.y), 0.0, 1.0)).rgb * 0.25;    // 4/16
        result += texture(texture0, clamp(fragTexCoord + vec2(0.0,  2.0 * texelSize.y), 0.0, 1.0)).rgb * 0.0625;  // 1/16
    }

    // Framerate-independent exponential decay (evaporation)
    float safeHalfLife = max(halfLife, 0.001);
    float decayMultiplier = exp(-0.693147 * deltaTime / safeHalfLife);

    vec3 fadedColor = result * decayMultiplier;

    finalColor = vec4(fadedColor, 1.0);
}
