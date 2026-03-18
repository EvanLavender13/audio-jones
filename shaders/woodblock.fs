#version 330

// Woodblock: Multi-layer woodcut print with static registration offset, wood grain, and keyblock outlines

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int levels;
uniform float edgeThreshold;
uniform float edgeSoftness;
uniform float edgeThickness;
uniform float grainIntensity;
uniform float grainScale;
uniform float grainAngle;
uniform float registrationOffset;
uniform float inkDensity;
uniform float paperTone;

// --- Simplex 2D noise (from risograph.fs) ---

vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289((( x * 34.0) + 1.0) * x); }

float snoise(vec2 v) {
  const vec4 C = vec4(0.211324865405187, 0.366025403784439,
    -0.577350269189626, 0.024390243902439);
  vec2 i = floor(v + dot(v, C.yy));
  vec2 x0 = v - i + dot(i, C.xx);
  vec2 i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;
  i = mod289(i);
  vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));
  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m; m = m*m;
  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 a0 = x - floor(x + 0.5);
  m *= 1.792843 - 0.853735 * (a0*a0 + h*h);
  vec3 g;
  g.x = a0.x * x0.x + h.x * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}

// --- Hash-based 3D noise for edge thickness variation (from toon.fs) ---

float hash(vec3 p)
{
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yxz + 19.19);
    return fract((p.x + p.y) * p.z);
}

float gnoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash(i + vec3(0.0, 0.0, 0.0));
    float n001 = hash(i + vec3(0.0, 0.0, 1.0));
    float n010 = hash(i + vec3(0.0, 1.0, 0.0));
    float n011 = hash(i + vec3(0.0, 1.0, 1.0));
    float n100 = hash(i + vec3(1.0, 0.0, 0.0));
    float n101 = hash(i + vec3(1.0, 0.0, 1.0));
    float n110 = hash(i + vec3(1.0, 1.0, 0.0));
    float n111 = hash(i + vec3(1.0, 1.0, 1.0));

    float n00 = mix(n000, n001, f.z);
    float n01 = mix(n010, n011, f.z);
    float n10 = mix(n100, n101, f.z);
    float n11 = mix(n110, n111, f.z);

    float n0 = mix(n00, n01, f.y);
    float n1 = mix(n10, n11, f.y);

    return mix(n0, n1, f.x) * 2.0 - 1.0;
}

// --- Value noise for wood grain ---

vec2 grainRandom(vec2 pos) {
    return fract(sin(vec2(
        dot(pos, vec2(12.9898, 78.233)),
        dot(pos, vec2(-148.998, -65.233))
    )) * 43758.5453);
}

float valueNoise(vec2 pos) {
    vec2 p = floor(pos);
    vec2 f = fract(pos);
    float v00 = grainRandom(p).x;
    float v10 = grainRandom(p + vec2(1.0, 0.0)).x;
    float v01 = grainRandom(p + vec2(0.0, 1.0)).x;
    float v11 = grainRandom(p + vec2(1.0, 1.0)).x;
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(v00, v10, u.x), mix(v01, v11, u.x), u.y);
}

// --- Wood grain generation ---

float woodGrain(vec2 uv, float scale, float angle) {
    float ca = cos(angle), sa = sin(angle);
    vec2 ruv = vec2(ca * uv.x + sa * uv.y, -sa * uv.x + ca * uv.y);
    ruv.x += valueNoise(ruv * 2.0);
    float x = ruv.x + sin(ruv.y * 4.0);
    return mod(x * scale + grainRandom(ruv).x * 0.3, 1.0);
}

// --- Posterization (max-RGB luminance quantization, from toon.fs) ---

