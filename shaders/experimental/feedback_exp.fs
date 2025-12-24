#version 330

// Experimental feedback pass: 3x3 gaussian blur + half-life decay + zoom
// MilkDrop-style feedback where blur and decay dominate the visual

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float halfLife;     // Decay half-life in seconds
uniform float deltaTime;    // Frame time in seconds
uniform float zoomFactor;   // Zoom toward center per frame (0.99-1.0)

void main()
{
    // Apply zoom toward center first
    vec2 center = vec2(0.5);
    vec2 uv = fragTexCoord - center;
    uv *= zoomFactor;
    uv += center;

    // Clamp to prevent edge artifacts
    uv = clamp(uv, 0.0, 1.0);

    // Fixed 1-pixel texel size for proper 3x3 Gaussian blur
    vec2 texelSize = 1.0 / resolution;

    // 3x3 Gaussian kernel:
    // [1 2 1]
    // [2 4 2] / 16
    // [1 2 1]
    vec3 result = vec3(0.0);

    // Top row
    result += texture(texture0, clamp(uv + vec2(-texelSize.x, -texelSize.y), 0.0, 1.0)).rgb * 0.0625;  // 1/16
    result += texture(texture0, clamp(uv + vec2(0.0,          -texelSize.y), 0.0, 1.0)).rgb * 0.125;   // 2/16
    result += texture(texture0, clamp(uv + vec2( texelSize.x, -texelSize.y), 0.0, 1.0)).rgb * 0.0625;  // 1/16

    // Middle row
    result += texture(texture0, clamp(uv + vec2(-texelSize.x, 0.0), 0.0, 1.0)).rgb * 0.125;   // 2/16
    result += texture(texture0, uv).rgb * 0.25;                                                // 4/16
    result += texture(texture0, clamp(uv + vec2( texelSize.x, 0.0), 0.0, 1.0)).rgb * 0.125;   // 2/16

    // Bottom row
    result += texture(texture0, clamp(uv + vec2(-texelSize.x, texelSize.y), 0.0, 1.0)).rgb * 0.0625;  // 1/16
    result += texture(texture0, clamp(uv + vec2(0.0,          texelSize.y), 0.0, 1.0)).rgb * 0.125;   // 2/16
    result += texture(texture0, clamp(uv + vec2( texelSize.x, texelSize.y), 0.0, 1.0)).rgb * 0.0625;  // 1/16

    // Framerate-independent exponential decay
    float safeHalfLife = max(halfLife, 0.001);
    float decayMultiplier = exp(-0.693147 * deltaTime / safeHalfLife);
    result *= decayMultiplier;

    finalColor = vec4(result, 1.0);
}
