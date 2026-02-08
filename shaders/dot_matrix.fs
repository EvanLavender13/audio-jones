#version 330

// Dot matrix: circular LED dots with glow halo on a rotated grid

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float dotScale;    // Grid cell count factor
uniform float softness;    // Blend from crisp circles (low) to glow halo (high)
uniform float brightness;  // Output intensity multiplier
uniform float rotation;    // Grid rotation angle

out vec4 finalColor;

mat2 rotm(float r) {
    float c = cos(r), s = sin(r);
    return mat2(c, -s, s, c);
}

void main()
{
    const float DOT_RADIUS = 0.7;
    const float DOT_EDGE = 0.05;       // smoothstep antialiasing half-width
    const float GLOW_KERN_S = 1.2;     // inverse-cube kernel sharpness
    const float GLOW_WINDOW = 0.0625;  // 1/16 — squared-distance window falloff

    vec2 ratio = vec2(1.0, resolution.x / resolution.y);
    vec2 fc = fragTexCoord * resolution;
    mat2 m = rotm(rotation);
    mat2 mt = transpose(m);

    // Pixel position in grid space
    vec2 gridUV = m * (fc * ratio / dotScale);
    vec2 cellBase = floor(gridUV);
    vec2 frac = fract(gridUV);

    vec3 result = vec3(0.0);

    // softness normalized to [0,1] blend factor
    float t = clamp((softness - 0.2) / 3.8, 0.0, 1.0);

    // Accumulate from 3x3 neighborhood — glow bleeds across cell boundaries
    for (float j = -1.0; j <= 1.0; j += 1.0)
    for (float i = -1.0; i <= 1.0; i += 1.0)
    {
        vec2 off = vec2(i, j);

        // Distance from pixel to this cell's dot center
        vec2 d = (frac - off) * 2.0 - 1.0;
        float dist = length(d);

        // Hard circle dot with antialiased edge
        float dotMask = 1.0 - smoothstep(DOT_RADIUS - DOT_EDGE, DOT_RADIUS + DOT_EDGE, dist);

        // Glow dot: solid core inside radius, inverse-cube falloff beyond
        float glowDist = max(0.0, dist - DOT_RADIUS);
        float glowMask = GLOW_KERN_S / pow(1.0 + GLOW_KERN_S * glowDist, 3.0);
        glowMask *= clamp(1.0 - glowDist * glowDist * GLOW_WINDOW, 0.0, 1.0);
        glowMask = clamp(glowMask, 0.0, 1.0);

        // Softness blends: low = crisp circles, high = glow halo
        float intensity = mix(dotMask, glowMask, t);

        // Sample texture at this cell's center
        vec2 cellCenter = mt * (cellBase + off + 0.5);
        vec2 texUV = cellCenter * dotScale / ratio / resolution;
        vec3 texColor = texture(texture0, texUV).rgb;
        float luma = dot(texColor, vec3(0.299, 0.587, 0.114));

        result += texColor * luma * intensity;
    }

    // Tonemap to prevent clamp saturation from neighbor accumulation
    result = result / (1.0 + result) * brightness;
    finalColor = vec4(result, 1.0);
}
