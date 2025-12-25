#version 330

// Experimental feedback pass: 3x3 gaussian blur + half-life decay + spatial flow field
// MilkDrop-style feedback with position-dependent UV displacement

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float halfLife;     // Decay half-life in seconds
uniform float deltaTime;    // Frame time in seconds

// Spatial flow field parameters
uniform float zoomBase;     // Base zoom factor (0.98-1.02)
uniform float zoomRadial;   // Radial zoom coefficient
uniform float rotBase;      // Base rotation in radians
uniform float rotRadial;    // Radial rotation coefficient
uniform float dxBase;       // Base horizontal translation
uniform float dxRadial;     // Radial horizontal coefficient
uniform float dyBase;       // Base vertical translation
uniform float dyRadial;     // Radial vertical coefficient

void main()
{
    vec2 center = vec2(0.5);
    vec2 uv = fragTexCoord - center;

    // Compute aspect-corrected radius (rad=1 is a circle touching shorter edge)
    float aspect = resolution.x / resolution.y;
    vec2 normalized = uv;
    if (aspect > 1.0) {
        normalized.x /= aspect;
    } else {
        normalized.y *= aspect;
    }
    float rad = length(normalized) * 2.0;

    // Compute spatially-varying parameters
    float zoom = zoomBase + rad * zoomRadial;
    float rot = rotBase + rad * rotRadial;
    float dx = dxBase + rad * dxRadial;
    float dy = dyBase + rad * dyRadial;

    // Apply transforms in MilkDrop order: zoom -> rotate -> translate
    uv *= zoom;

    float cosR = cos(rot);
    float sinR = sin(rot);
    uv = vec2(uv.x * cosR - uv.y * sinR, uv.x * sinR + uv.y * cosR);

    uv += vec2(dx, dy);
    uv += center;

    // Fade to black at edges instead of clamping (prevents streak artifacts)
    float edgeFade = 1.0;
    float fadeWidth = 0.02;
    edgeFade *= smoothstep(0.0, fadeWidth, uv.x);
    edgeFade *= smoothstep(0.0, fadeWidth, 1.0 - uv.x);
    edgeFade *= smoothstep(0.0, fadeWidth, uv.y);
    edgeFade *= smoothstep(0.0, fadeWidth, 1.0 - uv.y);

    // Clamp after computing fade
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

    // Apply edge fade to prevent streak artifacts
    result *= edgeFade;

    finalColor = vec4(result, 1.0);
}
