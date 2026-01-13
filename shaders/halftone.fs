#version 330

// Halftone: CMYK dot-matrix transform simulating print halftoning
// Converts to CMYK, samples each channel on rotated grid, renders anti-aliased dots

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float dotScale;    // Grid cell size in pixels
uniform float dotSize;     // Dot radius multiplier within cell
uniform float rotation;    // Combined rotationAngle + accumulated rotationSpeed
uniform float threshold;   // Smoothstep center for dot edge
uniform float softness;    // Smoothstep width for edge antialiasing

out vec4 finalColor;

// Standard CMYK screen angles (radians) to prevent moire
const float ANGLE_C = 0.2618;  // 15 degrees
const float ANGLE_M = 1.3090;  // 75 degrees
const float ANGLE_Y = 0.0;     // 0 degrees
const float ANGLE_K = 0.7854;  // 45 degrees

// Inverted CMYK: CMY as ratios, K as max RGB channel
vec4 rgb2cmyki(vec3 c) {
    float k = max(max(c.r, c.g), c.b);
    return min(vec4(c.rgb / k, k), 1.0);
}

vec3 cmyki2rgb(vec4 c) {
    return c.rgb * c.a;
}

// 2D rotation matrix
mat2 rotm(float r) {
    float cr = cos(r);
    float sr = sin(r);
    return mat2(cr, -sr, sr, cr);
}

// Snap to grid cell corner
vec2 grid(vec2 px, float S) {
    return px - mod(px, S);
}

// Sample halftone for one CMYK channel
// Returns channel value + distance factor for dot generation
vec4 halftone(vec2 fc, mat2 m, float S) {
    vec2 smp = (grid(m * fc, S) + 0.5 * S) * m;  // Rotate, grid, unrotate
    float s = min(length(fc - smp) / (dotSize * 0.5 * S), 1.0);
    vec3 texc = texture(texture0, smp / resolution).rgb;
    texc = pow(texc, vec3(2.2));  // Gamma decode
    vec4 c = rgb2cmyki(texc);
    return c + s;  // Add distance to ink values
}

// Smoothstep antialiasing for dot edges
vec4 ss(vec4 v) {
    return smoothstep(threshold - softness, threshold + softness, v);
}

void main()
{
    vec2 fc = fragTexCoord * resolution;

    // Build rotation matrices for each CMYK channel
    mat2 mc = rotm(rotation + ANGLE_C);
    mat2 mm = rotm(rotation + ANGLE_M);
    mat2 my = rotm(rotation + ANGLE_Y);
    mat2 mk = rotm(rotation + ANGLE_K);

    // Sample halftone for each CMYK channel
    vec4 halftoneResult = vec4(
        halftone(fc, mc, dotScale).r,  // Cyan
        halftone(fc, mm, dotScale).g,  // Magenta
        halftone(fc, my, dotScale).b,  // Yellow
        halftone(fc, mk, dotScale).a   // Key (black)
    );

    // Apply smoothstep for anti-aliased dots and convert back to RGB
    vec3 c = cmyki2rgb(ss(halftoneResult));

    // Gamma encode
    c = pow(c, vec3(1.0 / 2.2));

    finalColor = vec4(c, 1.0);
}
