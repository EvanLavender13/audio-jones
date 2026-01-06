#version 330

// Generic effect blend post-process
// Composites simulation textures (physarum, MNCA) onto accumulation buffer
// Supports multiple blend modes with soft saturation safety net

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;   // Main accumulation texture
uniform sampler2D effectMap;  // Effect RGBA32F texture (trails, MNCA state, etc.)
uniform float intensity;      // User-controlled 0.0-5.0
uniform int blendMode;        // 0=Boost, 1=TintedBoost, 2=Screen, 3=Mix, 4=SoftLight

out vec4 finalColor;

// Standard luminance weights (Rec. 601)
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

void main()
{
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 effectColor = texture(effectMap, fragTexCoord).rgb;
    float luminance = dot(effectColor, LUMA_WEIGHTS);

    // Headroom for boost modes - reduce effect on already-bright pixels
    float maxChan = max(original.r, max(original.g, original.b));
    float headroom = max(0.0, 1.0 - maxChan);

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
    else {
        // Soft Light (Pegtop formula)
        vec3 b = clamp(effectColor * intensity, vec3(0.0), vec3(1.0));
        blended = (1.0 - 2.0 * b) * original * original + 2.0 * b * original;
    }

    finalColor = vec4(blended, 1.0);
}
