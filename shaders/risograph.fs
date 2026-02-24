#version 330

// Risograph: CMY ink-layer separation with grain erosion and misregistration

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float grainScale;
uniform float grainIntensity;
uniform float grainTime;
uniform float misregAmount;
uniform float misregTime;
uniform float inkDensity;
uniform int posterize;
uniform float paperTone;

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

// Rotate between octaves to break simplex lattice alignment
const mat2 octRot = mat2(0.80, 0.60, -0.60, 0.80);

float grainNoise(vec2 st, float timeOffset) {
  // Snap to discrete frames — grain jumps randomly like film stock
  float frame = floor(timeOffset * 24.0);
  vec2 drift = vec2(
      fract(sin(frame * 127.1) * 43758.5453),
      fract(sin(frame * 311.7) * 43758.5453));
  vec2 p = st + drift;
  float n = 0.1 * snoise(p * 200.0);
  p = octRot * p;
  n += 0.05 * snoise(p * 400.0);
  p = octRot * p;
  n += 0.025 * snoise(p * 800.0);
  p = octRot * p;
  n += 0.0125 * snoise(p * 1600.0);
  return n;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 st = (fragTexCoord - 0.5) * resolution / grainScale;

    // --- Misregistration: slow base drift + frame-jitter wobble ---
    vec2 centered = fragTexCoord - 0.5;

    // Slow base drift
    vec2 offC = misregAmount * vec2(sin(misregTime * 0.7), cos(misregTime * 0.9));
    vec2 offM = misregAmount * vec2(sin(misregTime * 1.1 + 2.0), cos(misregTime * 0.6 + 1.0));
    vec2 offY = misregAmount * vec2(sin(misregTime * 0.8 + 4.0), cos(misregTime * 1.3 + 3.0));

    // Per-layer frame jitter — trembles like loose film gate
    float mFrame = floor(misregTime * 6.0);
    offC += misregAmount * 0.3 * vec2(
        fract(sin(mFrame * 93.7) * 43758.5453) - 0.5,
        fract(sin(mFrame * 147.3) * 43758.5453) - 0.5);
    offM += misregAmount * 0.3 * vec2(
        fract(sin(mFrame * 213.1) * 43758.5453) - 0.5,
        fract(sin(mFrame * 371.9) * 43758.5453) - 0.5);
    offY += misregAmount * 0.3 * vec2(
        fract(sin(mFrame * 457.3) * 43758.5453) - 0.5,
        fract(sin(mFrame * 619.7) * 43758.5453) - 0.5);

    // Per-layer spatial warp simulates paper feed distortion
    vec2 warpC = misregAmount * 0.15 * vec2(
        snoise(centered * 5.0 + vec2(0.0, mFrame)),
        snoise(centered * 5.0 + vec2(3.1, mFrame)));
    vec2 warpM = misregAmount * 0.15 * vec2(
        snoise(centered * 5.0 + vec2(7.7, mFrame)),
        snoise(centered * 5.0 + vec2(11.3, mFrame)));
    vec2 warpY = misregAmount * 0.15 * vec2(
        snoise(centered * 5.0 + vec2(17.1, mFrame)),
        snoise(centered * 5.0 + vec2(23.9, mFrame)));

    vec3 rgbC = texture(texture0, uv + offC + warpC).rgb;
    vec3 rgbM = texture(texture0, uv + offM + warpM).rgb;
    vec3 rgbY = texture(texture0, uv + offY + warpY).rgb;

    // Posterize each layer independently (if enabled)
    if (posterize > 0) {
        float levels = float(posterize);
        rgbC = floor(rgbC * levels + 0.5) / levels;
        rgbM = floor(rgbM * levels + 0.5) / levels;
        rgbY = floor(rgbY * levels + 0.5) / levels;
    }

    // --- CMY decomposition per layer ---
    vec3 cmyC = 1.0 - rgbC;
    float kC = min(cmyC.r, min(cmyC.g, cmyC.b));
    float cAmount = cmyC.r - kC;

    vec3 cmyM = 1.0 - rgbM;
    float kM = min(cmyM.r, min(cmyM.g, cmyM.b));
    float mAmount = cmyM.g - kM;

    vec3 cmyY = 1.0 - rgbY;
    float kY = min(cmyY.r, min(cmyY.g, cmyY.b));
    float yAmount = cmyY.b - kY;

    float k = (kC + kM + kY) / 3.0;

    // --- Grain: per-layer noise erodes ink coverage ---
    float nC = grainNoise(st, grainTime);
    float nM = grainNoise(st, grainTime + 17.3);
    float nY = grainNoise(st, grainTime + 31.7);

    float thresh = 1.0 - grainIntensity;
    float band = grainIntensity * 0.25;
    float grainC = smoothstep(thresh - band, thresh + band, nC * 0.5 + 0.5);
    float grainM = smoothstep(thresh - band, thresh + band, nM * 0.5 + 0.5);
    float grainY = smoothstep(thresh - band, thresh + band, nY * 0.5 + 0.5);

    cAmount *= (1.0 - grainC);
    mAmount *= (1.0 - grainM);
    yAmount *= (1.0 - grainY);

    // K gets its own grain too
    float nK = grainNoise(st, grainTime + 47.1);
    float grainK = smoothstep(thresh - band, thresh + band, nK * 0.5 + 0.5);
    k *= (1.0 - grainK);

    // --- Subtractive compositing ---
    vec3 tealInk = vec3(0.0, 0.72, 0.74);
    vec3 pinkInk = vec3(0.91, 0.16, 0.54);
    vec3 yellowInk = vec3(0.98, 0.82, 0.05);

    vec3 paper = mix(vec3(1.0), vec3(0.96, 0.93, 0.88), paperTone);

    float paperNoise = snoise(st * 3.0 + grainTime * 0.1) * 0.02;
    paper += paperNoise;

    vec3 result = paper;
    result *= 1.0 - cAmount * inkDensity * (1.0 - tealInk);
    result *= 1.0 - mAmount * inkDensity * (1.0 - pinkInk);
    result *= 1.0 - yAmount * inkDensity * (1.0 - yellowInk);
    result *= 1.0 - k * inkDensity * 0.7;

    finalColor = vec4(result, 1.0);
}
