// shaders/multi_scale_grid.fs
#version 330

// Multi-Scale Grid: overlapping wavy grids at three zoom levels,
// sampling input texture per cell with parallax scroll and glow.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;

uniform float scale1;       // Coarse grid density
uniform float scale2;       // Medium grid density
uniform float scale3;       // Fine grid density
uniform float scrollSpeed;  // Base drift rate
uniform float warpAmount;   // Sine distortion on grid lines
uniform float edgeContrast; // Cell boundary darkness
uniform float edgePower;    // Edge sharpness
uniform float glowThreshold; // Brightness cutoff for glow
uniform float glowAmount;   // Glow intensity multiplier
uniform int glowMode;       // 0=squared glow, 1=linear glow
uniform float time;

// Sine-warped grid: organic waviness from subtracting sin(uv)
vec2 grid(vec2 uv, float s) {
    uv *= s;
    return uv - warpAmount * 0.5 * sin(uv);
}

// Fractional distance from cell edge: 0 at edges, 0.5 at centers
vec2 edge(vec2 x) {
    return abs(fract(x) - 0.5);
}

void main() {
    vec2 uv = fragTexCoord;
    float t = time * scrollSpeed;

    // Three grid layers at different scales
    vec2 a = grid(uv, scale1);
    vec2 b = grid(uv, scale2);
    vec2 c = grid(uv, scale3);

    // Cell coordinates with parallax scroll offsets (1.0, 0.7, 1.3)
    vec2 cellA = floor(a) / scale1 + t;
    vec2 cellB = floor(b) / scale2 + t * 0.7;
    vec2 cellC = floor(c) / scale3 + t * 1.3;

    // Combined edge pattern sharpened by edgePower
    vec2 edges = (edge(a) + edge(b) + edge(c)) / 1.5;
    edges = pow(edges, vec2(edgePower));

    // Weighted texture sampling, normalized to [0,1]
    vec3 tex = (0.8 * texture(texture0, cellA).rgb
              + 0.6 * texture(texture0, cellB).rgb
              + 0.4 * texture(texture0, cellC).rgb) / 1.8;

    // Darken at cell boundaries
    tex -= edgeContrast * (edges.x + edges.y);

    // Glow gate: cells below threshold go dark, squaring attenuates mid-range
    float glow = glowAmount * smoothstep(glowThreshold, glowThreshold + 0.45, tex.r);
    if (glowMode == 0) {
        tex *= glow * tex;
    } else {
        tex *= glow;
    }

    finalColor = vec4(tex, 1.0);
}
