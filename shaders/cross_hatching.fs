#version 330

// Cross-Hatching: Hand-drawn NPR effect with temporal stutter and varied stroke angles
// Maps luminance to 4 angle-varied hatch layers with Sobel edge outlines

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform float width;
uniform float threshold;
uniform float noise;
uniform float outline;
uniform float blend;

out vec4 finalColor;

// Temporal frame for 12 FPS stutter
float frame;

// Per-pixel hash noise
float hash(float x)
{
    return fract(sin(x) * 43758.5453);
}

// Generate anti-aliased hatch line with varied angle and organic thickness
// slope: angle coefficient (coord.x + slope * coord.y)
// offset: phase shift for layer variation
// freq: pixel spacing between lines
// baseWidth: line thickness before modulation
// noiseAmt: per-pixel irregularity strength
float hatchLine(vec2 coord, float slope, float offset, float freq, float baseWidth, float noiseAmt)
{
    // Varied angle via slope coefficient on y
    float lineCoord = coord.x + slope * coord.y + offset;

    // Sin-modulated line width for organic thickness variation
    float widthMod = 0.5 * sin((coord.x + slope * coord.y) * 0.05) + 0.5;
    float w = baseWidth * (0.5 + widthMod);

    // Per-pixel noise displacement
    float noiseOffset = (hash(coord.x - 0.75 * coord.y + frame) - 0.5) * noiseAmt * 4.0;
    float d = mod(lineCoord + noiseOffset, freq);

    // Anti-aliased line
    return smoothstep(w, w * 0.5, d) + smoothstep(freq - w, freq - w * 0.5, d);
}

void main()
{
    // Quantize time to 12 FPS for hand-drawn stutter
    frame = floor(time * 12.0);

    vec2 uv = fragTexCoord;
    vec4 color = texture(texture0, uv);

    // Luminance extraction (Rec. 709)
    float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));

    // Scale coords to pixels, add temporal jitter
    vec2 coord = uv * resolution;
    coord += vec2(hash(frame * 1.23), hash(frame * 4.56)) * 2.0;

    // 4-layer hatching with varied angles (matching Shadertoy reference)
    // Layer 1: Light shading (slope 0.15, ~8.5 deg)
    float hatch1 = (luma < 0.8 * threshold) ? hatchLine(coord, 0.15, 0.0, 10.0, width, noise) : 0.0;

    // Layer 2: Medium shading (slope 1.0, 45 deg)
    float hatch2 = (luma < 0.6 * threshold) ? hatchLine(coord, 1.0, 0.0, 14.0, width, noise) : 0.0;

    // Layer 3: Dark shading (slope -0.75, ~-37 deg)
    float hatch3 = (luma < 0.3 * threshold) ? hatchLine(coord, -0.75, 0.0, 8.0, width, noise) : 0.0;

    // Layer 4: Deep shadow (slope 0.15, offset 4.0)
    float hatch4 = (luma < 0.15 * threshold) ? hatchLine(coord, 0.15, 4.0, 7.0, width, noise) : 0.0;

    // Sobel edge detection (3x3 kernel)
    vec2 texel = 1.0 / resolution;
    vec4 n[9];
    n[0] = texture(texture0, uv + vec2(-texel.x, -texel.y));
    n[1] = texture(texture0, uv + vec2(    0.0, -texel.y));
    n[2] = texture(texture0, uv + vec2( texel.x, -texel.y));
    n[3] = texture(texture0, uv + vec2(-texel.x,     0.0));
    n[4] = texture(texture0, uv);
    n[5] = texture(texture0, uv + vec2( texel.x,     0.0));
    n[6] = texture(texture0, uv + vec2(-texel.x,  texel.y));
    n[7] = texture(texture0, uv + vec2(    0.0,  texel.y));
    n[8] = texture(texture0, uv + vec2( texel.x,  texel.y));

    vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);
    float edge = length(sqrt(sobelH * sobelH + sobelV * sobelV).rgb);
    float outlineMask = smoothstep(0.1, 0.3, edge) * outline;

    // Combine hatching layers and outline
    float ink = max(max(hatch1, hatch2), max(hatch3, hatch4));
    ink = max(ink, outlineMask);

    // Color blend compositing: original color -> black ink
    vec3 inkColor = mix(color.rgb, vec3(0.0), ink);
    vec3 result = mix(color.rgb, inkColor, blend);

    finalColor = vec4(result, color.a);
}
