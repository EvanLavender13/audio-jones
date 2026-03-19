# Polyhedral Mirror

Raymarched reflective polyhedron viewed from inside. The camera orbits within a mirrored platonic solid, bouncing light off crystalline faces that color-shift by reflection depth via gradient LUT. Glowing wireframe edges trace the geometry. Configurable shape (all 5 platonic solids), reflection depth, camera distance, and rotation speeds. Audio reactivity is modulation-driven (no FFT in shader).

**Research**: `docs/research/polyhedral_mirror.md`

## Design

### Types

```c
// polyhedral_mirror.h

struct PolyhedralMirrorConfig {
  bool enabled = false;

  // Shape
  int shape = 4; // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa

  // Camera
  float orbitSpeedXZ = 0.5f;   // XZ orbit rate rad/s (-PI..+PI)
  float orbitSpeedYZ = 0.2f;   // YZ orbit rate rad/s (-PI..+PI)
  float cameraDistance = 0.4f; // Origin to camera (0.1-0.8)

  // Reflection
  float maxBounces = 5.0f;    // Reflection bounces (1-8), float for modulation
  float reflectivity = 0.6f;  // Per-bounce color mix factor (0.1-1.0)

  // Geometry
  float edgeRadius = 0.05f;   // Wireframe capsule radius (0.01-0.15)
  float edgeGlow = 1.0f;      // Edge brightness multiplier (0.0-2.0)
  int maxIterations = 80;     // Raymarch step limit (32-128)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define POLYHEDRAL_MIRROR_CONFIG_FIELDS                                        \
  enabled, shape, orbitSpeedXZ, orbitSpeedYZ, cameraDistance, maxBounces,       \
      reflectivity, edgeRadius, edgeGlow, maxIterations, gradient, blendMode,   \
      blendIntensity

typedef struct PolyhedralMirrorEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated rotation angles
  float angleXZAccum;
  float angleYZAccum;

  // Uniform locations
  int resolutionLoc;
  int faceNormalsLoc;
  int faceCountLoc;
  int planeOffsetLoc;
  int edgeALoc;
  int edgeBLoc;
  int edgeCountLoc;
  int angleXZLoc;
  int angleYZLoc;
  int cameraDistanceLoc;
  int maxBouncesLoc;
  int reflectivityLoc;
  int edgeRadiusLoc;
  int edgeGlowLoc;
  int maxIterationsLoc;
  int gradientLUTLoc;
} PolyhedralMirrorEffect;
```

### Algorithm

#### Shader (`shaders/polyhedral_mirror.fs`)

Attribution header (before `#version`):
```glsl
// Based on "Icosahedral Party" by OldEclipse
// https://www.shadertoy.com/view/33tSzs
// License: CC BY-NC-SA 3.0 Unported
// Modified: All 5 platonic solids via uniforms, IQ sdCapsule, gradient LUT
// coloring, configurable reflections and camera
```

Uniforms:
```glsl
uniform vec2 resolution;
uniform vec3 faceNormals[20];
uniform int faceCount;
uniform float planeOffset;
uniform vec3 edgeA[30];
uniform vec3 edgeB[30];
uniform int edgeCount;
uniform float angleXZ;
uniform float angleYZ;
uniform float cameraDistance;
uniform int maxBounces;
uniform float reflectivity;
uniform float edgeRadius;
uniform float edgeGlow;
uniform int maxIterations;
uniform sampler2D gradientLUT;
```

Helper functions:
```glsl
// Reference: Rot(th) — kept verbatim
mat2 Rot(float th) {
    float c = cos(th);
    float s = sin(th);
    return mat2(vec2(c, s), vec2(-s, c));
}

// Reference: sdPlane — kept verbatim
float sdPlane(vec3 p, vec3 n, float h) {
    return dot(p, n) + h;
}

// IQ sdCapsule (replaces reference sdCylinder)
// https://iquilezles.org/articles/distfunctions/
float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a;
    vec3 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}
```

Distance map — face planes from uniform normals, edges from sdCapsule:
```glsl
vec2 map(vec3 p) {
    float d = 1e6;
    float type = 0.0;

    for (int i = 0; i < faceCount; i++) {
        d = min(d, sdPlane(p, faceNormals[i], planeOffset));
    }

    float dCap = 1e6;
    for (int i = 0; i < edgeCount; i++) {
        dCap = min(dCap, sdCapsule(p, edgeA[i], edgeB[i], edgeRadius));
    }
    if (dCap < d) {
        d = dCap;
        type = 2.0;
    }

    return vec2(d, type);
}
```

