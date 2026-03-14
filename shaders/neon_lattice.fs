// Based on "Inside the System" by kishimisu (2022)
// https://www.shadertoy.com/view/msj3D3
// License: CC BY-NC-SA 4.0
// Neon glow falloff inspired by: https://www.shadertoy.com/view/3dlcWl
// Modified: uniforms replace defines, gradientLUT for color, configurable axis count

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D gradientLUT;

// Grid
uniform float spacing;       // 7.0
uniform float lightSpacing;  // 2.0
uniform float attenuation;   // 22.0
uniform float glowExponent;  // 1.3

// Speed phases (CPU-accumulated)
uniform float cameraTime;
uniform float columnsTime;
uniform float lightsTime;

// Quality
uniform int iterations;       // 50
uniform float maxDist;        // 80.0

// Torus shape
uniform float torusRadius;    // 0.6
uniform float torusTube;      // 0.06

// Axis count
uniform int axisCount;         // 3

// FFT audio
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define EPSILON 0.005

#define rot(a) mat2(cos(a), -sin(a), sin(a), cos(a))
#define rep(p, r) (mod(p + r / 2.0, r) - r / 2.0)

float torusSDF(vec3 p) {
    return length(vec2(length(p.xz) - torusRadius, p.y)) - torusTube;
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 getLight(vec3 p, vec3 color) {
    return max(vec3(0.0), color / (1.0 + pow(abs(torusSDF(p) * attenuation), glowExponent)) - 0.001);
}

vec3 geo(vec3 po, inout float d, inout vec2 f) {
    // Shape repetition
    float r = hash12(floor(po.yz / spacing + vec2(0.5))) - 0.5;
    vec3 p = rep(po + vec3(columnsTime * r, 0.0, 0.0), vec3(0.5, spacing, spacing));
    p.xy *= rot(1.5707963);
    d = min(d, torusSDF(p));

    // Light repetition
    f = floor(po.yz / (spacing * lightSpacing) - vec2(0.5));
    r = hash12(f) - 0.5;
    if (r > -0.45)
        p = rep(po + vec3(lightsTime * r, 0.0, 0.0), spacing * lightSpacing * vec3(r + 0.54, 1.0, 1.0));
    else
        p = rep(po + vec3(lightsTime * 0.5 * (1.0 + r * 0.003 * hash12(floor(po.yz * spacing))), 0.0, 0.0), spacing * lightSpacing);
    p.xy *= rot(1.5707963);
    f = cos(f.xy) * 0.5 + 0.5;

    return p;
}

float fftMag(float t) {
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = texture(fftTexture, vec2(bin, 0.5)).r;
    return pow(clamp(energy * gain, 0.0, 1.0), curve);
}

vec4 map(vec3 p) {
    float d = 1e6;
    vec3 po, col = vec3(0.0);
    vec2 f;

    // Axis 1 (always)
    po = geo(p, d, f);
    float t1 = fract(f.x + f.y);
    col += getLight(po, texture(gradientLUT, vec2(t1, 0.5)).rgb) * (baseBright + fftMag(t1));

    if (axisCount >= 2) {
        // Rotate into axis 2
        p.z += spacing / 2.0;
        p.xy *= rot(1.5707963);
        po = geo(p, d, f);
        float t2 = fract(f.x + f.y + 0.33);
        col += getLight(po, texture(gradientLUT, vec2(t2, 0.5)).rgb) * (baseBright + fftMag(t2));
    }

    if (axisCount >= 3) {
        // Rotate into axis 3
        p.xy += spacing / 2.0;
        p.xz *= rot(1.5707963);
        po = geo(p, d, f);
        float t3 = fract(f.x + f.y + 0.66);
        col += getLight(po, texture(gradientLUT, vec2(t3, 0.5)).rgb) * (baseBright + fftMag(t3));
    }

    return vec4(col, d);
}

vec3 getOrigin(float t) {
    t = (t + 35.0) * -0.05;
    float rad = mix(50.0, 80.0, cos(t * 1.24) * 0.5 + 0.5);
    return vec3(rad * sin(t * 0.97), rad * cos(t * 1.11), rad * sin(t * 1.27));
}

void initRay(vec2 uv, out vec3 ro, out vec3 rd) {
    ro = getOrigin(cameraTime);
    vec3 f = normalize(getOrigin(cameraTime + 0.5) - ro);
    vec3 r = normalize(cross(normalize(ro), f));
    rd = normalize(f + uv.x * r + uv.y * cross(f, r));
}

void main() {
    // Centered coordinates from pixel position (raymarcher — fragTexCoord convention N/A)
    vec2 uv = (2.0 * gl_FragCoord.xy - resolution) / resolution.y;
    vec3 p, ro, rd, col = vec3(0.0);

    initRay(uv, ro, rd);

    float t = 2.0;
    for (int i = 0; i < iterations; i++) {
        p = ro + t * rd;

        vec4 res = map(p);
        col += res.rgb;
        t += abs(res.w);

        if (abs(res.w) < EPSILON) t += EPSILON;

        // Early saturation exit — significant perf win in bright scenes
        if (col.r >= 1.0 && col.g >= 1.0 && col.b >= 1.0) break;
        if (t > maxDist) break;
    }

    col = pow(col, vec3(0.45));
    finalColor = vec4(col, 1.0);
}
