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
uniform int glowMode;           // 0=squared glow, 1=linear glow
uniform float cellVariation;    // Per-cell brightness spread centered on 1.0
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

// Per-cell hash: unique value per grid cell for brightness variation
float cellHash(vec2 cell) {
    return fract(sin(dot(cell, vec2(12.9898, 8.233))) * 43758.5453);
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

    // Per-cell brightness: centered on 1.0, some cells brighter, some dimmer
    float hA = 1.0 + cellVariation * (2.0 * cellHash(floor(a)) - 1.0);
    float hB = 1.0 + cellVariation * (2.0 * cellHash(floor(b)) - 1.0);
    float hC = 1.0 + cellVariation * (2.0 * cellHash(floor(c)) - 1.0);

    // Weighted texture sampling, normalized to [0,1], with per-cell variation
    vec3 tex = (0.8 * hA * texture(texture0, cellA).rgb
              + 0.6 * hB * texture(texture0, cellB).rgb
              + 0.4 * hC * texture(texture0, cellC).rgb) / 1.8;

    // Darken at cell boundaries
    tex -= edgeContrast * (edges.x + edges.y);

    // Glow gate: cells below threshold go dark
    float gate = smoothstep(glowThreshold, glowThreshold + 0.45, tex.r);
    if (glowMode == 0) {
        tex = gate * tex * tex; // squared: high contrast, darker mid-tones
    } else {
        tex *= gate;            // linear: preserves relative brightness
    }
    // Gamma lift: glowAmount > 1 brightens without clipping past 1.0
    tex = pow(tex, vec3(1.0 / glowAmount));

    finalColor = vec4(tex, 1.0);
}
