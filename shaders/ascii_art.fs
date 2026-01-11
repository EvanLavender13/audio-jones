#version 330

// ASCII Art: Luminance-based character rendering
// Divides screen into cells, samples center, renders bit-packed character glyphs

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int cellPixels;
uniform int colorMode;      // 0 = original, 1 = mono, 2 = CRT green
uniform vec3 foreground;    // Mono mode foreground color
uniform vec3 background;    // Mono mode background color
uniform int invert;

out vec4 finalColor;

// Bit-packed 5x5 character bitmaps (25 bits each, row-major indexing with stride 5)
// Characters ordered by density: # @ 8 & o * : . (space)
int getCharBits(float lum) {
    if (lum > 0.85) return 11512810;  // #
    if (lum > 0.75) return 13195790;  // @
    if (lum > 0.65) return 15252014;  // 8
    if (lum > 0.55) return 13121101;  // &
    if (lum > 0.45) return 15255086;  // o
    if (lum > 0.35) return 163153;    // *
    if (lum > 0.20) return 65600;     // :
    if (lum > 0.05) return 4194304;   // .
    return 0;                          // space
}

// Decode bit at position within 5x5 character grid
float character(int bits, vec2 p) {
    vec2 pos = floor(p * 5.0);
    if (pos.x < 0.0 || pos.x > 4.0 || pos.y < 0.0 || pos.y > 4.0) {
        return 0.0;
    }
    int idx = int(pos.x) + int(pos.y) * 5;
    return float((bits >> idx) & 1);
}

void main()
{
    // Cell size in UV space
    vec2 cellSize = vec2(float(cellPixels)) / resolution;

    // Sample at cell center
    vec2 cellUV = floor(fragTexCoord / cellSize) * cellSize + cellSize * 0.5;
    vec3 color = texture(texture0, cellUV).rgb;

    // Luminance calculation (ITU-R BT.601)
    float lum = dot(color, vec3(0.299, 0.587, 0.114));
    if (invert == 1) {
        lum = 1.0 - lum;
    }

    // Character lookup and rendering (shrink to 80% of cell for spacing)
    vec2 cellLocalUV = fract(fragTexCoord / cellSize);
    cellLocalUV = (cellLocalUV - 0.1) / 0.8;
    int charBits = getCharBits(lum);
    float mask = character(charBits, cellLocalUV);

    // Color mode selection
    vec3 result;
    if (colorMode == 0) {
        result = color * mask;
    } else if (colorMode == 1) {
        result = mix(background, foreground, mask);
    } else {
        // CRT green preset
        result = mix(vec3(0.0, 0.02, 0.0), vec3(0.0, 1.0, 0.0), mask);
    }

    finalColor = vec4(result, 1.0);
}
