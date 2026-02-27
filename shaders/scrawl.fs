// Based on "Genuary 2025 !1" by Kali
// https://www.shadertoy.com/view/4XVczz
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT coloring replaces procedural color rotation, all
// constants parameterized as uniforms, smooth rotation accumulator replaces
// discrete snaps, per-cell tiling rotation, scroll/evolve/warpPhase
// accumulators replace iTime-based animation.

// Scrawl: IFS fractal fold with thick marker strokes and gradient LUT coloring.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform int iterations;
uniform float foldOffset;
uniform float tileScale;
uniform float zoom;
uniform float warpFreq;
uniform float warpAmp;
uniform float thickness;
uniform float glowIntensity;
uniform float scrollAccum;
uniform float evolveAccum;
uniform float rotationAccum;
uniform float warpPhaseAccum;
uniform int mode;
uniform sampler2D gradientLUT;

// Fractal computation for a single sample point
vec3 fractal(vec2 uv) {
    // Smooth rotation (replaces reference's discrete 45-degree snaps)
    float cr = cos(rotationAccum), sr = sin(rotationAccum);
    uv *= mat2(cr, -sr, sr, cr);

    // Save pre-fold position for drawing mask
    vec2 p2 = uv;

    // Breathing zoom (reference: p *= 0.3 + asin(0.9*sin(iTime*0.5))*0.2)
    uv *= zoom + asin(0.9 * sin(evolveAccum)) * 0.2;

    // Horizontal scroll
    uv.x += scrollAccum;

    // Tile into [0,1] with per-cell rotation
    vec2 cell = floor(uv * tileScale);
    uv = fract(uv * tileScale);
    float cellAngle = (cell.x * 3.17 + cell.y * 7.23); // pseudo-random per cell
    float cc = cos(cellAngle), sc_r = sin(cellAngle);
    uv = (uv - 0.5) * mat2(cc, -sc_r, sc_r, cc) + 0.5;

    // IFS fold loop
    float m = 1000.0;
    int winIt = 0;

    for (int i = 0; i < iterations; i++) {
        switch (mode) {
        case 0: // IFS Fold (default)
            uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
            break;
        case 1: // Kali Set
            uv = abs(uv) / dot(uv, uv) - foldOffset;
            break;
        case 2: // Burning Ship
            uv = vec2(uv.x * uv.x - uv.y * uv.y, 2.0 * abs(uv.x * uv.y)) - foldOffset;
            break;
        case 3: // Menger Fold
            uv = abs(uv) - foldOffset; if (uv.x < uv.y) uv = uv.yx;
            break;
        case 4: // Box Fold
            uv = clamp(uv, -foldOffset, foldOffset) * 2.0 - uv;
            uv /= clamp(dot(uv, uv), 0.25, 1.0);
            break;
        case 5: { // Spiral IFS
            uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
            float ca = 0.714, sa = 0.700;
            uv *= mat2(ca, -sa, sa, ca);
            break;
        }
        case 6: // Power Fold
            uv = pow(abs(uv), vec2(1.5)) * sign(uv) - foldOffset;
            break;
        }

        float l = abs(uv.x + asin(0.9 * sin(length(uv) * warpFreq + warpPhaseAccum)) * warpAmp);
        if (l < m) {
            m = l;
            winIt = i;
        }
    }

    // Stroke rendering (reference: smoothstep(.015, .01, m*.5))
    float stroke = smoothstep(thickness * 1.5, thickness, m * 0.5);

    // Drawing mask — progressive reveal creates growing-line effect
    stroke *= step(p2.x + float(winIt) * 0.1 - 0.8 + sin(uv.y * 2.0) * 0.1, 0.0);

    // Color from gradient LUT by iteration depth
    vec3 layerColor = texture(gradientLUT, vec2((float(winIt) + 0.5) / float(iterations), 0.5)).rgb;

    vec3 color = stroke * glowIntensity * layerColor;

    // Scanline stripe background (reference: step(.5,fract(p2.y*50.))*.5)
    color += step(0.5, fract(p2.y * 50.0)) * 0.5 * layerColor;

    return color;
}

void main() {
    // UV: centered, aspect-corrected
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y;

    // Distance-scaled AA supersampling (reference: 7x7 kernel, wider farther from center)
    const int aa = 3;
    float f = max(abs(uv.x), abs(uv.y));
    vec2 pixelSize = 0.01 / normalize(resolution) / float(aa) * f;

    vec3 col = vec3(0.0);
    for (int i = -aa; i <= aa; i++) {
        for (int j = -aa; j <= aa; j++) {
            vec2 offset = vec2(float(i), float(j)) * pixelSize;
            col += fractal(uv + offset);
        }
    }

    float totalSamples = float((aa * 2 + 1) * (aa * 2 + 1));
    col /= totalSamples;

    finalColor = vec4(col, 1.0);
}