vec3 posterize(vec3 color) {
    float greyscale = max(color.r, max(color.g, color.b));
    float fLevels = float(levels);
    float lower = floor(greyscale * fLevels) / fLevels;
    float upper = ceil(greyscale * fLevels) / fLevels;
    float level = (abs(greyscale - lower) <= abs(upper - greyscale)) ? lower : upper;
    float adjustment = level / max(greyscale, 0.001);
    return color * adjustment;
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 centered = fragTexCoord - 0.5;

    // --- Registration offsets: fixed per-layer direction + static spatial warp ---

    vec2 off0 = registrationOffset * vec2( 0.71,  0.71);
    vec2 off1 = registrationOffset * vec2(-0.87,  0.50);
    vec2 off2 = registrationOffset * vec2( 0.26, -0.97);

    // Static spatial warp - varies across image like uneven paper pressure
    vec2 warp0 = registrationOffset * 0.15 * vec2(
        snoise(centered * 5.0 + vec2(0.0, 1.7)),
        snoise(centered * 5.0 + vec2(3.1, 4.2)));
    vec2 warp1 = registrationOffset * 0.15 * vec2(
        snoise(centered * 5.0 + vec2(7.7, 9.3)),
        snoise(centered * 5.0 + vec2(11.3, 13.8)));
    vec2 warp2 = registrationOffset * 0.15 * vec2(
        snoise(centered * 5.0 + vec2(17.1, 19.6)),
        snoise(centered * 5.0 + vec2(23.9, 27.4)));

    // Sample three misregistered layers
    vec3 rgb0 = texture(texture0, uv + off0 + warp0).rgb;
    vec3 rgb1 = texture(texture0, uv + off1 + warp1).rgb;
    vec3 rgb2 = texture(texture0, uv + off2 + warp2).rgb;

    // --- Posterize each layer independently ---

    vec3 post0 = posterize(rgb0);
    vec3 post1 = posterize(rgb1);
    vec3 post2 = posterize(rgb2);

    // --- Sobel edge detection on un-offset center sample ---

    float noise = gnoise(vec3(uv * 5.0, 0.0));
    float thickMul = 1.0 + noise * 0.5;
    vec2 texel = edgeThickness * thickMul / resolution;

    // Sample 3x3 neighborhood for Sobel
    vec4 n[9];
    n[0] = texture(texture0, uv + vec2(-texel.x, -texel.y));
    n[1] = texture(texture0, uv + vec2(    0.0, -texel.y));
    n[2] = texture(texture0, uv + vec2( texel.x, -texel.y));
    n[3] = texture(texture0, uv + vec2(-texel.x,     0.0));
    n[4] = texture(texture0, uv);
    n[5] = texture(texture0, uv + vec2( texel.x,     0.0));
    n[6] = texture(texture0, uv + vec2(-texel.x,  texel.y));
    n[7] = texture(texture0, uv + vec2(    0.0,  texel.y));
    n[8] = texture(texture0, uv + vec2( texel.x,  texel.y));

    // Sobel horizontal and vertical gradients
    vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    // Edge magnitude from RGB channels
    float edge = length(sqrt(sobelH * sobelH + sobelV * sobelV).rgb);

    // Soft threshold to binary outline
    float outline = smoothstep(edgeThreshold - edgeSoftness, edgeThreshold + edgeSoftness, edge);

    // --- Compositing ---

    // Paper base (same warm tint as risograph.fs)
    vec3 paper = mix(vec3(1.0), vec3(0.96, 0.93, 0.88), paperTone);

    // Average the 3 misregistered posterized layers to get ink color
    vec3 ink = (post0 + post1 + post2) / 3.0;

    // Subtractive compositing: ink darkens paper, inkDensity controls coverage
    vec3 result = paper * mix(vec3(1.0), ink, inkDensity);

    // Wood grain: darken inked areas (darker ink shows more grain)
    float luma = dot(ink, vec3(0.299, 0.587, 0.114));
    float inked = 1.0 - luma;
    float grain = woodGrain(centered * resolution / 200.0, grainScale, grainAngle);
    result *= 1.0 - grainIntensity * inked * grain;

    // Keyblock outline on top (black)
    result = mix(result, vec3(0.0), outline);

    finalColor = vec4(result, 1.0);
}
