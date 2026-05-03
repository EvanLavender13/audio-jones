#version 330

// Horizontal pass of separable Gaussian blur
// Physarum-style diffusion: spreads trails outward

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;  // Screen resolution for texel size calculation
uniform float blurScale;  // Sampling distance in pixels (0 = no blur, higher = wider bloom)

out vec4 finalColor;

void main()
{
    // No blur when scale is 0 - just pass through
    if (blurScale < 0.01) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    // Dense unit-stride Gaussian: sigma = blurScale, radius = ceil(3 sigma)
    vec2 texelSize = 1.0 / resolution;

    float sigma = blurScale;
    float twoSigma2 = 2.0 * sigma * sigma;
    int radius = int(ceil(3.0 * sigma));

    vec3 result = vec3(0.0);
    float wsum = 0.0;
    for (int i = -radius; i <= radius; i++) {
        float w = exp(-float(i * i) / twoSigma2);
        vec2 uv = clamp(fragTexCoord + vec2(float(i) * texelSize.x, 0.0), 0.0, 1.0);
        result += texture(texture0, uv).rgb * w;
        wsum += w;
    }
    result /= wsum;

    finalColor = vec4(result, 1.0);
}
