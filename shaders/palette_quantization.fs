#version 330

// Palette Quantization: Reduces image colors to limited palette with ordered Bayer dithering
// Creates retro/pixel-art aesthetics from 8-color to 4096-color palettes

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float colorLevels;    // Quantization levels per channel (2-16)
uniform float ditherStrength; // Dithering intensity (0-1)
uniform int bayerSize;        // Dither matrix size (4 or 8)

out vec4 finalColor;

// 8x8 Bayer matrix (finer pattern)
const float BAYER_8X8[64] = float[64](
     0.0, 32.0,  8.0, 40.0,  2.0, 34.0, 10.0, 42.0,
    48.0, 16.0, 56.0, 24.0, 50.0, 18.0, 58.0, 26.0,
    12.0, 44.0,  4.0, 36.0, 14.0, 46.0,  6.0, 38.0,
    60.0, 28.0, 52.0, 20.0, 62.0, 30.0, 54.0, 22.0,
     3.0, 35.0, 11.0, 43.0,  1.0, 33.0,  9.0, 41.0,
    51.0, 19.0, 59.0, 27.0, 49.0, 17.0, 57.0, 25.0,
    15.0, 47.0,  7.0, 39.0, 13.0, 45.0,  5.0, 37.0,
    63.0, 31.0, 55.0, 23.0, 61.0, 29.0, 53.0, 21.0
);

// 4x4 Bayer matrix (coarser pattern)
const float BAYER_4X4[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

float getBayerThreshold8(vec2 fragCoord) {
    int x = int(mod(fragCoord.x, 8.0));
    int y = int(mod(fragCoord.y, 8.0));
    return (BAYER_8X8[x + y * 8] + 0.5) / 64.0;
}

float getBayerThreshold4(vec2 fragCoord) {
    int x = int(mod(fragCoord.x, 4.0));
    int y = int(mod(fragCoord.y, 4.0));
    return (BAYER_4X4[x + y * 4] + 0.5) / 16.0;
}

// Snap value to nearest quantization level
float quantize(float value, float levels) {
    return floor(value * levels + 0.5) / levels;
}

// Dithered quantization using Bayer threshold
vec3 ditherQuantize(vec3 color, vec2 fragCoord, float levels, float strength) {
    float threshold = (bayerSize == 4) ? getBayerThreshold4(fragCoord) : getBayerThreshold8(fragCoord);
    float stepSize = 1.0 / levels;

    // Offset color by threshold before quantizing
    vec3 dithered = color + (threshold - 0.5) * stepSize * strength;
    return vec3(
        quantize(dithered.r, levels),
        quantize(dithered.g, levels),
        quantize(dithered.b, levels)
    );
}

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    vec2 fragCoord = gl_FragCoord.xy;

    vec3 quantized = ditherQuantize(texColor.rgb, fragCoord, colorLevels, ditherStrength);

    finalColor = vec4(quantized, texColor.a);
}
