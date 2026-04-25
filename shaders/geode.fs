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

uniform int cutMode;
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

float gyroid(vec3 p) {
  return dot(sin(p.xyz * TAU), cos(p.yzx * TAU));
}

float box(vec3 p, vec3 s) {
  vec3 d = abs(p) - s;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

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

vec3 sgn(vec3 v) { return step(vec3(0), v) * 2.0 - 1.0; }

float maxcomp(vec3 v) { return max(max(v.x, v.y), v.z); }

struct result {
  vec3 color;
  float dist;
};

result combine(result a, result b) {
  if (a.dist < b.dist) return a;
  return b;
}

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
