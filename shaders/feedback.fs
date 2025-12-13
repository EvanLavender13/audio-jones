#version 330

// Feedback pass: sample previous frame with zoom + rotation
// Creates MilkDrop-style infinite tunnel effect

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

void main()
{
    const float zoom = 0.98;      // 2% inward motion per frame
    const float rotation = 0.005; // ~0.3 degrees, full rotation in ~20s at 60fps

    vec2 center = vec2(0.5);
    vec2 uv = fragTexCoord - center;

    // Rotate around center
    float s = sin(rotation);
    float c = cos(rotation);
    uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

    // Scale toward center (zoom < 1.0 = zoom in)
    uv *= zoom;

    uv += center;

    // Sample with clamping to avoid edge artifacts
    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
