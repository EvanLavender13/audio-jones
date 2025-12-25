#version 330

// Physarum trail blend post-process effect
// Supports multiple blend modes with soft saturation safety net

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;   // Main accumulation texture
uniform sampler2D trailMap;   // Physarum RGBA32F trail color
uniform float boostIntensity; // User-controlled 0.0-5.0
uniform int blendMode;        // 0=Boost, 1=TintedBoost, 2=Screen, 3=Mix, 4=SoftLight

out vec4 finalColor;

// Standard luminance weights (Rec. 601)
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

void main()
{
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 trailColor = texture(trailMap, fragTexCoord).rgb;
    float luminance = dot(trailColor, LUMA_WEIGHTS);

    // Headroom for boost modes - reduce effect on already-bright pixels
    float maxChan = max(original.r, max(original.g, original.b));
    float headroom = max(0.0, 1.0 - maxChan);

    vec3 blended;

    if (blendMode == 0) {
        // Boost: luminance-based brightness multiplication
        blended = original * (1.0 + luminance * boostIntensity * headroom);
    }
    else if (blendMode == 1) {
        // Tinted Boost: brightness boost tinted by trail color
        vec3 tintedBoost = trailColor * luminance * boostIntensity * headroom;
        blended = original * (1.0 + tintedBoost);
    }
    else if (blendMode == 2) {
        // Screen: additive-like blend that never exceeds 1.0
        vec3 scaledTrail = min(trailColor * boostIntensity, vec3(1.0));
        blended = 1.0 - (1.0 - original) * (1.0 - scaledTrail);
    }
    else if (blendMode == 3) {
        // Mix: linear interpolation based on trail luminance
        blended = mix(original, trailColor, luminance * boostIntensity);
    }
    else {
        // Soft Light (Pegtop formula): clamp b to valid range
        vec3 b = min(trailColor * boostIntensity, vec3(0.5));
        blended = (1.0 - 2.0 * b) * original * original + 2.0 * b * original;
    }

    // Soft saturation safety net - Reinhard tone mapping
    vec3 result = blended / (blended + vec3(1.0));
    result *= 2.0;  // Rescale since Reinhard caps at 0.5

    finalColor = vec4(result, 1.0);
}
