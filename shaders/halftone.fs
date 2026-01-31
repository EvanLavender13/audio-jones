#version 330

// Halftone: luminance-based dot pattern simulating print halftoning

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float dotScale;    // Grid cell size in pixels
uniform float dotSize;     // Dot radius multiplier within cell
uniform float rotation;    // Grid rotation angle

out vec4 finalColor;

mat2 rotm(float r) {
    float c = cos(r), s = sin(r);
    return mat2(c, -s, s, c);
}

void main()
{
    vec2 fc = fragTexCoord * resolution;
    mat2 m = rotm(rotation);

    // Rotate, snap to grid, find cell center
    vec2 rotated = m * fc;
    vec2 cell = floor(rotated / dotScale) * dotScale + dotScale * 0.5;
    vec2 center = transpose(m) * cell;  // Unrotate back

    // Sample color at cell center
    vec3 texColor = texture(texture0, center / resolution).rgb;
    float luma = dot(texColor, vec3(0.299, 0.587, 0.114));

    // Dot radius scales with darkness (dark = big dots)
    float radius = dotScale * dotSize * (1.0 - luma) * 0.5;

    // Distance from pixel to dot center
    float dist = length(fc - center);

    // Smooth dot edge
    float edge = dotScale * 0.05;
    float inDot = 1.0 - smoothstep(radius - edge, radius + edge, dist);

    // Blend: show texture color where dot is, white where paper shows
    vec3 result = mix(vec3(1.0), texColor, inDot);

    finalColor = vec4(result, 1.0);
}
