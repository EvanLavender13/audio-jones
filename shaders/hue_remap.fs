#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Input image
uniform sampler2D texture1;   // 1D gradient LUT (256x1)
uniform vec2 resolution;
uniform float shift;          // Palette rotation offset
uniform float intensity;      // Global blend strength
uniform vec2 center;          // Radial center (0-1)
uniform float time;           // Elapsed time for noise drift

// Blend spatial coefficients (multiplicative field)
uniform float blendRadial;
uniform float blendAngular;
uniform int blendAngularFreq;
uniform float blendLinear;
uniform float blendLinearAngle;
uniform float blendLuminance;
uniform float blendNoise;

// Shift spatial coefficients (additive field)
uniform float shiftRadial;
uniform float shiftAngular;
uniform int shiftAngularFreq;
uniform float shiftLinear;
uniform float shiftLinearAngle;
uniform float shiftLuminance;
uniform float shiftNoise;

uniform int shiftMode;        // 0 = Replace (LUT), 1 = Shift (direct hue offset)

// Noise parameters
uniform float noiseScale;

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

// 2D hash-based smooth noise (no grid artifacts)
vec2 hash22(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);  // smoothstep interpolant

    return mix(mix(dot(hash22(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
                   dot(hash22(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
               mix(dot(hash22(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
                   dot(hash22(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x), u.y);
}

// Multiplicative spatial field for blend masking
// All zeros = uniform 1.0 (full blend everywhere)
float computeSpatialField(float radialCoeff, float angularCoeff, int angularFreq,
                          float linearCoeff, float linearAngle,
                          float luminanceCoeff, float noiseCoeff,
                          float rad, float ang, float luma, float n,
                          vec2 fragUV, vec2 cen) {
    float field = 1.0;

    if (radialCoeff != 0.0) {
        float rv = (radialCoeff > 0.0) ? rad : 1.0 - rad;
        field *= mix(1.0, rv, abs(radialCoeff));
    }

    if (angularCoeff != 0.0) {
        float av = sin(ang * float(angularFreq)) * 0.5 + 0.5;
        if (angularCoeff < 0.0) av = 1.0 - av;
        field *= mix(1.0, av, abs(angularCoeff));
    }

    if (linearCoeff != 0.0) {
        vec2 dir = vec2(cos(linearAngle), sin(linearAngle));
        float lv = dot(fragUV - cen, dir) + 0.5;
        lv = clamp(lv, 0.0, 1.0);
        if (linearCoeff < 0.0) lv = 1.0 - lv;
        field *= mix(1.0, lv, abs(linearCoeff));
    }

    if (luminanceCoeff != 0.0) {
        float lumv = luma;
        if (luminanceCoeff < 0.0) lumv = 1.0 - lumv;
        field *= mix(1.0, lumv, abs(luminanceCoeff));
    }

    if (noiseCoeff != 0.0) {
        float nv = n * 0.5 + 0.5;
        if (noiseCoeff < 0.0) nv = 1.0 - nv;
        field *= mix(1.0, nv, abs(noiseCoeff));
    }

    return field;
}

// Additive spatial field for hue shift offset
// All zeros = zero offset = no spatial shift
float computeShiftField(float radialCoeff, float angularCoeff, int angularFreq,
                         float linearCoeff, float linearAngle,
                         float luminanceCoeff, float noiseCoeff,
                         float rad, float ang, float luma, float n,
                         vec2 fragUV, vec2 cen) {
    float field = 0.0;

    field += radialCoeff * rad;
    field += angularCoeff * (sin(ang * float(angularFreq)) * 0.5 + 0.5);

    vec2 dir = vec2(cos(linearAngle), sin(linearAngle));
    float lv = clamp(dot(fragUV - cen, dir) + 0.5, 0.0, 1.0);
    field += linearCoeff * lv;

    field += luminanceCoeff * luma;
    field += noiseCoeff * (n * 0.5 + 0.5);

    return field;
}

void main() {
    vec4 color = texture(texture0, fragTexCoord);
    vec3 hsv = rgb2hsv(color.rgb);

    // Compute shared spatial coords
    vec2 uv = fragTexCoord - center;
    float aspect = resolution.x / resolution.y;
    if (aspect > 1.0) { uv.x /= aspect; } else { uv.y *= aspect; }
    float rad = length(uv) * 2.0;
    float ang = atan(uv.y, uv.x);
    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    float n = noise2D((fragTexCoord - center) * noiseScale + time);

    // Blend spatial field (multiplicative)
    float blendField = computeSpatialField(
        blendRadial, blendAngular, blendAngularFreq,
        blendLinear, blendLinearAngle,
        blendLuminance, blendNoise,
        rad, ang, luma, n, fragTexCoord, center);
    float blend = clamp(intensity * blendField, 0.0, 1.0);

    // Shift spatial field (additive)
    float shiftField = computeShiftField(
        shiftRadial, shiftAngular, shiftAngularFreq,
        shiftLinear, shiftLinearAngle,
        shiftLuminance, shiftNoise,
        rad, ang, luma, n, fragTexCoord, center);

    // Compute remapped color based on mode
    vec3 result;
    if (shiftMode != 0) {
        // Shift mode: offset hue directly, skip LUT
        float newHue = fract(hsv.x + shift + shiftField);
        result = hsv2rgb(vec3(newHue, hsv.y, hsv.z));
    } else {
        // Replace mode: sample custom color wheel LUT
        float t = fract(hsv.x + shift + shiftField);
        vec3 remappedRGB = texture(texture1, vec2(t, 0.5)).rgb;
        vec3 remappedHSV = rgb2hsv(remappedRGB);
        result = hsv2rgb(vec3(remappedHSV.x, hsv.y, hsv.z));
    }

    finalColor = vec4(mix(color.rgb, result, blend), color.a);
}