Normal estimation — kept from reference (IQ finite-difference):
```glsl
vec3 getNormal(vec3 p) {
    float d = map(p).x;
    vec2 e = vec2(0.0001, 0.0);
    vec3 n = d - vec3(
        map(p - e.xyy).x,
        map(p - e.yxy).x,
        map(p - e.yyx).x
    );
    return normalize(n);
}
```

Main function — apply all substitutions from research:
<!-- Research says "2 angles replace 4 speeds" but doesn't specify how target
     rotation derives from origin rotation. These ratios preserve the reference's
     relative drift (0.4/0.5=0.8 for XZ, 0.3/0.2=1.5 for YZ). The coupling is
     fixed in the shader so the visual character is preserved regardless of
     which speed values the user or modulation engine sets. -->
```glsl
// Camera-to-target rotation ratios (reference speeds 0.4/0.5, 0.3/0.2)
const float TARGET_XZ_RATIO = 0.8;
const float TARGET_YZ_RATIO = 1.5;

void main() {
    vec2 uv = (fragTexCoord * 2.0 - 1.0) * vec2(resolution.x / resolution.y, 1.0);

    // Camera rotation from 2 accumulated angles (replaces 4 hardcoded speeds)
    mat2 R = Rot(angleXZ);
    mat2 R2 = Rot(angleYZ);
    vec3 ro = vec3(0.0, 0.0, -cameraDistance);
    ro.xz = R * ro.xz;
    ro.yz = R2 * ro.yz;

    mat2 R3 = Rot(angleXZ * TARGET_XZ_RATIO);
    mat2 R4 = Rot(angleYZ * TARGET_YZ_RATIO);
    vec3 ta = ro + vec3(1.2 * uv, 1.0);
    ta.xz = R3 * ta.xz;
    ta.yz = R4 * ta.yz;
    vec3 rd = normalize(ta - ro);

    vec3 lightPos = vec3(0.0, 0.4, 0.0);

    float minDist = 0.001;
    float t = 0.0;
    int reflCount = 0;
    vec3 col = vec3(0.0);
    vec3 prevCol = col;

    for (int i = 0; i < maxIterations; i++) {
        vec3 pos = ro + t * rd;
        vec2 m = map(pos);
        float d = m.x;
        float type = m.y;

        if (d < minDist) {
            vec3 n = getNormal(pos);

            // Edge hit: gradient LUT at fixed position for contrast, apply edgeGlow
            if (type == 2.0) {
                col += texture(gradientLUT, vec2(0.0, 0.5)).rgb * edgeGlow;
                if (reflCount > 0) {
                    float p = pow(reflectivity, float(reflCount));
                    col = mix(prevCol, col, p);
                }
                break;
            }

            // Face hit: color by reflection depth via gradient LUT
            // Research says reflCount/maxBounces, but reflCount ranges 0..maxBounces-1
            // (break fires after increment), so maxBounces-1 maps the full gradient 0..1
            float gradT = float(reflCount) / max(float(maxBounces) - 1.0, 1.0);
            col = texture(gradientLUT, vec2(gradT, 0.5)).rgb;

            // Diffuse lighting (kept from reference)
            vec3 l = normalize(lightPos - pos);
            float diffuse = max(dot(n, l), 0.0);
            col = col * (0.5 + 0.5 * diffuse);

            // Reflection color mixing (kept from reference)
            if (reflCount > 0) {
                float p = pow(reflectivity, float(reflCount));
                col = mix(prevCol, col, p);
            }

            prevCol = col;

            if (reflCount >= maxBounces) {
                break;
            }

            // Reflected ray (kept from reference)
            ro = pos;
            rd = reflect(rd, n);
            t = 0.01;
            reflCount++;
            continue;
        }

        t += d;
    }

    // Reference: col *= col (tonemap) — REMOVED, gradient LUT handles coloring
    finalColor = vec4(col, 1.0);
}
```

#### Face Normal Derivation (CPU)

Face normals of a platonic solid are the vertex positions of its dual solid (both normalized to unit sphere):

| Solid (idx) | Dual (idx) | Face Count |
|-------------|------------|------------|
| Tetrahedron (0) | Tetrahedron (0) | 4 |
| Cube (1) | Octahedron (2) | 6 |
| Octahedron (2) | Cube (1) | 8 |
| Dodecahedron (3) | Icosahedron (4) | 12 |
| Icosahedron (4) | Dodecahedron (3) | 20 |

```c
static const int DUAL_IDX[] = {0, 2, 1, 4, 3};
```

Upload `SHAPES[DUAL_IDX[shapeIdx]].vertices` as `faceNormals[]` and `SHAPES[DUAL_IDX[shapeIdx]].vertexCount` as `faceCount`.

#### Plane Offset (CPU)

Inradius ratio for unit-sphere-inscribed solids — distance from center to face:

```c
static const float PLANE_OFFSETS[] = {0.3333f, 0.5774f, 0.5774f, 0.7946f, 0.7946f};
```

