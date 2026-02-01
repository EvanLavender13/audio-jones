#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;

// Horizon and color
uniform float horizonY;
uniform float colorMix;
uniform vec3 palettePhase;

// Grid
uniform float gridSpacing;
uniform float gridThickness;
uniform float gridOpacity;
uniform float gridGlow;
uniform vec3 gridColor;

// Stripes
uniform float stripeCount;
uniform float stripeSoftness;
uniform float stripeIntensity;
uniform vec3 sunColor;

// Horizon glow
uniform float horizonIntensity;
uniform float horizonFalloff;
uniform vec3 horizonColor;

// Animation (pre-accumulated with speed on CPU)
uniform float gridTime;
uniform float stripeTime;

// Cosine palette: phase controls color mapping
vec3 synthwavePalette(float t, vec3 phase) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    return a + b * cos(6.283185 * (c * t + phase));
}

// Perspective grid with vanishing point at horizon (below horizon = ground)
float perspectiveGrid(vec2 uv) {
    float depth = (horizonY - uv.y) / horizonY;
    if (depth <= 0.0) return 0.0;

    float z = 1.0 / depth;
    float x = (uv.x - 0.5) * z;

    // Scroll grid toward viewer
    vec2 gridUV = vec2(x, z + gridTime) * gridSpacing;
    vec2 fw = fwidth(gridUV);
    vec2 grid = smoothstep(gridThickness + fw, gridThickness - fw,
                           abs(fract(gridUV) - 0.5));

    return max(grid.x, grid.y);
}

// Horizontal sun stripes in sky region (above horizon)
// Classic synthwave sun: stripes thick at bottom, thin at top
float sunStripes(vec2 uv) {
    if (uv.y < horizonY) return 0.0;

    float skyPos = (uv.y - horizonY) / (1.0 - horizonY);

    // Sin wave + position-dependent offset creates varying stripe thickness
    // Stripes are thick near horizon, thin near top
    float cut = 5.0 * sin((skyPos + stripeTime) * stripeCount * 6.283185)
              + clamp((skyPos - 0.5) * 10.0, -6.0, 6.0);
    cut = clamp(cut, 0.0, 1.0);

    // Blend with softness
    float stripe = smoothstep(stripeSoftness, 1.0 - stripeSoftness, cut);

    // Fade at edges
    float fade = smoothstep(0.0, 0.2, skyPos) * smoothstep(1.0, 0.8, skyPos);

    return stripe * fade;
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 inputColor = texture(texture0, uv);

    // Luminance for masking and palette lookup
    float luma = dot(inputColor.rgb, vec3(0.299, 0.587, 0.114));

    // Cosine palette color remap
    vec3 remapped = synthwavePalette(luma, palettePhase);
    vec3 result = mix(inputColor.rgb, remapped, colorMix);

    // Grid on dark areas
    float gridMask = perspectiveGrid(uv);
    float darknessMask = 1.0 - luma;
    gridMask *= darknessMask * gridOpacity;
    result += gridColor * gridMask * gridGlow;

    // Sun stripes on bright areas
    float stripeMask = sunStripes(uv);
    stripeMask *= luma * stripeIntensity;
    result = mix(result, sunColor, stripeMask);

    // Horizon glow
    float horizonGlow = exp(-abs(uv.y - horizonY) * horizonFalloff);
    result += horizonColor * horizonGlow * horizonIntensity;

    finalColor = vec4(result, inputColor.a);
}
