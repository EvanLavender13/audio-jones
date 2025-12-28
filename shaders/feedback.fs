#version 330

// Feedback pass: spatial flow field with position-dependent UV transforms
// Creates MilkDrop-style infinite tunnel effects with radially-varying motion

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float desaturate;  // 0.0-1.0, higher = faster fade to gray

// Spatial flow field parameters
uniform float zoomBase;     // Base zoom factor (0.98-1.02)
uniform float zoomRadial;   // Radial zoom coefficient
uniform float rotBase;      // Base rotation in radians
uniform float rotRadial;    // Radial rotation coefficient
uniform float dxBase;       // Base horizontal translation
uniform float dxRadial;     // Radial horizontal coefficient
uniform float dyBase;       // Base vertical translation
uniform float dyRadial;     // Radial vertical coefficient

out vec4 finalColor;

void main()
{
    vec2 center = vec2(0.5);
    vec2 uv = fragTexCoord - center;

    // Compute aspect-corrected radius (rad=1 is a circle touching shorter edge)
    float aspect = resolution.x / resolution.y;
    vec2 normalized = uv;
    if (aspect > 1.0) {
        normalized.x /= aspect;
    } else {
        normalized.y *= aspect;
    }
    float rad = length(normalized) * 2.0;

    // Compute spatially-varying parameters
    float zoom = zoomBase + rad * zoomRadial;
    float rot = rotBase + rad * rotRadial;
    float dx = dxBase + rad * dxRadial;
    float dy = dyBase + rad * dyRadial;

    // Apply transforms in MilkDrop order: zoom -> rotate -> translate
    uv *= zoom;

    float cosR = cos(rot);
    float sinR = sin(rot);
    uv = vec2(uv.x * cosR - uv.y * sinR, uv.x * sinR + uv.y * cosR);

    uv += vec2(dx, dy);
    uv += center;

    // Mirror UVs at boundaries instead of clamping - eliminates edge discontinuities
    // that cause trailing artifacts when zooming/rotating
    vec2 mirroredUV = uv;
    mirroredUV = abs(mirroredUV);                        // Handle negative coords
    mirroredUV = mod(mirroredUV, 2.0);                   // Wrap to 0-2 range
    mirroredUV = 1.0 - abs(mirroredUV - 1.0);           // Mirror at 1.0 boundary

    finalColor = texture(texture0, mirroredUV);

    // Fade trails toward luminance-matched dark gray
    float luma = dot(finalColor.rgb, vec3(0.299, 0.587, 0.114));
    finalColor.rgb = mix(finalColor.rgb, vec3(luma * 0.3), desaturate);
}
