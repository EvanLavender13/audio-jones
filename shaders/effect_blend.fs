#version 330

// Generic effect blend post-process
// Composites simulation textures (physarum, curl flow, attractor flow) onto accumulation buffer
// Supports multiple Photoshop-style blend modes

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;   // Main accumulation texture
uniform sampler2D effectMap;  // Effect RGBA32F texture (trails, etc.)
uniform float intensity;      // User-controlled 0.0-5.0
uniform int blendMode;        // See EffectBlendMode enum

out vec4 finalColor;

// Standard luminance weights (Rec. 601)
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

// Helper functions for compound blend modes
float colorDodge(float base, float blend) {
    return (blend >= 1.0) ? 1.0 : min(1.0, base / (1.0 - blend));
}

float colorBurn(float base, float blend) {
    return (blend <= 0.0) ? 0.0 : max(0.0, 1.0 - (1.0 - base) / blend);
}

vec3 colorDodgeV(vec3 base, vec3 blend) {
    return vec3(colorDodge(base.r, blend.r), colorDodge(base.g, blend.g), colorDodge(base.b, blend.b));
}

vec3 colorBurnV(vec3 base, vec3 blend) {
    return vec3(colorBurn(base.r, blend.r), colorBurn(base.g, blend.g), colorBurn(base.b, blend.b));
}

void main()
{
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 effectColor = texture(effectMap, fragTexCoord).rgb;
    float luminance = dot(effectColor, LUMA_WEIGHTS);

    // Headroom for boost modes - reduce effect on already-bright pixels
    float maxChan = max(original.r, max(original.g, original.b));
    float headroom = max(0.0, 1.0 - maxChan);

    // Scaled blend color clamped to valid range
    vec3 b = clamp(effectColor * intensity, vec3(0.0), vec3(1.0));

    vec3 blended;

    if (blendMode == 0) {
        // Boost: luminance-based brightness multiplication
        blended = original * (1.0 + luminance * intensity * headroom);
    }
    else if (blendMode == 1) {
        // Tinted Boost: brightness boost tinted by effect color
        vec3 tintedBoost = effectColor * luminance * intensity * headroom;
        blended = original * (1.0 + tintedBoost);
    }
    else if (blendMode == 2) {
        // Screen: additive-like blend
        vec3 scaledEffect = effectColor * intensity;
        blended = 1.0 - (1.0 - original) * (1.0 - scaledEffect);
    }
    else if (blendMode == 3) {
        // Mix: linear interpolation based on effect luminance
        blended = mix(original, effectColor, clamp(luminance * intensity, 0.0, 1.0));
    }
    else if (blendMode == 4) {
        // Soft Light (Pegtop formula)
        blended = (1.0 - 2.0 * b) * original * original + 2.0 * b * original;
    }
    else if (blendMode == 5) {
        // Overlay: contrast boost (darken darks, brighten brights)
        blended = mix(
            2.0 * original * b,
            1.0 - 2.0 * (1.0 - original) * (1.0 - b),
            step(0.5, original)
        );
    }
    else if (blendMode == 6) {
        // Color Burn: darken + saturate
        blended = colorBurnV(original, b);
    }
    else if (blendMode == 7) {
        // Linear Burn: subtractive darkening
        blended = max(vec3(0.0), original + b - 1.0);
    }
    else if (blendMode == 8) {
        // Vivid Light: extreme contrast (burn/dodge hybrid)
        blended = mix(
            colorBurnV(original, 2.0 * b),
            colorDodgeV(original, 2.0 * b - 1.0),
            step(0.5, b)
        );
    }
    else if (blendMode == 9) {
        // Linear Light: linear burn/dodge hybrid
        blended = mix(
            max(vec3(0.0), original + 2.0 * b - 1.0),
            min(vec3(1.0), original + 2.0 * b - 1.0),
            step(0.5, b)
        );
    }
    else if (blendMode == 10) {
        // Pin Light: replace shadows/highlights selectively
        blended = mix(
            min(original, 2.0 * b),
            max(original, 2.0 * b - 1.0),
            step(0.5, b)
        );
    }
    else if (blendMode == 11) {
        // Difference: inversion where effect exists
        blended = abs(original - b);
    }
    else if (blendMode == 12) {
        // Negation: softer difference
        blended = 1.0 - abs(1.0 - original - b);
    }
    else if (blendMode == 13) {
        // Subtract: darken by removal
        blended = max(vec3(0.0), original - b);
    }
    else if (blendMode == 14) {
        // Reflect: specular-like glow
        blended = min(vec3(1.0), original * original / max(1.0 - b, vec3(0.001)));
    }
    else {
        // Phoenix: unique color shifts
        blended = min(original, b) - max(original, b) + 1.0;
    }

    finalColor = vec4(blended, 1.0);
}
