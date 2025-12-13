#version 330

// Feedback pass: sample previous frame with zoom + rotation
// Creates MilkDrop-style infinite tunnel effect

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float zoom;        // 0.9-1.0, lower = faster inward motion
uniform float rotation;    // radians per frame
uniform float desaturate;  // 0.0-1.0, higher = faster fade to gray

out vec4 finalColor;

void main()
{

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

    // Fade trails toward luminance-matched dark gray
    float luma = dot(finalColor.rgb, vec3(0.299, 0.587, 0.114));
    finalColor.rgb = mix(finalColor.rgb, vec3(luma * 0.3), desaturate);
}
