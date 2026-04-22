// Based on "Escher's Droste warp" by perlinw
// https://www.shadertoy.com/view/sfBSWm
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized spiralStrength, rotationOffset, innerRadius, CPU-accumulated zoom phase, configurable center

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 center;           // Pixel-space drift from screen center (0,0 = no drift)
uniform float scale;           // Tile scale factor k
uniform float zoomPhase;       // CPU-accumulated phase (replaces iTime * 0.5)
uniform float spiralStrength;  // 0 = zoom-only tiling, 1 = Escher spiral
uniform float rotationOffset;  // Static angle added to warpV (radians)
uniform float innerRadius;     // Fade mask radius (0 disables)

const float TWO_PI = 6.28318530718;

void main() {
    vec2 uv = (fragTexCoord * resolution - 0.5 * resolution - center) / resolution.y;

    float L = log(scale);

    // log-polar space transform
    float r = length(uv);
    float a = atan(uv.y, uv.x);
    float u = log(r);
    float v = a;

    // Escher warp with parameterized strength
    // c = 0: warpU/V collapse to u/v (pure zoom-only tiling)
    // c = L/TWO_PI: reference Escher spiral
    float c = (L / TWO_PI) * spiralStrength;
    float warpU = u + v * c;
    float warpV = v - u * c;

    // continuous zoom along log-radius (signed)
    warpU -= zoomPhase;

    // static rotation of tiled pattern
    warpV += rotationOffset;

    // map back to tile UV space and tile
    vec2 texCoord = vec2(warpU / L, warpV / TWO_PI);

    // seam-corrected derivatives to hide atan2 branch cut at deep zoom
    vec2 dx = dFdx(texCoord);
    vec2 dy = dFdy(texCoord);
    dx -= floor(dx + 0.5);
    dy -= floor(dy + 0.5);

    // fract() wraps the sample; input texture has no GL_REPEAT.
    vec3 col = textureGrad(texture0, fract(texCoord), dx, dy).rgb;

    // fade the singular center where log-polar math diverges
    float fade = (innerRadius > 0.0) ? smoothstep(0.0, innerRadius, r) : 1.0;

    finalColor = vec4(col * fade, 1.0);
}
