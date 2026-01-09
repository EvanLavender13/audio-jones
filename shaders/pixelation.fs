#version 330

// Pixelation with Bayer dithering and color posterization
// UV quantization snaps fragments to cell centers for mosaic effect

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float cellCount;
uniform float ditherScale;
uniform int posterizeLevels;

out vec4 finalColor;

// Bayer 8x8 ordered dither matrix (values 0-63)
const float bayer8x8[64] = float[64](
     0.0, 32.0,  8.0, 40.0,  2.0, 34.0, 10.0, 42.0,
    48.0, 16.0, 56.0, 24.0, 50.0, 18.0, 58.0, 26.0,
    12.0, 44.0,  4.0, 36.0, 14.0, 46.0,  6.0, 38.0,
    60.0, 28.0, 52.0, 20.0, 62.0, 30.0, 54.0, 22.0,
     3.0, 35.0, 11.0, 43.0,  1.0, 33.0,  9.0, 41.0,
    51.0, 19.0, 59.0, 27.0, 49.0, 17.0, 57.0, 25.0,
    15.0, 47.0,  7.0, 39.0, 13.0, 45.0,  5.0, 37.0,
    63.0, 31.0, 55.0, 23.0, 61.0, 29.0, 53.0, 21.0
);

void main()
{
    // Aspect-corrected cell count
    float cellsX = cellCount;
    float cellsY = cellCount * resolution.y / resolution.x;

    // Quantize UV to cell centers
    vec2 pixelatedUV = (floor(fragTexCoord * vec2(cellsX, cellsY)) + 0.5) / vec2(cellsX, cellsY);
    vec4 color = texture(texture0, pixelatedUV);

    // Compute dither threshold (0.5 = neutral when dithering disabled)
    float threshold = 0.5;
    if (ditherScale > 0.0) {
        vec2 ditherPos = mod(fragTexCoord * resolution / ditherScale, 8.0);
        int idx = int(ditherPos.y) * 8 + int(ditherPos.x);
        threshold = (bayer8x8[idx] + 0.5) / 64.0;
    }

    // Apply posterization with dither bias
    if (posterizeLevels > 0) {
        float levels = float(posterizeLevels);
        float ditherBias = (threshold - 0.5) / levels;
        color.rgb = floor((color.rgb + ditherBias) * levels + 0.5) / levels;
    }

    finalColor = color;
}
