# Geode

A spherical cluster of randomly-rotated crystal cubes carved open by a 3D gyroid lattice or Perlin noise field. The camera orbits while the cut field drifts and pulses, opening caverns on one side as they close on another. Full lighting (self-shadowing, AO, specular, fog) gives each cube crystal-like depth. Single-pass DDA voxel raymarcher.

**Research**: `docs/research/geode.md`
**Reference**: "randomly rotated cubes noise cut" by jt (Shadertoy `tXKXzh`, MIT)

## Design

### Types

**`GeodeConfig`** (in `src/effects/geode.h`):

```cpp
struct GeodeConfig {
  bool enabled = false;

  // Cluster
  float clusterRadius = 20.0f;  // Spherical bound (5.0-25.0)
  float cubeSize = 0.632f;      // Per-cube SDF half-extent (0.3-0.7); default = sqrt(0.40)
  float colorRate = 0.1f;       // Gradient cycle rate along radial distance (0.02-0.5)

  // Cut Field
  int cutMode = 0;              // 0=GYROID, 1=NOISE
  float cutScale = 0.05f;       // Cut-field wavelength (0.02-0.25); 0.05 = reference's 1/20 for gyroid
  float cutThresholdBase = 0.0f;     // Static threshold offset (-2.0-2.0)
  float cutThresholdPulse = 0.0f;    // Threshold oscillation amplitude (0.0-2.0)
  float cutPulseSpeed = 0.5f;        // Threshold oscillation frequency Hz (0.0-4.0)
  float fieldDriftX = 0.0f;          // Cut-field X drift rate units/sec (-2.0-2.0)
  float fieldDriftY = 0.0f;          // Cut-field Y drift rate units/sec (-2.0-2.0)
  float fieldDriftZ = 0.0f;          // Cut-field Z drift rate units/sec (-2.0-2.0)

  // Camera
  float orbitSpeed = 0.06f;          // Yaw orbit rate rad/sec (-1.0-1.0); reference's 2*pi*0.01
  float orbitPitch = 2.094395f;      // Fixed pitch radians (-PI to PI); default 2*PI/3 from reference
  float cameraDistance = 50.0f;      // Camera Z distance pre-rotation (30.0-80.0)

  // Lighting
  float ambient = 0.1f;              // Baseline lighting (0.0-0.5)
  float specularPower = 250.0f;      // Specular exponent (10.0-1000.0)
  float fogDistance = 25.0f;         // Distance at which fog reaches ~1/e (10.0-100.0)

  // Audio
  float baseFreq = 55.0f;            // FFT low Hz (27.5-440)
  float maxFreq = 14000.0f;          // FFT high Hz (1000-16000)
  float gain = 2.0f;                 // FFT gain (0.1-10)
  float curve = 1.5f;                // FFT curve (0.1-3)
  float baseBright = 0.15f;          // FFT min floor (0-1)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_ADD;
  float blendIntensity = 1.0f;
};

#define GEODE_CONFIG_FIELDS                                                    \
  enabled, clusterRadius, cubeSize, colorRate, cutMode, cutScale,              \
      cutThresholdBase, cutThresholdPulse, cutPulseSpeed, fieldDriftX,         \
      fieldDriftY, fieldDriftZ, orbitSpeed, orbitPitch, cameraDistance,        \
      ambient, specularPower, fogDistance, baseFreq, maxFreq, gain, curve,     \
      baseBright, gradient, blendMode, blendIntensity
```

**`GeodeEffect`** (runtime state):

```cpp
typedef struct ColorLUT ColorLUT;

typedef struct GeodeEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases (per conventions: shader never sees raw time)
  float cutPulsePhase;
  float orbitPhase;
  float fieldOffsetX;
  float fieldOffsetY;
  float fieldOffsetZ;

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;
  int frameLoc;

  int cutModeLoc;
  int cutScaleLoc;
  int cutThresholdBaseLoc;
  int cutThresholdPulseLoc;
  int cutPulsePhaseLoc;
  int fieldOffsetLoc;
  int clusterRadiusLoc;
  int cubeSizeLoc;
  int colorRateLoc;

  int orbitPhaseLoc;
  int orbitPitchLoc;
  int cameraDistanceLoc;

  int ambientLoc;
  int specularPowerLoc;
  int fogDistanceLoc;

  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} GeodeEffect;
```

**Public functions**:

