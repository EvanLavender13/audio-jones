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

    vec2 texelSize = (1.0 / resolution) * blurScale;

    // 5-tap Gaussian kernel [1, 4, 6, 4, 1] / 16
    // Wider than 3-tap for smoother organic diffusion
    vec3 result = vec3(0.0);

    // Clamp UVs to prevent wrapping at screen edges
    result += texture(texture0, clamp(fragTexCoord + vec2(-2.0 * texelSize.x, 0.0), 0.0, 1.0)).rgb * 0.0625;  // 1/16
    result += texture(texture0, clamp(fragTexCoord + vec2(-1.0 * texelSize.x, 0.0), 0.0, 1.0)).rgb * 0.25;    // 4/16
    result += texture(texture0, fragTexCoord).rgb * 0.375;                                                     // 6/16
    result += texture(texture0, clamp(fragTexCoord + vec2( 1.0 * texelSize.x, 0.0), 0.0, 1.0)).rgb * 0.25;    // 4/16
    result += texture(texture0, clamp(fragTexCoord + vec2( 2.0 * texelSize.x, 0.0), 0.0, 1.0)).rgb * 0.0625;  // 1/16

    finalColor = vec4(result, 1.0);
}
