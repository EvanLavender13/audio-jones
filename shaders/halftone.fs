#version 330

// Halftone: four-color CMYK dot pattern simulating print halftoning

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float dotScale;    // Grid cell size in pixels
uniform float dotSize;     // Ink density multiplier
uniform float rotation;    // Grid rotation angle

out vec4 finalColor;

const float ANGLE_C = 0.2618;  // 15 degrees
const float ANGLE_M = 1.3090;  // 75 degrees
const float ANGLE_Y = 0.0;     // 0 degrees
const float ANGLE_K = 0.7854;  // 45 degrees

mat2 rotm(float r) {
    float c = cos(r), s = sin(r);
    return mat2(c, -s, s, c);
}

float cmykDot(vec2 fc, float angle, float cellSize) {
    mat2 m = rotm(angle);
    vec2 rotated = m * fc;
    vec2 cellLocal = fract(rotated / cellSize) - 0.5;
    return 1.7 - length(cellLocal) * 4.0;
}

void main()
{
    vec2 fc = fragTexCoord * resolution;

    // Sample texture at K-channel cell center
    mat2 mk = rotm(rotation + ANGLE_K);
    vec2 rotK = mk * fc;
    vec2 cellK = floor(rotK / dotScale) * dotScale + dotScale * 0.5;
    vec2 centerK = transpose(mk) * cellK;
    vec3 texColor = texture(texture0, centerK / resolution).rgb;

    // RGB to CMYK conversion
    float K = 1.0 - max(texColor.r, max(texColor.g, texColor.b));
    float invK = 1.0 - K;
    float C = invK > 0.001 ? (invK - texColor.r) / invK : 0.0;
    float M = invK > 0.001 ? (invK - texColor.g) / invK : 0.0;
    float Y = invK > 0.001 ? (invK - texColor.b) / invK : 0.0;

    // Ink presence test per channel
    float cInk = step(0.5, C * dotSize + cmykDot(fc, rotation + ANGLE_C, dotScale));
    float mInk = step(0.5, M * dotSize + cmykDot(fc, rotation + ANGLE_M, dotScale));
    float yInk = step(0.5, Y * dotSize + cmykDot(fc, rotation + ANGLE_Y, dotScale));
    float kInk = step(0.5, K * dotSize + cmykDot(fc, rotation + ANGLE_K, dotScale));

    // Subtractive compositing
    float r = (1.0 - cInk) * (1.0 - kInk);
    float g = (1.0 - mInk) * (1.0 - kInk);
    float b = (1.0 - yInk) * (1.0 - kInk);

    finalColor = vec4(r, g, b, 1.0);
}
