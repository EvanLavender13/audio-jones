#version 330

// Voronoi edge post-process effect
// Overlays animated cell boundaries on the accumulated frame

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float scale;      // Cell count across screen width
uniform float intensity;  // Blend amount (0 = bypass)
uniform float time;       // Animation driver
uniform float edgeWidth;  // Edge thickness

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

// Hash function for pseudo-random 2D point
vec2 hash2(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

void main()
{
    // Bypass when disabled
    if (intensity <= 0.0) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    // Scale UV to create cells (maintain aspect ratio)
    float aspect = resolution.x / resolution.y;
    vec2 st = fragTexCoord * vec2(scale * aspect, scale);

    vec2 i_st = floor(st);  // Tile index
    vec2 f_st = fract(st);  // Position within tile

    float m_dist = 100.0;   // Closest distance
    float m_dist2 = 100.0;  // Second closest for edge detection

    // Search 3x3 neighborhood for closest points
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(i_st + neighbor);

            // Animate point position
            point = 0.5 + 0.5 * sin(time + TWO_PI * point);

            float dist = length(neighbor + point - f_st);

            if (dist < m_dist) {
                m_dist2 = m_dist;
                m_dist = dist;
            } else if (dist < m_dist2) {
                m_dist2 = dist;
            }
        }
    }

    // Edge detection from distance difference
    float edge = m_dist2 - m_dist;
    float edgeMask = 1.0 - smoothstep(0.0, edgeWidth, edge);

    // Brighten original color on edges (reveals underlying content)
    // Clamp input to prevent HDR runaway with multiplicative boost
    vec3 original = min(texture(texture0, fragTexCoord).rgb, vec3(1.0));

    // Headroom-limited boost - reduce boost on already-bright pixels
    float maxChan = max(original.r, max(original.g, original.b));
    float headroom = 1.0 - maxChan;
    vec3 boosted = original * (1.0 + edgeMask * intensity * headroom);

    finalColor = vec4(boosted, 1.0);
}
