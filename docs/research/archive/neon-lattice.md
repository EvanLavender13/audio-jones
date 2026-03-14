# Neon Lattice

A raymarched infinite lattice of torus shapes with neon glow — flying through a vast repeating digital infrastructure. Columns of tiny rings tile space along configurable axes (1-3), each scrolling at hashed speeds. Soft light falloff reveals the structure as luminous streaks against black void, like being inside a network backbone.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator pass (blend compositor)
- **Section Index**: 10 (Geometric)

## Attribution

- **Based on**: "Inside the System" by kishimisu (2022)
- **Source**: https://www.shadertoy.com/view/msj3D3
- **License**: CC BY-NC-SA 4.0
- **Inspiration cited by author**: https://www.shadertoy.com/view/3dlcWl (neon color intensity falloff)

## References

- kishimisu's shader (above) — complete implementation of torus lattice with domain repetition, neon glow, and orbital camera

## Reference Code

```glsl
/* "Inside the System" by @kishimisu (2022) - https://www.shadertoy.com/view/msj3D3

   This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en)

   3 torus, 3 lights, infinite domain repetition.
   Mouse interactive.

   I got inspired to create this after seeing
   https://www.shadertoy.com/view/3dlcWl which
   plays with similar neon colors. The key equation that
   allow this intensity fallout is 1./(1. + pow(abs(d), n))
*/

#define LOW_PERF      0   // set to 1 for better performances

// spacing controls
#define spacing       7.  // columns repetition spacing
#define light_spacing 2.  // light   repetition spacing (try 1. for a psychedelic effect!)

#define attenuation  22.  // light   attenuation

// speed controls
#define GLOBAL_SPEED  .7
#define camera_speed  1.
#define lights_speed 30.
#define columns_speed 4.

#if LOW_PERF
    #define iterations 30.
    #define max_dist   30.
#else
    #define iterations 50.
    #define max_dist   80.
#endif

#define epsilon 0.005
#define iTime (iTime*GLOBAL_SPEED)

#define rot(a) mat2(cos(a), -sin(a), sin(a), cos(a))
#define rep(p, r) (mod(p+r/2., r)-r/2.)
#define torus(p) (length( vec2(length(p.xz)-.6,p.y) ) - .06)

float hash12(vec2 p) {
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 getLight(vec3 p, vec3 color) {
    return max(vec3(0.), color / (1. + pow(abs(torus(p) * attenuation), 1.3)) - .001);
}

vec3 geo(vec3 po, inout float d, inout vec2 f) {
    // shape repetition
    float r = hash12(floor(po.yz/spacing+vec2(.5)))-.5;
    vec3  p = rep(po + vec3(iTime*r*columns_speed, 0., 0.), vec3(.5, spacing, spacing));
    p.xy   *= rot(1.57);
    d       = min(d, torus(p));

    // light repetition
    f       = floor(po.yz/(spacing*light_spacing)-vec2(.5));
    r       = hash12(f)-.5;
    if (r > -.45) p = rep(po + vec3(iTime*lights_speed*r, 0., 0.), spacing*light_spacing*vec3(r+.54, 1., 1.));
    else p  = rep(po + vec3(iTime*lights_speed*.5*(1.+r*0.003*hash12(floor(po.yz*spacing))), 0., 0.), spacing*light_spacing);
    p.xy   *= rot(1.57);
    f       = (cos(f.xy)*.5+.5)*.4;

    return p;
}

vec4 map(vec3 p) {
    float d = 1e6;
    vec3 po, col = vec3(0.);
    vec2 f;

    po = geo(p, d, f);
    col  += getLight(po, vec3(1., f));        // x

    p.z  += spacing/2.;
    p.xy *= rot(1.57);
    po    = geo(p, d, f);
    col  += getLight(po, vec3(f.x, .5, f.y)); // y

    p.xy += spacing/2.;
    p.xz *= rot(1.57);
    po    = geo(p, d, f);
    col  += getLight(po, vec3(f, 1.));        // z

    return vec4(col, d);
}

vec3 getOrigin(float t) {
    t = (t+35.)*-.05*camera_speed;
    float rad = mix(50., 80., cos(t*1.24)*.5+.5);
    return vec3(rad*sin(t*.97), rad*cos(t*1.11), rad*sin(t*1.27));
}

void initRayOriginAndDirection(vec2 uv, inout vec3 ro, inout vec3 rd) {
    vec2 m = iMouse.xy/iResolution.xy*2.-1.;

    ro = getOrigin(iTime+m.x*10.);

    vec3 f = normalize(getOrigin(iTime+m.x*10.+.5) - ro);
    vec3 r = normalize(cross(normalize(ro), f));
    rd = normalize(f + uv.x*r + uv.y*cross(f, r));
}

void mainImage(out vec4 O, in vec2 F) {
    vec2 uv = (2.*F - iResolution.xy)/iResolution.y;
    vec3 p, ro, rd, col;

    initRayOriginAndDirection(uv, ro, rd);

    float t = 2.;
    for (float i = 0.; i < iterations; i++) {
        p = ro + t*rd;

        vec4 res = map(p);
        col += res.rgb;
        t += abs(res.w);

        if (abs(res.w) < epsilon) t += epsilon;

        if (col.r >= 1. && col.g >= 1. && col.b >= 1.) break;
        if (t > max_dist) break;
    }

    col = pow(col, vec3(.45));
    O = vec4(col, 1.0);
}
```