Upload `PLANE_OFFSETS[shapeIdx]` as `planeOffset`.

#### Edge Endpoints (CPU)

Pre-resolve edge indices to 3D positions from `SHAPES[shapeIdx]`. Use unit-sphere vertex positions directly as edge endpoints (no scaling). With per-solid `planeOffset`, the SDF polyhedron's circumradius is 1.0 (unit sphere), so vertices sit exactly at the SDF surface. The capsule `edgeRadius` provides visual protrusion.

<!-- Intentional deviation: research says edgeScale = 1/planeOffset, but with correct per-solid planeOffset the SDF circumradius = 1.0, so edges at unit-sphere positions are already at SDF vertices. The 1/planeOffset formula produces incorrect results for non-icosahedron solids (e.g., tetrahedron edges at 3x face distance). -->

#### Phase Accumulation (CPU)

```c
e->angleXZAccum += cfg->orbitSpeedXZ * deltaTime;
e->angleYZAccum += cfg->orbitSpeedYZ * deltaTime;
```

Upload as `angleXZ` and `angleYZ`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| shape | int | 0-4 | 4 | no | Shape |
| orbitSpeedXZ | float | -PI..+PI | 0.5 | yes | Orbit XZ |
| orbitSpeedYZ | float | -PI..+PI | 0.2 | yes | Orbit YZ |
| cameraDistance | float | 0.1-0.8 | 0.4 | yes | Camera Dist |
| maxBounces | float | 1-8 | 5.0 | yes | Max Bounces |
| reflectivity | float | 0.1-1.0 | 0.6 | yes | Reflectivity |
| edgeRadius | float | 0.01-0.15 | 0.05 | yes | Edge Radius |
| edgeGlow | float | 0.0-2.0 | 1.0 | yes | Edge Glow |
| maxIterations | int | 32-128 | 80 | no | Max Iterations |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend |

### Constants

- Enum: `TRANSFORM_POLYHEDRAL_MIRROR_BLEND`
- Display name: `"Polyhedral Mirror"`
- Category: `"GEN"`, section 10 (Geometric)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)
- Macro: `REGISTER_GENERATOR`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Effect header

**Files**: `src/effects/polyhedral_mirror.h` (create)
**Creates**: `PolyhedralMirrorConfig`, `PolyhedralMirrorEffect` types, function declarations

**Do**: Create header with config struct, effect struct, and function declarations exactly as specified in the Design > Types section. Follow `polymorph.h` structure: includes `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Forward-declare `ColorLUT`. Declare `Init(Effect*, const Config*)`, `Setup(Effect*, const Config*, float)` (no fftTexture parameter — this generator has no FFT), `Uninit(Effect*)`, `RegisterParams(Config*)`.

**Verify**: `cmake.exe --build build` compiles (header-only, no link errors expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect implementation

**Files**: `src/effects/polyhedral_mirror.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `polymorph.cpp` as pattern. Implement:
- `PolyhedralMirrorEffectInit`: load `shaders/polyhedral_mirror.fs`, cache all 16 uniform locations listed in the Effect struct, init `ColorLUT` from `cfg->gradient`. On failure, unload shader and return false.
- `PolyhedralMirrorEffectSetup`: Compute face normals from dual solid (`DUAL_IDX`), plane offset (`PLANE_OFFSETS`), and edge endpoints from `SHAPES[shapeIdx]` (unit-sphere vertices, no scaling). Accumulate `angleXZAccum += orbitSpeedXZ * dt` and `angleYZAccum += orbitSpeedYZ * dt`. Convert `maxBounces` float to int for shader upload. Update `ColorLUT`. Bind all uniforms. This function takes `(Effect*, const Config*, float deltaTime)` — no fftTexture (this generator has no FFT).
- `PolyhedralMirrorEffectUninit`: unload shader, uninit ColorLUT.
- `PolyhedralMirrorRegisterParams`: register all 9 modulatable params from the Parameters table. `orbitSpeedXZ`/`orbitSpeedYZ` use `ROTATION_SPEED_MAX` bounds.
- `SetupPolyhedralMirror(PostEffect*)`: bridge function (non-static) calling `PolyhedralMirrorEffectSetup` with `pe->currentDeltaTime`. No fftTexture argument.
- `SetupPolyhedralMirrorBlend(PostEffect*)`: bridge function (non-static) calling `BlendCompositorApply`.
- Colocated UI (`DrawPolyhedralMirrorParams`): Shape section (Shape combo), Camera section (Orbit XZ with `ModulatableSliderSpeedDeg`, Orbit YZ with `ModulatableSliderSpeedDeg`, Camera Dist), Reflection section (Max Bounces with `ModulatableSliderInt`, Reflectivity), Geometry section (Edge Radius with `ModulatableSliderLog`, Edge Glow, Max Iterations with `ImGui::SliderInt`).
- `STANDARD_GENERATOR_OUTPUT(polyhedralMirror)` macro.
- `REGISTER_GENERATOR(TRANSFORM_POLYHEDRAL_MIRROR_BLEND, PolyhedralMirror, polyhedralMirror, "Polyhedral Mirror", SetupPolyhedralMirrorBlend, SetupPolyhedralMirror, 10, DrawPolyhedralMirrorParams, DrawOutput_polyhedralMirror)`.

