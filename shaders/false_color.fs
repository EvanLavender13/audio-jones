#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;  // Input image
uniform sampler2D texture1;  // 1D Gradient LUT (256x1) - TODO Phase 2
uniform float intensity;     // Blend: 0 = original, 1 = full false color

const vec3 LUMINANCE_WEIGHTS = vec3(0.299, 0.587, 0.114);

void main()
{
    vec4 color = texture(texture0, fragTexCoord);

    // Extract luminance
    float luma = dot(color.rgb, LUMINANCE_WEIGHTS);

    // TODO Phase 2: Sample gradient LUT
    // For now, just output grayscale as placeholder
    vec3 mappedColor = vec3(luma);

    // Blend with original
    vec3 result = mix(color.rgb, mappedColor, intensity);

    finalColor = vec4(result, color.a);
}
