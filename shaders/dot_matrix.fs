#version 330

// Dot matrix: inverse-cube glow dots on a rotated grid, composited on black

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float dotScale;    // Grid cell size in pixels
uniform float softness;    // Glow falloff control
uniform float brightness;  // Output intensity multiplier
uniform float rotation;    // Grid rotation angle

out vec4 finalColor;

mat2 rotm(float r) {
    float c = cos(r), s = sin(r);
    return mat2(c, -s, s, c);
}

void main()
{
    vec2 ratio = vec2(1.0, resolution.x / resolution.y);
    vec2 fc = fragTexCoord * resolution;
    mat2 m = rotm(rotation);

    // Scale UV by dotScale with aspect-ratio correction, rotate, snap to grid
    vec2 scaled = fc * ratio / dotScale;
    vec2 rotated = m * scaled;
    vec2 cell = floor(rotated) + 0.5;

    // Unrotate cell center back to screen space
    vec2 center = transpose(m) * cell;
    vec2 texUV = center * dotScale / ratio / resolution;

    // Sample color at cell center
    vec3 texColor = texture(texture0, texUV).rgb;
    float luma = dot(texColor, vec3(0.299, 0.587, 0.114));

    // Cell-local coords remapped to [-1, 1]
    vec2 localUV = (rotated - floor(rotated)) * 2.0 - 1.0;
    float dist = length(localUV);

    // Inverse-cube glow kernel
    float glow = softness / pow(1.0 + softness * dist, 3.0);
    glow = clamp(glow, 0.0, 1.0);

    // Composite on black background
    vec3 result = texColor * luma * glow * brightness;

    finalColor = vec4(result, 1.0);
}