```cpp
bool GeodeEffectInit(GeodeEffect *e, const GeodeConfig *cfg);
void GeodeEffectSetup(GeodeEffect *e, const GeodeConfig *cfg,
                      float deltaTime, const Texture2D &fftTexture);
void GeodeEffectUninit(GeodeEffect *e);
void GeodeRegisterParams(GeodeConfig *cfg);
```

### Algorithm

The shader is a near-verbatim transcription of the reference (`tXKXzh`) with the substitutions in the Replacements table below. Transcribe top-to-bottom; do not reorder helpers, do not "improve" the math, do not collapse function boundaries.

#### Shader file: `shaders/geode.fs`

```glsl
// Based on "randomly rotated cubes noise cut" by jt (Jakob Thomsen)
// https://www.shadertoy.com/view/tXKXzh
// License: MIT
// DDA voxel renderer: "dda voxel no div raymarch subobj" by jt
// https://www.shadertoy.com/view/3XBSW1
// Hash functions: "Hash without Sine" by Dave_Hoskins
// https://www.shadertoy.com/view/4djSRW
// Integer hash (triple32 / lowbias32): Chris Wellons / FabriceNeyret2
// Modified for AudioJones: parameterized uniforms, gradientLUT coloring,
//   radial-band FFT modulation, CPU-accumulated phases, time-driven camera,
//   output linear HDR (no tanh / no sqrt gamma).
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform int frame;

uniform int cutMode;             // 0 = GYROID, 1 = NOISE
uniform float cutScale;
uniform float cutThresholdBase;
uniform float cutThresholdPulse;
uniform float cutPulsePhase;
uniform vec3 fieldOffset;
uniform float clusterRadius;
uniform float cubeSize;
uniform float colorRate;

uniform float orbitPhase;
uniform float orbitPitch;
uniform float cameraDistance;

uniform float ambient;
uniform float specularPower;
uniform float fogDistance;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const int MAX_VOXEL_STEPS = 128;
const int MAX_MARCH_STEPS = 100;
const float DIST_MAX = 200.0;

#define ZERO (min(0, frame))
#define PI 3.1415926
#define TAU (PI + PI)

// --- Hash functions (verbatim from Dave_Hoskins, Shadertoy 4djSRW) ---

float hash12(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p3) {
  p3 = fract(p3 * 0.1031);
  p3 += dot(p3, p3.zyx + 31.32);
  return fract((p3.x + p3.y) * p3.z);
}

vec3 hash32(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
  p3 += dot(p3, p3.yxz + 33.33);
  return fract((p3.xxy + p3.yzz) * p3.zyx);
}

vec3 hash33(vec3 p3) {
  p3 = fract(p3 * vec3(0.1031, 0.1030, 0.0973));
  p3 += dot(p3, p3.yxz + 33.33);
  return fract((p3.xxy + p3.yxx) * p3.zyx);
}

// --- Integer hashes (Wellons / FabriceNeyret2) ---

uint triple32(uint x) {
  x ^= x >> 17;
  x *= 0xed5ad4bbU;
  x ^= x >> 11;
  x *= 0xac4c1b51U;
  x ^= x >> 15;
  x *= 0x31848babU;
  x ^= x >> 14;
  return x;
}

uint lowbias32(uint x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

#define HASH(u) triple32(u)

uint uhash(uvec2 v) { return HASH(uint(v.x) + HASH(uint(v.y))); }
uint uhash(uvec3 v) { return HASH(v.x + HASH(v.y + HASH(v.z))); }

// --- 3D Perlin noise (used when cutMode == NOISE) ---

vec3 grad(ivec3 v) { return hash33(vec3(v)) * 2.0 - 1.0; }

float noise(vec3 p) {
  ivec3 i = ivec3(floor(p));
  vec3 f = fract(p);
  vec3 u = smoothstep(vec3(0.0), vec3(1.0), f);
  return mix(
      mix(mix(dot(grad(i + ivec3(0, 0, 0)), f - vec3(0, 0, 0)),
              dot(grad(i + ivec3(1, 0, 0)), f - vec3(1, 0, 0)), u.x),
          mix(dot(grad(i + ivec3(0, 1, 0)), f - vec3(0, 1, 0)),
              dot(grad(i + ivec3(1, 1, 0)), f - vec3(1, 1, 0)), u.x),
          u.y),
      mix(mix(dot(grad(i + ivec3(0, 0, 1)), f - vec3(0, 0, 1)),
              dot(grad(i + ivec3(1, 0, 1)), f - vec3(1, 0, 1)), u.x),
          mix(dot(grad(i + ivec3(0, 1, 1)), f - vec3(0, 1, 1)),
              dot(grad(i + ivec3(1, 1, 1)), f - vec3(1, 1, 1)), u.x),
          u.y),
      u.z);
}

// --- Gyroid (used when cutMode == GYROID) ---

float gyroid(vec3 p) {
  return dot(sin(p.xyz * TAU), cos(p.yzx * TAU));
}

// --- Box SDF (iquilezles) ---

float box(vec3 p, vec3 s) {
  vec3 d = abs(p) - s;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

// --- Random rotation via uniform quaternion ---

vec4 random_unit_quaternion(vec3 r) {
  return vec4(sqrt(1.0 - r.x) * sin(2.0 * PI * r.y),
              sqrt(1.0 - r.x) * cos(2.0 * PI * r.y),
              sqrt(r.x) * sin(2.0 * PI * r.z),
              sqrt(r.x) * cos(2.0 * PI * r.z));
}

mat3 quaternion_to_matrix(vec4 q) {
  vec3 u = q.xyz;
  float w = q.w;
  mat3 u_outer = outerProduct(u, u);
  mat3 u_cross = mat3(vec3(0, -u.z, u.y),
                      vec3(u.z, 0, -u.x),
                      vec3(-u.y, u.x, 0));
  float s = dot(q, q);
  return (mat3(w * w - dot(u, u)) + 2.0 * (u_outer + w * u_cross)) / s;
}

mat3 random_rotation(vec3 r) {
  return quaternion_to_matrix(random_unit_quaternion(r));
}

// --- Camera basis ---

mat3 yaw_pitch_roll(float yaw, float pitch, float roll) {
  mat3 R = mat3(vec3(cos(yaw), sin(yaw), 0.0),
                vec3(-sin(yaw), cos(yaw), 0.0),
                vec3(0.0, 0.0, 1.0));
  mat3 S = mat3(vec3(1.0, 0.0, 0.0),
                vec3(0.0, cos(pitch), sin(pitch)),
                vec3(0.0, -sin(pitch), cos(pitch)));
  mat3 T = mat3(vec3(cos(roll), 0.0, sin(roll)),
                vec3(0.0, 1.0, 0.0),
                vec3(-sin(roll), 0.0, cos(roll)));
  return R * S * T;
}

// --- Sign workaround (some compilers misbehave on builtin sign) ---

vec3 sgn(vec3 v) { return step(vec3(0), v) * 2.0 - 1.0; }

float maxcomp(vec3 v) { return max(max(v.x, v.y), v.z); }

// --- Result struct and combiner ---

struct result {
  vec3 color;
  float dist;
};

result combine(result a, result b) {
  if (a.dist < b.dist) return a;
  return b;
}

// --- Cluster / cut-field tests ---
// Cluster bound: parameterized clusterRadius (replaces 10.0 * N).
// Cut sample: cutScale * voxel + fieldOffset (replaces /20 or /10 with drift).
// Threshold: cutThresholdBase + cutThresholdPulse * sin(cutPulsePhase).

bool shape_single(ivec3 voxel) {
  if (length(vec3(voxel)) > clusterRadius) return false;
  vec3 p = vec3(voxel) * cutScale + fieldOffset;
  float threshold = cutThresholdBase + cutThresholdPulse * sin(cutPulsePhase);
  float field = (cutMode == 0) ? gyroid(p) : noise(p);
  return field < threshold;
}

bool shape(ivec3 voxel) {
  for (int z = ZERO; z <= 1; z++) {
    for (int y = ZERO; y <= 1; y++) {
      for (int x = ZERO; x <= 1; x++) {
        if (shape_single(voxel + ivec3(x, y, z))) return true;
      }
    }
  }
  return false;
}

// --- Per-voxel cube SDF + color ---
// Color: gradientLUT sampled by fract(length(voxel) * colorRate)
//        (replaces rainbow(length(voxel) * 0.1)).
// FFT injection: log-spaced bands by radial distance from cluster center.
// Cube half-extent: cubeSize uniform (replaces sqrt(0.40)).
// Rotation seed: hash33(voxel * 123.456) — UNCHANGED from reference.

result item(ivec3 voxel, vec3 local) {
  if (!shape_single(voxel)) return result(vec3(0), DIST_MAX);

  float t = fract(length(vec3(voxel)) * colorRate);
  vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

  float radialT = clamp(length(vec3(voxel)) / clusterRadius, 0.0, 1.0);
  float freq = baseFreq * pow(maxFreq / baseFreq, radialT);
  float bin = freq / (sampleRate * 0.5);
  float energy = baseBright +
                 pow(clamp(texture(fftTexture, vec2(bin, 0.5)).r * gain, 0.0, 1.0),
                     curve);
  color *= energy;

  vec3 r = hash33(vec3(voxel) * 123.456);
  mat3 R = random_rotation(r.xyz);
  return result(color, box(R * (local - 0.5), vec3(cubeSize)));
}

result map(ivec3 voxel, vec3 local) {
  result r = result(vec3(0), DIST_MAX);
  for (int z = ZERO; z <= 1; z++) {
    for (int y = ZERO; y <= 1; y++) {
      for (int x = ZERO; x <= 1; x++) {
        r = combine(r, item(voxel + ivec3(x, y, z),
                            local - vec3(x, y, z) + 0.5));
      }
    }
  }
  return r;
}

result map(vec3 p) {
  ivec3 voxel = ivec3(floor(p));
  vec3 local = fract(p);
  if (shape(voxel)) return map(voxel, local);
  return result(vec3(0), DIST_MAX);
}

// --- Normals (both overloads — reference keeps both) ---

vec3 normal(ivec3 voxel, vec3 p) {
  const float h = 0.001;
  const vec2 k = vec2(1, -1);
  return normalize(k.xyy * map(voxel, p + k.xyy * h).dist +
                   k.yyx * map(voxel, p + k.yyx * h).dist +
                   k.yxy * map(voxel, p + k.yxy * h).dist +
                   k.xxx * map(voxel, p + k.xxx * h).dist);
}

vec3 normal(vec3 p) {
  const float h = 0.0001;
  vec3 n = vec3(0);
  for (int i = ZERO; i < 4; i++) {
    vec3 e = (2.0 * vec3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0) /
             sqrt(3.0);
    n += e * map(p + e * h).dist;
  }
  return normalize(n);
}

// --- Inner SDF raymarch (for the cubes inside one voxel) ---

result raymarch(ivec3 voxel, vec3 ro, vec3 rd, float t0, float t1, bool pass) {
  const float raymarch_epsilon = 0.001;
  result h;
  int i;
  float t;
  for (t = t0, i = ZERO; t < t1 && i < MAX_MARCH_STEPS; i++) {
    vec3 p = ro + rd * t;
    h = map(voxel, p);
    if (h.dist < raymarch_epsilon) return result(h.color, t);
    t += h.dist;
  }
  if (!pass) return result(h.color, t);
  return result(vec3(0), t1);
}

// --- DDA voxel traversal (verbatim — including the duplicate `nearest`
//     shadowing pattern and the `margin = 0.001` constant) ---

result traverse_voxels(vec3 ray_pos, vec3 ray_dir, bool pass) {
  ivec3 voxel = ivec3(floor(ray_pos));
  bvec3 nearest = bvec3(0);

  for (int i = ZERO; i < MAX_VOXEL_STEPS; i++) {
    vec3 offset = ray_pos - vec3(voxel) - vec3(0.5);
    vec3 side_offset = 0.5 - sgn(ray_dir) * offset;

    if (shape(voxel)) {
      vec3 color0 = vec3(1, 1, 0) * dot(vec3(0.5, 1.0, 0.75), vec3(nearest));
      float d0 = maxcomp(vec3(nearest) * (side_offset - 1.0) / abs(ray_dir));

      vec3 code = side_offset * abs(ray_dir.yzx * ray_dir.zxy);
      bvec3 nearest = lessThanEqual(code.xyz, min(code.yzx, code.zxy));

      vec3 side_offset = 0.5 - (sgn(ray_dir) * offset - vec3(nearest));

      vec3 color1 = vec3(0, 0, 1) * dot(vec3(0.5, 1.0, 0.75), vec3(nearest));
      float d1 = maxcomp(vec3(nearest) * (side_offset - 1.0) / abs(ray_dir));

      const float margin = 0.001;
      result r = raymarch(voxel, offset + 0.5, ray_dir, d0 - margin, d1 + margin, pass);
      r.dist = max(d0, r.dist);
      if (r.dist < d1 + margin) return r;
    }

    vec3 code = side_offset * abs(ray_dir.yzx * ray_dir.zxy);
    nearest = lessThanEqual(code.xyz, min(code.yzx, code.zxy));
    voxel += ivec3(vec3(nearest) * sgn(ray_dir));
  }

  return result(vec3(0), DIST_MAX);
}

// --- Ambient occlusion (iq Xds3zN) ---

float ambient_occlusion(vec3 pos, vec3 nor) {
  float occ = 0.0;
  float sca = 1.0;
  for (int i = ZERO; i < 5; i++) {
    float h = 0.01 + 0.12 * float(i) / 4.0;
    float d = map(pos + h * nor).dist;
    occ += (h - d) * sca;
    sca *= 0.95;
    if (occ > 0.35) break;
  }
  return clamp(1.0 - 3.0 * occ, 0.0, 1.0);
}

// --- Main ---
// I mapping: AudioJones convention (centered, range [-aspect, aspect] x [-1, 1]).
// SPEEDUP_CHEAT: the cluster is bounded; pixels outside the unit disc cannot hit.
// Camera: yaw = orbitPhase (CPU-accumulated), pitch = orbitPitch.
// Tonemap removed: output linear HDR, the pipeline's Gamma effect handles output.

void main() {
  vec2 I = (fragTexCoord * resolution - resolution * 0.5) /
           (resolution.y * 0.5);

  if (length(I) > 1.0) {
    finalColor = vec4(0.0);
    return;
  }

  float yaw = orbitPhase;
  float pitch = orbitPitch;

  vec3 ray_pos = vec3(0.0, 0.0, -cameraDistance);
  vec3 ray_dir = vec3(I.x, I.y, 2.0);

  mat3 M = yaw_pitch_roll(yaw, pitch, 0.0);
  ray_pos = M * ray_pos;
  ray_dir = M * ray_dir;
  ray_dir = normalize(ray_dir);

  vec3 light_dir = normalize(vec3(0, 0, -3));
  light_dir = M * light_dir;

  vec3 sky_color = vec3(0);
  vec3 color = vec3(0);
  result r = traverse_voxels(ray_pos, ray_dir, false);
  if (r.dist < DIST_MAX) {
    color = r.color;
    vec3 surf = ray_pos + ray_dir * r.dist;
    vec3 norm = normal(surf);

    float ao = ambient_occlusion(surf, norm);
    float diffuse = max(0.0, dot(light_dir, norm));

    if (diffuse > 0.0) {
      if (traverse_voxels(surf + norm * 0.01, light_dir, true).dist < DIST_MAX) {
        diffuse = 0.0;
      }
    }

    color *= (ambient * ao + diffuse);

    if (diffuse > 0.0) {
      float specular = pow(max(0.0, dot(norm, normalize(-ray_dir + light_dir))),
                           specularPower);
      color += specular;
    }

    vec3 fog_color = sky_color;
    color = mix(fog_color, vec3(color),
                exp(-pow(r.dist / fogDistance, 2.0)));
  } else {
    color = sky_color;
  }

  finalColor = vec4(color, 1.0);
}
```

