#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Input image
uniform sampler2D texture1;   // 1D gradient LUT (256x1)
uniform vec2 resolution;
uniform float shift;          // Palette rotation offset
uniform float intensity;      // Global blend strength
uniform float radial;         // Radial blend coefficient
uniform vec2 center;          // Radial center (0-1)

// RGB to HSV (same as color_grade.fs)
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec4 color = texture(texture0, fragTexCoord);
    vec3 hsv = rgb2hsv(color.rgb);

    // Sample custom color wheel using pixel hue + shift offset
    float t = fract(hsv.x + shift);
    vec3 remappedRGB = texture(texture1, vec2(t, 0.5)).rgb;
    vec3 remappedHSV = rgb2hsv(remappedRGB);

    // Keep original saturation and value, take remapped hue
    vec3 result = hsv2rgb(vec3(remappedHSV.x, hsv.y, hsv.z));

    // Compute radial blend field
    vec2 uv = fragTexCoord - center;
    float aspect = resolution.x / resolution.y;
    if (aspect > 1.0) { uv.x /= aspect; } else { uv.y *= aspect; }
    float rad = length(uv) * 2.0;
    float blend = clamp(intensity + radial * rad, 0.0, 1.0);

    finalColor = vec4(mix(color.rgb, result, blend), color.a);
}
