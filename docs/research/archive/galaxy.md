# Galaxy

A single spiral galaxy filling the screen — concentric elliptical rings with progressive rotation create spiral arm structure. Inner rings carry bass energy, outer rings carry treble. Dust detail from value noise, embedded point stars with twinkle and rare supernova flashes, and a Gaussian central bulge. Style params (twist, stretch, ring width) are exposed directly so the shape can range from tight spiral to diffuse elliptical.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator (blend composited)
- **Compute Model**: Single-pass fragment shader

## Attribution

- **Based on**: "Galaxy Generator Study" by guinetik (technique from "Megaparsecs" by Martijn Steinrucken / BigWings)
- **Source**: https://www.shadertoy.com/view/scl3DH
- **License**: CC BY-NC-SA 3.0

## References

- [Galaxy Generator Study](https://www.shadertoy.com/view/scl3DH) - Complete ring-loop galaxy rendering with 5 morphological types
- Megaparsecs by BigWings - Original overlapping rotated elliptical orbits technique

## Reference Code

```glsl
// === Common: noise utilities ===

float hashN2(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

float valueNoise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hashN2(i + vec2(0.0, 0.0)), hashN2(i + vec2(1.0, 0.0)), u.x),
               mix(hashN2(i + vec2(0.0, 1.0)), hashN2(i + vec2(1.0, 1.0)), u.x), u.y);
}

// === Common: galaxy library ===

#define _GAL_TAU 6.2831853

#define GAL_MAX_RADIUS 1.5
#define GAL_MIN_COS_TILT 0.15
#define GAL_RING_PHASE_OFFSET 100.0
#define GAL_ORBIT_SPEED 0.1
#define GAL_DUST_UV_SCALE 0.2
#define GAL_DUST_NOISE_FREQ 4.0
#define GAL_STAR_GLOW_RADIUS 0.5
#define GAL_STAR_BRIGHTNESS 0.2
#define GAL_SUPERNOVA_THRESH 0.9999
#define GAL_SUPERNOVA_MULT 10.0
#define GAL_INNER_RADIUS 0.1
#define GAL_OUTER_RADIUS 1.0
#define GAL_MAX_RINGS 25
#define GAL_RING_DECORR_A 563.2
#define GAL_RING_DECORR_B 673.2
#define GAL_STAR_OFFSET_A 17.3
#define GAL_STAR_OFFSET_B 31.7
#define GAL_TWINKLE_FREQ 784.0
#define GAL_SUPERNOVA_TIME_SCALE 0.05
#define GAL_STAR_COLOR_FREQ 100.0

mat2 _galRot(float a) {
  float s = sin(a), c = cos(a);
  return mat2(c, -s, s, c);
}

struct Galaxy {
  int type;
  uint seed;
  vec2 center;
  float scale;
  float angleX;
  float angleY;
  float angleZ;
  vec3 color;
  float axialRatio;
  float mass_log10;
  float velocity_kmps;
  float distance_mpc;
  float time;
};

struct GalaxyStyle {
  float twist;
  float innerStretch;
  float ringWidth;
  float numRings;
  float diskThickness;
  float bulgeSize;
  float bulgeBright;
  float dustContrast;
  float starDensity;
};

vec2 _galApplyTilt(vec2 uv, float angleX) {
  uv.y /= max(abs(cos(angleX)), GAL_MIN_COS_TILT);
  return uv;
}

vec3 _galRenderBulge(vec2 uv, float size, float brightness, vec3 tint) {
  return vec3(exp(-0.5 * dot(uv, uv) * size)) * brightness * tint;
}

vec3 _galRenderRingLoop(Galaxy g, vec2 uv, GalaxyStyle style) {
  vec3 col = vec3(0.0);
  vec3 dustCol = vec3(0.3, 0.6, 1.0);
  float flip = 1.0;
  float t = g.time * GAL_ORBIT_SPEED;
  t *= (float(g.seed % 2u) * 2.0 - 1.0);

  for (int j = 0; j < GAL_MAX_RINGS; j++) {
    float i = float(j) / style.numRings;
    if (i >= 1.0) break;
    flip *= -1.0;

    float z = mix(style.diskThickness, 0.0, i) * flip * fract(sin(i * GAL_RING_DECORR_A) * GAL_RING_DECORR_B);
    float r = mix(GAL_INNER_RADIUS, GAL_OUTER_RADIUS, i);
    vec2 ringUv = uv + vec2(0.0, z * 0.5);
    vec2 st = ringUv * _galRot(i * _GAL_TAU * style.twist);
    st.x *= mix(style.innerStretch, 1.0, i);
    float ell = exp(-0.5 * abs(dot(st, st) - r) * style.ringWidth);
    vec2 texUv = GAL_DUST_UV_SCALE * st * _galRot(i * GAL_RING_PHASE_OFFSET + t / r);
    vec3 dust = vec3(valueNoise2D((texUv + vec2(i)) * GAL_DUST_NOISE_FREQ));
    vec3 dL = pow(max(ell * dust / r, vec3(0.0)), vec3(0.5 + style.dustContrast));
    col += dL * dustCol;

    // Point Stars
    vec2 starId = floor(texUv * style.starDensity);
    vec2 starUv = fract(texUv * style.starDensity) - 0.5;
    float n = hashN2(starId + vec2(i * GAL_STAR_OFFSET_A, i * GAL_STAR_OFFSET_B));
    float starDist = length(starUv);
    float sL = smoothstep(GAL_STAR_GLOW_RADIUS, 0.0, starDist)
             * pow(max(dL.r, 0.0), 2.0) * GAL_STAR_BRIGHTNESS
             / max(starDist, 0.001);
    float sN = sL;
    sL *= sin(n * GAL_TWINKLE_FREQ + g.time) * 0.5 + 0.5;
    sL += sN * smoothstep(GAL_SUPERNOVA_THRESH, 1.0, sin(n * GAL_TWINKLE_FREQ + g.time * GAL_SUPERNOVA_TIME_SCALE))
        * GAL_SUPERNOVA_MULT;

    if (i > 3.0 / style.starDensity) {
      vec3 starCol = mix(dustCol, vec3(1.0), 0.3 + n * 0.5);
      col += sL * starCol;
    }
  }

  col /= style.numRings;
  return col;
}

// === Spiral renderer (type 0 from reference, used as basis) ===

vec3 renderSpiral(Galaxy g, vec2 fragCoord) {
  vec2 uv = (fragCoord - g.center) / g.scale;
  uv = _galApplyTilt(uv * _galRot(g.angleZ), g.angleX);
  if (length(uv) > GAL_MAX_RADIUS) return vec3(0.0);

  GalaxyStyle s;
  s.twist        = 1.0;
  s.innerStretch = mix(1.8, 2.2, g.axialRatio);
  s.ringWidth    = 15.0;
  s.numRings     = 20.0;
  s.diskThickness = 0.04;
  s.bulgeSize    = 25.0;
  s.bulgeBright  = 1.2;
  s.dustContrast = 0.5;
  s.starDensity  = 8.0;

  vec3 col = _galRenderRingLoop(g, uv, s);
  col += _galRenderBulge(uv, s.bulgeSize, s.bulgeBright,
           mix(vec3(1.0, 0.9, 0.8), g.color, 0.6));
  col *= g.color;
  return col;
}
```

## Algorithm

Adaptation from reference to AudioJones generator:

### Keep verbatim
- `_galRot()` rotation matrix
- `_galApplyTilt()` UV Y-stretch for fake 3D tilt
- `_galRenderBulge()` Gaussian center glow
- Ring-loop structure: progressive rotation, inner stretch, Gaussian ring brightness, dust noise, point stars with twinkle/supernova
- `hashN2()` and `valueNoise2D()` noise functions

### Replace
- **Galaxy struct / GalaxyStyle struct**: replace with uniforms (all style params become uniform floats)
- **`dustCol` hardcoded blue-white**: replace with `gradientLUT` sampling — `texture(gradientLUT, vec2(t, 0.5)).rgb` where `t = float(j) / float(layers)`
- **`g.color` post-multiply tint**: remove, gradientLUT handles all coloring
- **Grid layout / multi-galaxy dispatch**: remove entirely — single galaxy centered at `resolution * 0.5`
- **`g.scale` pixel radius**: replace with `resolution * 0.5` (galaxy fills screen)
- **Per-ring brightness**: add FFT band lookup (spectral_arcs pattern) — `baseBright + mag` multiplied into dust and star brightness per ring
- **`g.time`**: use CPU-accumulated `time` uniform (same as all generators)
- **`g.seed`**: use a `seed` uniform (int) for rotation direction

### FFT integration per ring
Follow spectral_arcs pattern exactly:
```
float t0 = float(j) / float(layers);
float t1 = float(j + 1) / float(layers);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
// ... BAND_SAMPLES=4 averaging ...
float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```
Multiply `brightness` into both `dL` (dust) and `sL` (star glow) for that ring.

### Coordinate convention
Center UV at `resolution * 0.5` per project conventions:
```
vec2 centered = fragTexCoord * resolution - resolution * 0.5;
vec2 uv = centered / (min(resolution.x, resolution.y) * 0.5);
```
This gives uv in [-1, 1] normalized by shortest axis. Galaxy fills to edges.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| layers | int | 8-25 | 20 | Ring count (visual density, FFT band count) |
| twist | float | 0.0-2.0 | 1.0 | Spiral winding per ring. 0=elliptical, 1=classic spiral |
| innerStretch | float | 1.0-4.0 | 2.0 | Inner ring X elongation. 1=circular, 4=strong bar |
| ringWidth | float | 4.0-30.0 | 15.0 | Gaussian sharpness of rings. Low=diffuse, high=crisp bands |
| diskThickness | float | 0.01-0.15 | 0.04 | Ring-to-ring Y perturbation. Higher=thicker, messier disk |
| tilt | float | 0.0-1.57 | 0.3 | Viewing angle (radians). 0=face-on, PI/2=edge-on |
| rotation | float | -PI-PI | 0.0 | In-plane rotation angle |
| orbitSpeed | float | 0.0-1.0 | 0.1 | Orbital animation speed |
| dustContrast | float | 0.1-1.5 | 0.5 | Dust pow() exponent. Low=soft, high=sharp |
| starDensity | float | 2.0-16.0 | 8.0 | Star grid resolution per ring |
| starBright | float | 0.05-1.0 | 0.2 | Star point intensity |
| bulgeSize | float | 5.0-50.0 | 25.0 | Center glow Gaussian tightness. Higher=smaller bulge |
| bulgeBright | float | 0.0-3.0 | 1.2 | Center glow intensity |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Ring brightness when silent |

## Modulation Candidates

- **twist**: spiral arms wind tighter/looser
- **innerStretch**: bar grows/shrinks, changes morphology
- **tilt**: galaxy rocks between face-on and edge-on
- **rotation**: galaxy spins in-plane
- **bulgeSize**: core glow expands/contracts
- **bulgeBright**: core pulses
- **dustContrast**: arms go sharp/soft
- **orbitSpeed**: ring orbital animation accelerates/decelerates
- **starBright**: star field brightens/dims independently of dust

## Notes

- Performance: 20 rings with 4 FFT band samples each = 80 texture lookups + 20 noise evaluations. Comparable to spectral_arcs. Should run fine at full res.
- The reference `GAL_MAX_RADIUS 1.5` early-out is still useful — fragments in screen corners beyond the tilted galaxy radius skip the loop entirely.
- `seed` uniform (int) controls clockwise vs counter-clockwise orbit. Could be a bool or just hardcoded.
- Gamma correction is NOT handled in the shader — the output pipeline's gamma effect handles that.