#### CPU phase accumulation (in `GeodeEffectSetup`)

```
e->cutPulsePhase += cfg->cutPulseSpeed * deltaTime;
e->orbitPhase    += cfg->orbitSpeed * deltaTime;
e->fieldOffsetX  += cfg->fieldDriftX * deltaTime;
e->fieldOffsetY  += cfg->fieldDriftY * deltaTime;
e->fieldOffsetZ  += cfg->fieldDriftZ * deltaTime;
```

Bind `fieldOffset` as a `vec3` uniform built from `(e->fieldOffsetX, e->fieldOffsetY, e->fieldOffsetZ)`.

The `frame` int uniform feeds the `ZERO = min(0, frame)` macro that prevents loop unrolling on Windows GLSL compilers (per the reference's compile-time note). It does NOT come from raylib; maintain `int frame;` on `GeodeEffect`, increment it inside `Setup` (`e->frame++;`), and bind via `SetShaderValue(e->shader, e->frameLoc, &e->frame, SHADER_UNIFORM_INT);`.

#### Setup function flow

1. Accumulate phases (above).
2. `ColorLUTUpdate(e->gradientLUT, &cfg->gradient);`
3. Bind `resolution` (vec2 from `GetScreenWidth/Height`).
4. Bind `fftTexture` (via `SetShaderValueTexture`).
5. Bind `sampleRate` (from `AUDIO_SAMPLE_RATE`).
6. Bind `frame` (incremented int counter).
7. Bind all per-frame phase uniforms: `cutPulsePhase`, `orbitPhase`, `fieldOffset`.
8. Bind all config uniforms via a static `BindUniforms` helper.
9. Bind `gradientLUT` texture.

#### Init / Uninit

- `Init`: `LoadShader`, get all uniform locs, `ColorLUTInit(&cfg->gradient)` (unload shader on LUT failure), zero all phase accumulators and `frame`.
- `Uninit`: `UnloadShader`, `ColorLUTUninit`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| enabled | bool | - | false | no | (toggle) |
| **Geometry** | | | | | |
| clusterRadius | float | 5.0-25.0 | 20.0 | yes | "Cluster Radius" |
| cubeSize | float | 0.3-0.7 | 0.632 | yes | "Cube Size" |
| colorRate | float | 0.02-0.5 | 0.1 | yes | "Color Rate" |
| **Cut Field** | | | | | |
| cutMode | int (combo) | {Gyroid, Noise} | 0 (Gyroid) | no | "Cut Mode" |
| cutScale | float | 0.02-0.25 | 0.05 | yes | "Cut Scale" |
| cutThresholdBase | float | -2.0-2.0 | 0.0 | yes | "Threshold" |
| cutThresholdPulse | float | 0.0-2.0 | 0.0 | yes | "Pulse Amount" |
| cutPulseSpeed | float | 0.0-4.0 | 0.5 | yes | "Pulse Speed" |
| fieldDriftX | float | -2.0-2.0 | 0.0 | yes | "Drift X" |
| fieldDriftY | float | -2.0-2.0 | 0.0 | yes | "Drift Y" |
| fieldDriftZ | float | -2.0-2.0 | 0.0 | yes | "Drift Z" |
| **Camera** | | | | | |
| orbitSpeed | float | -1.0-1.0 | 0.06 | yes | "Orbit Speed" |
| orbitPitch | float (rad) | -PI to PI (`ROTATION_OFFSET_MAX`) | 2.094 (= 2*PI/3) | yes | "Pitch" |
| cameraDistance | float | 30.0-80.0 | 50.0 | yes | "Distance" |
| **Lighting** | | | | | |
| ambient | float | 0.0-0.5 | 0.1 | yes | "Ambient" |
| specularPower | float | 10.0-1000.0 | 250.0 | yes | "Specular Power" |
| fogDistance | float | 10.0-100.0 | 25.0 | yes | "Fog Distance" |
| **Audio** | | | | | |
| baseFreq | float | 27.5-440 | 55.0 | yes | "Base Freq (Hz)" |
| maxFreq | float | 1000-16000 | 14000.0 | yes | "Max Freq (Hz)" |
| gain | float | 0.1-10 | 2.0 | yes | "Gain" |
| curve | float | 0.1-3 | 1.5 | yes | "Contrast" |
| baseBright | float | 0.0-1.0 | 0.15 | yes | "Base Bright" |
| **Output** | | | | | |
| gradient | ColorConfig | - | gradient mode | (gradient widget) | (handled by `STANDARD_GENERATOR_OUTPUT`) |
| blendIntensity | float | 0.0-1.0 | 1.0 | yes | "Blend Intensity" |
| blendMode | EffectBlendMode | - | EFFECT_BLEND_ADD | no | "Blend Mode" |

#### UI section ordering (Signal Stack)

In `DrawGeodeParams`, use `SeparatorText` dividers in this order:

1. `"Audio"` — baseFreq, maxFreq, gain, curve (label "Contrast"), baseBright
2. `"Geometry"` — clusterRadius, cubeSize, colorRate
3. `"Cut Field"` — cutMode (Combo), cutScale, cutThresholdBase, cutThresholdPulse, cutPulseSpeed, fieldDriftX, fieldDriftY, fieldDriftZ
4. `"Camera"` — orbitSpeed, orbitPitch, cameraDistance
5. `"Lighting"` — ambient, specularPower, fogDistance

Then `STANDARD_GENERATOR_OUTPUT(geode)` emits the Color/Output section.

Use `ModulatableSlider` for plain floats, `ModulatableSliderAngleDeg` for `orbitPitch` (angle field, displays degrees, stores radians) with bounds `-ROTATION_OFFSET_MAX` to `+ROTATION_OFFSET_MAX` (defined in `src/config/constants.h`). `cutMode` is `ImGui::Combo` with `"Gyroid\0Noise\0"`. Do not use `ModulatableSliderLog`.

### Constants

- Enum value: `TRANSFORM_GEODE_BLEND` (added to `TransformEffectType` in `src/config/effect_config.h`, before `TRANSFORM_ACCUM_COMPOSITE`).
- Display name: `"Geode"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `13` (Field, per `docs/conventions.md`)
- Field name: `geode` on both `EffectConfig` and `PostEffect`
- Param prefix: `"geode."`

### Registration macro

Use `REGISTER_GENERATOR` (single-pass; no resize; `Init(Effect*, const Config*)`):

```cpp
// clang-format off
STANDARD_GENERATOR_OUTPUT(geode)
REGISTER_GENERATOR(TRANSFORM_GEODE_BLEND, Geode, geode, "Geode",
                   SetupGeodeBlend, SetupGeode, 13,
                   DrawGeodeParams, DrawOutput_geode)
// clang-format on
```

`SetupGeode` calls `GeodeEffectSetup(&pe->geode, &pe->effects.geode, pe->currentDeltaTime, pe->fftTexture)`. `SetupGeodeBlend` calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.geode.blendIntensity, pe->effects.geode.blendMode)`. Pattern follows `voxel_march.cpp:174-183`.

---

## Tasks

### Wave 1: Header (creates types other files depend on)

#### Task 1.1: Create `src/effects/geode.h`

**Files**: `src/effects/geode.h`
**Creates**: `GeodeConfig`, `GeodeEffect` types; `GEODE_CONFIG_FIELDS` macro; public function declarations.

**Do**:
- Follow the exact field layout, `GEODE_CONFIG_FIELDS` macro, `GeodeEffect` struct (including all uniform locs and CPU phase fields), and function signatures from the Design > Types section above.
- Include guards `GEODE_H`.
- Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`.
- Forward-declare `typedef struct ColorLUT ColorLUT;` (matches `voxel_march.h:64`).
- Inline range comments on every field (use the table in Design > Parameters; the comment text is the range, not a description of behavior).

**Verify**: `cmake.exe --build build` compiles. The file alone won't be referenced yet — header builds clean on its own once Wave 2 wiring lands.

---

### Wave 2: Implementation, shader, integration (parallel — no file overlap)

#### Task 2.1: Create `src/effects/geode.cpp`

**Files**: `src/effects/geode.cpp`
**Depends on**: Wave 1 complete.

**Do**:
- Implement `GeodeEffectInit`, a static `BindUniforms`, `GeodeEffectSetup`, `GeodeEffectUninit`, `GeodeRegisterParams`, the bridge functions `SetupGeode` and `SetupGeodeBlend`, the colocated `DrawGeodeParams`, the `STANDARD_GENERATOR_OUTPUT(geode)` macro expansion, and the `REGISTER_GENERATOR` macro at the bottom.
- Pattern: mirror `src/effects/voxel_march.cpp` line-by-line. Differences from voxel_march:
  - No `DualLissajousConfig` / no `pan` uniform / no `DualLissajousUpdate` call.
  - Phase accumulation: 5 fields (`cutPulsePhase`, `orbitPhase`, `fieldOffsetX/Y/Z`) plus the `frame` int counter.
  - `Setup` binds `fieldOffset` as a single `vec3` uniform (`{fieldOffsetX, fieldOffsetY, fieldOffsetZ}`).
  - `Setup` increments `e->frame` and binds it as `SHADER_UNIFORM_INT`.
  - `cutMode` binds as `SHADER_UNIFORM_INT`.
  - Section index `13`, badge auto-set by macro.
- `RegisterParams` registers every float marked Modulatable in the Design > Parameters table, plus `blendIntensity`. Use the bounds shown there. Do NOT register `cutMode` (int), `enabled` (bool), `blendMode` (enum).
- UI: implement `DrawGeodeParams` in the canonical Signal Stack order (Audio, Geometry, Cut Field, Camera, Lighting). Use `ImGui::SeparatorText` for section dividers. Use `ImGui::Combo("Cut Mode##geode", &cfg->cutMode, "Gyroid\0Noise\0")`. Use `ModulatableSliderAngleDeg("Pitch##geode", &cfg->orbitPitch, "geode.orbitPitch", -ROTATION_OFFSET_MAX, +ROTATION_OFFSET_MAX, modSources)` for `orbitPitch` per project angle convention. Include `"ui/ui_units.h"` for the angle slider.
- Includes (alphabetical within groups): `"geode.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.
- Bridge functions `SetupGeode` and `SetupGeodeBlend` are NOT static — the registration macro forward-declares them.

**Verify**: Compiles. Effect appears in the Field section of the Effects panel with badge "GEN".

---

#### Task 2.2: Create `shaders/geode.fs`

**Files**: `shaders/geode.fs`
**Depends on**: nothing structural — independent of Wave 1.

**Do**:
- Transcribe the Algorithm > Shader file block in Design verbatim. Do not paraphrase, reorder, or "clean up" the helpers — the duplicate-`nearest`-shadowing pattern in `traverse_voxels` and the `margin = 0.001` constant are deliberate.
- Begin with the attribution comment block exactly as shown.
- Use `#version 330`.
- Both `normal` overloads are present per the research's "keep available for completeness" instruction.

**Verify**: Compiles via `cmake.exe --build build` (raylib loads .fs at runtime, so shader compile errors will only surface when the effect first initializes — visually verify by enabling the effect after the build).

---

#### Task 2.3: Wire into `src/config/effect_config.h`

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete.

**Do**:
- Add `#include "effects/geode.h"` in alphabetical position (between `galaxy.h` and `glitch.h`).
- Add `TRANSFORM_GEODE_BLEND,` to the `TransformEffectType` enum. Place it adjacent to other section-13 generators (e.g., between `TRANSFORM_GALAXY_BLEND` and `TRANSFORM_LASER_DANCE_BLEND`) — order inside the enum is for grouping legibility; numeric value doesn't matter. Must be before `TRANSFORM_EFFECT_COUNT`.
- Add the member `GeodeConfig geode;` to `EffectConfig` near the other section-13 generators (e.g., right after `GalaxyConfig galaxy;`). Add a one-line comment `// Geode (DDA voxel cluster carved by gyroid/noise field)` above it, matching the existing pattern.

**Verify**: Compiles. Note: the `TransformOrderConfig` array is sized to `TRANSFORM_EFFECT_COUNT` and auto-populated by the constructor loop in `effect_config.h:355-360`, so no manual `order[]` initialization edit is needed — adding the enum entry is sufficient.

---

#### Task 2.4: Wire into `src/render/post_effect.h`

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete.

**Do**:
- Add `#include "effects/geode.h"` in alphabetical position (between `galaxy.h` and `glitch.h`).
- Add the member `GeodeEffect geode;` to the `PostEffect` struct near other section-13 generator effects (e.g., adjacent to `GalaxyEffect galaxy;`).

**Verify**: Compiles.

---

#### Task 2.5: Wire into `src/config/effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete.

**Do**:
- Add `#include "effects/geode.h"` in alphabetical position (between `galaxy.h` and `glitch.h`).
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GeodeConfig, GEODE_CONFIG_FIELDS)` near the `G` cluster (e.g., right after `GalaxyConfig`).
- Add `X(geode)` to the `EFFECT_CONFIG_FIELDS(X)` macro — append on its own continuation line near the bottom of the macro, following the existing pattern (each effect on a continuation line).

**Verify**: Compiles. Round-trip a preset save/load through the UI to confirm field serialization (manual smoke test only — no unit test scaffolding).

---

## Final Verification

- [ ] Build succeeds with no warnings.
- [ ] Effect appears in the **Field** section (section 13) of the Effects panel with badge **GEN**.
- [ ] Toggling `enabled` adds the effect to the pipeline list.
- [ ] All sliders modify the effect in real time.
- [ ] `Cut Mode` combo switches between Gyroid and Noise without re-init (default `cutScale = 0.05` matches the reference's gyroid density; user adjusts after switching to Noise per research note).
- [ ] Self-shadowing visible: when `ambient` is low, occluded sides of the cluster go dark.
- [ ] `cubeSize` near 0.632 produces seamless rock; below ~0.5 reveals inter-cube gaps; above ~0.65 begins SDF self-intersection (per research notes).
- [ ] FFT reactivity: silent audio renders at `baseBright` floor; loud audio modulates radial bands (low freq at center, high freq at outer shell).
- [ ] Modulation routes attach to all registered params (`geode.cubeSize`, `geode.cutThresholdBase`, etc.).
- [ ] Preset save/load preserves all fields including the `gradient` ColorConfig.
- [ ] No `tanh` or `sqrt` post-processing in the shader (linear HDR output).
- [ ] No tonemap, no Reinhard, no other compression of the output.
- [ ] No mouse-based camera control.