## Algorithm

### What to keep verbatim from reference code
- `hash12()` — spatial hash function
- `rot(a)` — 2D rotation macro
- `rep(p, r)` — domain repetition macro
- `torus(p)` — torus SDF
- `getLight()` — neon glow falloff: `color / (1. + pow(abs(torus(p) * attenuation), 1.3))`
- `geo()` — shape and light repetition with hashed per-cell speed variation
- `map()` — multi-axis scene composition (but make axis count configurable)
- `getOrigin()` — orbital camera path with Lissajous-like ratios
- `initRayOriginAndDirection()` — ray setup from camera origin
- Main raymarch loop structure with early-out on saturation and max distance

### What to replace
| Reference | Replacement | Reason |
|-----------|-------------|--------|
| `iTime` | `time` uniform (CPU-accumulated) | Speed is accumulated on CPU per convention |
| `iResolution` | `resolution` uniform | Standard uniform name |
| `iMouse` | Remove — no mouse input | Generator has no mouse interaction |
| `#define` constants | Uniforms bound from config struct | All tunable params become modulatable |
| `#if LOW_PERF` branch | Single `iterations` and `maxDist` uniform | Quality controlled by config |
| Per-axis hardcoded RGB in `map()` | `gradientLUT` sampling at axis-dependent offsets (0.0, 0.33, 0.66) | Gradient LUT for user palette control |
| `pow(col, vec3(.45))` gamma | Keep as tonemap; attenuation uniform controls brightness | Standard gamma correction |
| 3-axis hardcoded in `map()` | `axisCount` uniform (int, 1-3) controls how many geo() calls execute | Configurable density |
| `spacing` define (7.0) | `spacing` uniform | Modulatable grid density |
| `light_spacing` define (2.0) | `lightSpacing` uniform | Modulatable light density |
| `attenuation` define (22.0) | `attenuation` uniform | Modulatable glow tightness |
| `columns_speed` define (4.0) | `columnsSpeed` uniform (CPU-accumulated phase) | Speed layer 2 |
| `lights_speed` define (30.0) | `lightsSpeed` uniform (CPU-accumulated phase) | Speed layer 3 |
| `camera_speed` define (1.0) | `cameraSpeed` uniform (CPU-accumulated phase) | Speed layer 1 |
| Camera radius `mix(50., 80., ...)` | `orbitRadius` + `orbitVariation` uniforms | Modulatable camera distance |
| Camera Lissajous ratios (0.97, 1.11, 1.27) | `orbitRatioX/Y/Z` config fields | Tunable camera path shape |
| `iterations` define (50) | `iterations` uniform (int) | Quality/performance tradeoff |
| `max_dist` define (80) | `maxDist` uniform | Ray termination distance |

### Speed accumulation (CPU side)
Three independent phase accumulators, all on CPU:
- `cameraPhase += cameraSpeed * deltaTime`
- `columnsPhase += columnsSpeed * deltaTime`
- `lightsPhase += lightsSpeed * deltaTime`

Pass as `cameraTime`, `columnsTime`, `lightsTime` uniforms. The shader uses these directly instead of `iTime * speed`.