Includes: `polyhedral_mirror.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `config/platonic_solids.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<math.h>`, `<stddef.h>`.

**Verify**: Compiles.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/polyhedral_mirror.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the shader exactly as specified in the Design > Algorithm > Shader section. Include the attribution header before `#version 330`. Declare all uniforms, helper functions (`Rot`, `sdPlane`, `sdCapsule`), `map()`, `getNormal()`, constants (`TARGET_XZ_RATIO`, `TARGET_YZ_RATIO`), and the `main()` function verbatim from the plan's Algorithm section. Do NOT consult the research doc — the plan's Algorithm section is the single source of truth.

**Verify**: Shader file exists and has correct `#version 330` header.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (all modify)
**Depends on**: Wave 1 complete

**Do**: Four mechanical edits following the Polymorph pattern:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/polyhedral_mirror.h"` with other effect includes (alphabetical)
   - Add `TRANSFORM_POLYHEDRAL_MIRROR_BLEND,` to `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE`
   - Add `TRANSFORM_POLYHEDRAL_MIRROR_BLEND` to `TransformOrderConfig::order` array (auto-populated by loop, no explicit edit needed — verify this)
   - Add `PolyhedralMirrorConfig polyhedralMirror;` to `EffectConfig` struct with comment `// Polyhedral Mirror (raymarched reflective polyhedron interior)`

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/polyhedral_mirror.h"` with other effect includes
   - Add `PolyhedralMirrorEffect polyhedralMirror;` to `PostEffect` struct near the other generator effects (after `polymorph`)

3. **`CMakeLists.txt`**:
   - Add `src/effects/polyhedral_mirror.cpp` to `EFFECTS_SOURCES` list (alphabetical)

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/polyhedral_mirror.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PolyhedralMirrorConfig, POLYHEDRAL_MIRROR_CONFIG_FIELDS)` with other per-config macros
   - Add `X(polyhedralMirror)` to `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

## Implementation Notes

### Dodecahedron vertex reorientation
The original `platonic_solids.h` had dodecahedron non-cube vertices using `(0, ±1/φ, ±φ)` cyclic permutations, but the icosahedron used `(0, ±1/φ, ±φ)` as well. Platonic duality requires the two solids to be co-oriented. The dodecahedron vertices were reoriented to `(0, ±φ, ±1/φ)`, `(±1/φ, 0, ±φ)`, `(±φ, ±1/φ, 0)` with recomputed edges. Cube vertices (0-7) were unchanged. This fix was necessary for face normals derived via duality to be correct for dodecahedron and icosahedron.

### Face coloring via gradient LUT
The plan specified `reflCount / maxBounces` as the gradient coordinate. This produced uniform single-color faces. Replaced with `faceIdx / faceCount` where `faceIdx` is determined by matching the surface normal to the nearest `faceNormals[]` entry. Each face gets a unique gradient position.

### FFT audio reactivity (not in original plan)
Added standard FFT band sampling. Each face's index maps to a frequency band (log-spaced between `baseFreq` and `maxFreq`). The 4-sample band averaging pattern from constellation/spectral_rings is used. FFT brightness multiplies the gradient color per face. Config fields: `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`. Setup function signature changed to accept `Texture2D fftTexture`.

### Edge coloring
Edges are dark by default (first hit has `col = vec3(0)`). Reflected edges inherit accumulated face color scaled by `edgeGlow`. The plan's gradient-sampled edge coloring produced distracting rainbow banding along capsule surfaces.

### Half-count face normal trick removed
The plan suggested uploading half the face normals and using `±n` in the shader. This fails because dodecahedron/icosahedron vertices are not ordered so that indices 0..N/2 form one half of centrosymmetric pairs (e.g., vertex 0's negation is vertex 7, not vertex 10). All face normals are uploaded directly to `faceNormals[20]`.

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Geometric generator section
- [ ] Shape combo switches between all 5 platonic solids
- [ ] Camera orbits smoothly with both XZ and YZ speeds
- [ ] Reflection depth changes with Max Bounces slider
- [ ] Edge wireframe visible and brightness changes with Edge Glow
- [ ] Gradient LUT colors shift by reflection depth
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
