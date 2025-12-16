#version 330

// Physarum trail boost post-process effect
// Combined: headroom-limited boost + soft saturation safety net

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;   // Main accumulation texture
uniform sampler2D trailMap;   // Physarum R32F trail intensity
uniform float boostIntensity;  // User-controlled 0.0-2.0

out vec4 finalColor;

void main()
{
    vec3 original = texture(texture0, fragTexCoord).rgb;
    float trail = texture(trailMap, fragTexCoord).r;

    // Headroom-limited boost - reduce boost on already-bright pixels
    float maxChan = max(original.r, max(original.g, original.b));
    float headroom = max(0.0, 1.0 - maxChan);  // Clamp for HDR input from voronoi
    vec3 boosted = original * (1.0 + trail * boostIntensity * headroom);

    // Soft saturation safety net - smooth rolloff for any remaining hot spots
    vec3 result = boosted / (boosted + vec3(1.0));
    result *= 2.0;  // Rescale since Reinhard caps at 0.5

    finalColor = vec4(result, 1.0);
}