### Axis count logic in shader
```glsl
// Always compute axis 1
po = geo(p, d, f, columnsTime, lightsTime);
col += getLight(po, texture(gradientLUT, vec2(0.0, 0.5)).rgb);

if (axisCount >= 2) {
    // Rotate into axis 2
    p.z  += spacing / 2.0;
    p.xy *= rot(1.5707963);
    po    = geo(p, d, f, columnsTime, lightsTime);
    col  += getLight(po, texture(gradientLUT, vec2(0.33, 0.5)).rgb);
}

if (axisCount >= 3) {
    // Rotate into axis 3
    p.xy += spacing / 2.0;
    p.xz *= rot(1.5707963);
    po    = geo(p, d, f, columnsTime, lightsTime);
    col  += getLight(po, texture(gradientLUT, vec2(0.66, 0.5)).rgb);
}
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| axisCount | int | 1-3 | 3 | Number of orthogonal torus column axes |
| spacing | float | 2.0-20.0 | 7.0 | Grid cell size — distance between columns |
| lightSpacing | float | 0.5-5.0 | 2.0 | Light repetition multiplier (1.0 = psychedelic) |
| attenuation | float | 5.0-60.0 | 22.0 | Glow tightness — higher = sharper falloff |
| glowExponent | float | 0.5-3.0 | 1.3 | Glow curve shape — higher = more concentrated |
| cameraSpeed | float | 0.0-3.0 | 0.7 | Orbital camera drift rate |
| columnsSpeed | float | 0.0-15.0 | 4.0 | Column scroll speed (medium layer) |
| lightsSpeed | float | 0.0-60.0 | 21.0 | Light streak speed (fast layer) |
| orbitRadius | float | 20.0-120.0 | 65.0 | Camera distance from origin |
| orbitVariation | float | 0.0-40.0 | 15.0 | Camera radius oscillation amplitude |
| orbitRatioX | float | 0.5-2.0 | 0.97 | Camera X-axis Lissajous ratio |
| orbitRatioY | float | 0.5-2.0 | 1.11 | Camera Y-axis Lissajous ratio |
| orbitRatioZ | float | 0.5-2.0 | 1.27 | Camera Z-axis Lissajous ratio |
| iterations | int | 20-80 | 50 | Raymarch step count (quality vs performance) |
| maxDist | float | 20.0-120.0 | 80.0 | Ray termination distance |
| torusRadius | float | 0.2-1.5 | 0.6 | Torus major radius |
| torusTube | float | 0.02-0.2 | 0.06 | Torus tube thickness |

## Modulation Candidates

- **spacing**: Grid density — lower values pack columns tighter, higher values open up vast corridors
- **attenuation**: Glow reach — low values flood the scene with light, high values isolate each ring
- **glowExponent**: Glow shape — modulating shifts between soft halos and laser-sharp lines
- **cameraSpeed**: Camera drift — slowing to a crawl for ambient sections, speeding through choruses
- **columnsSpeed**: Column scroll — the mid-layer motion that gives depth perception
- **lightsSpeed**: Light streak speed — the fastest layer, creates urgency and energy
- **orbitRadius**: Camera closeness — pulling in tight for claustrophobic density, pushing out for grand scale
- **torusRadius**: Ring size relative to grid — larger rings fill more of each cell
- **axisCount**: Structure complexity — dropping axes strips the lattice down to corridors

### Interaction Patterns

**Cascading threshold — attenuation gates visible density**: At high attenuation only the nearest rings glow. As attenuation drops (modulated by bass energy), distant rings become visible, making the lattice appear to multiply and deepen. Low-energy passages show sparse floating rings; high-energy sections reveal the full infinite grid.

**Competing forces — spacing vs orbitRadius**: Spacing controls how tightly packed the columns are; orbitRadius controls how far the camera sits. When spacing shrinks (dense grid) but orbitRadius grows (camera pulls back), you see a vast dense field from afar. When spacing grows but orbitRadius shrinks, you're close-up in a sparse open structure. Modulating them in opposition creates a breathing scale shift.

**Resonance — columnsSpeed + lightsSpeed alignment**: Columns and lights scroll at different rates. When both spike simultaneously (e.g., both mapped to transient energy), the lights appear to lock to the columns momentarily — a brief "sync" that breaks apart as they decouple. This creates rare coherent moments in otherwise chaotic motion.

## Notes

- **Performance**: At 50 iterations this is moderately expensive. The `iterations` and `maxDist` params let users dial quality vs framerate. Consider `EFFECT_FLAG_HALF_RES` if full-res is too heavy.
- **Early saturation exit**: The original breaks when all RGB channels >= 1.0. Keep this — it's a significant performance win in bright scenes.
- **Torus tube thickness**: Very small `torusTube` values make rings nearly invisible except at close range. Clamp minimum to 0.02.
- **Camera stability**: The Lissajous orbital path never passes through the origin, so no singularity risk. The `normalize(ro)` in the up-vector calculation is safe as long as `orbitRadius > 0`.
- **Shader coordinate convention**: The reference code's UV setup `(2.*F - iResolution.xy)/iResolution.y` already centers coordinates. This is a fullscreen raymarcher — the project's `fragTexCoord` centering convention doesn't apply since the ray direction handles all spatial positioning.
