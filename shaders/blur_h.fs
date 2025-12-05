#version 330

// Horizontal pass of separable Gaussian blur
// Physarum-style diffusion: spreads trails outward

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;  // Screen resolution for texel size calculation

out vec4 finalColor;

void main()
{
    vec2 texelSize = 1.0 / resolution;

    // 5-tap Gaussian kernel [1, 4, 6, 4, 1] / 16
    // Wider than 3-tap for smoother organic diffusion
    vec3 result = vec3(0.0);

    result += texture(texture0, fragTexCoord + vec2(-2.0 * texelSize.x, 0.0)).rgb * 0.0625;  // 1/16
    result += texture(texture0, fragTexCoord + vec2(-1.0 * texelSize.x, 0.0)).rgb * 0.25;    // 4/16
    result += texture(texture0, fragTexCoord).rgb * 0.375;                                    // 6/16
    result += texture(texture0, fragTexCoord + vec2( 1.0 * texelSize.x, 0.0)).rgb * 0.25;    // 4/16
    result += texture(texture0, fragTexCoord + vec2( 2.0 * texelSize.x, 0.0)).rgb * 0.0625;  // 1/16

    finalColor = vec4(result, 1.0);
}
