#version 330

// Cross-Hatching: NPR procedural diagonal strokes with 4 density layers
// Maps luminance to hatch patterns with Sobel edge outlines

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float density;
uniform float width;
uniform float threshold;
uniform float jitter;
uniform float outline;
uniform float blend;

out vec4 finalColor;

// Generate anti-aliased hatch line
// dir: 1.0 for diagonal /, -1.0 for diagonal \
// offset: phase shift for secondary layers
float hatchLine(vec2 uv, float dir, float offset)
{
    float coord = uv.x * resolution.x + dir * uv.y * resolution.y + offset;
    float d = mod(coord, density);
    return smoothstep(width, width * 0.5, d) + smoothstep(density - width, density - width * 0.5, d);
}

void main()
{
    vec2 uv = fragTexCoord;

    // Optional sketchy jitter UV perturbation
    vec2 jitteredUV = uv + vec2(
        sin(uv.y * 50.0) * jitter * 0.002,
        cos(uv.x * 50.0) * jitter * 0.002
    );

    vec4 color = texture(texture0, uv);

    // Luminance extraction (Rec. 709)
    float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));

    // 4-layer hatching with fixed thresholds
    // Layer 1: Light shading (diagonal /)
    float hatch1 = (luma < 0.8 * threshold) ? hatchLine(jitteredUV, 1.0, 0.0) : 0.0;

    // Layer 2: Medium shading (diagonal \)
    float hatch2 = (luma < 0.6 * threshold) ? hatchLine(jitteredUV, -1.0, 0.0) : 0.0;

    // Layer 3: Dark shading (offset /)
    float hatch3 = (luma < 0.3 * threshold) ? hatchLine(jitteredUV, 1.0, density * 0.5) : 0.0;

    // Layer 4: Deep shadow (offset \)
    float hatch4 = (luma < 0.15 * threshold) ? hatchLine(jitteredUV, -1.0, density * 0.5) : 0.0;

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
