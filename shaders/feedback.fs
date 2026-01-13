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

// Luminance gradient flow (content-based displacement)
uniform float feedbackFlowStrength;   // Displacement magnitude in pixels (0 = disabled)
uniform float feedbackFlowAngle;      // Direction relative to gradient in radians
uniform float feedbackFlowScale;      // Sampling distance for gradient (1-5 texels)
uniform float feedbackFlowThreshold;  // Minimum gradient magnitude to trigger flow

out vec4 finalColor;

float getLuminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

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

    // Luminance gradient flow: displace UV based on image edge direction
    if (feedbackFlowStrength > 0.0) {
        vec2 texelSize = 1.0 / resolution;
        vec2 offset = texelSize * feedbackFlowScale;

        float lumL = getLuminance(texture(texture0, uv - vec2(offset.x, 0.0)).rgb);
        float lumR = getLuminance(texture(texture0, uv + vec2(offset.x, 0.0)).rgb);
        float lumD = getLuminance(texture(texture0, uv - vec2(0.0, offset.y)).rgb);
        float lumU = getLuminance(texture(texture0, uv + vec2(0.0, offset.y)).rgb);

        // Gradient points toward brighter areas
        vec2 gradient = vec2(lumR - lumL, lumU - lumD);
        float gradMag = length(gradient);

        if (gradMag > feedbackFlowThreshold) {
            // Normalize and rotate by flowAngle
            vec2 gradNorm = gradient / gradMag;
            float c = cos(feedbackFlowAngle);
            float s = sin(feedbackFlowAngle);
            vec2 flowDir = vec2(
                gradNorm.x * c - gradNorm.y * s,
                gradNorm.x * s + gradNorm.y * c
            );

            // Displacement scales with gradient magnitude and strength
            uv += flowDir * gradMag * feedbackFlowStrength * texelSize;
        }
    }

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
