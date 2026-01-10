# Poincaré Disk Hyperbolic Tiling

Transform effect that warps texture through hyperbolic space using the Poincaré disk model. Combines Möbius translation with hyperbolic folding to create Escher-like repeating tiles within a circular boundary.

## Current State

Existing transform effects provide the pattern to follow:
- `src/config/mobius_config.h:1-21` - Config struct template
- `src/config/effect_config.h:18-64` - Enum, name function, order array
- `src/render/post_effect.h:32-163` - Shader and uniform location fields
- `src/render/post_effect.cpp:42-145` - Shader load, uniform lookup
- `src/render/shader_setup.cpp:10-36` - Dispatch table and setup functions
- `src/render/render_pipeline.cpp:142-234` - Time accumulation and Lissajous
- `src/automation/param_registry.cpp:65-71` - Modulation param registration
- `src/ui/imgui_effects.cpp:443-472` - UI section pattern
- `src/config/preset.cpp:122-124` - Serialization macro

## Technical Implementation

**Source**: [docs/research/poincare-disk.md](../research/poincare-disk.md)

### Complex Number Helpers

```glsl
vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

vec2 cdiv(vec2 a, vec2 b) {
    return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / dot(b, b);
}

vec2 conj(vec2 z) { return vec2(z.x, -z.y); }
```

### Möbius Translation

Translates viewpoint through hyperbolic space. Points near disk edge compress; center expands.

```glsl
vec2 mobiusTranslate(vec2 v, vec2 z) {
    float vv = dot(v, v);
    float zv = dot(z, v);
    float zz = dot(z, z);
    return ((1.0 + 2.0 * zv + zz) * v + (1.0 - vv) * z) /
           (1.0 + 2.0 * zv + vv * zz);
}
```

### Fundamental Domain Construction

Given angles π/P, π/Q, π/R, compute mirrors A, B and circle C:

```glsl
void solveFundamentalDomain(int P, int Q, int R,
                            out vec2 A, out vec2 B, out vec3 C) {
    A = vec2(1.0, 0.0);
    B = vec2(-cos(PI / float(P)), sin(PI / float(P)));

    mat2 m = inverse(mat2(A, B));
    vec2 cDir = vec2(cos(PI / float(R)), cos(PI / float(Q))) * m;

    float S2 = 1.0 / (dot(cDir, cDir) - 1.0);
    float S = sqrt(S2);
    C = vec3(cDir * S, S2);
}
```

### Hyperbolic Folding

Reflects point into fundamental triangular domain via 50 iterations:

```glsl
vec2 hyperbolicFold(vec2 z, vec2 A, vec2 B, vec3 C) {
    for (int i = 0; i < 50; i++) {
        int folds = 0;

        // Reflect across line A
        float tA = dot(z, A);
        if (tA < 0.0) {
            z -= 2.0 * tA * A;
            folds++;
        }

        // Reflect across line B
        float tB = dot(z, B);
        if (tB < 0.0) {
            z -= 2.0 * tB * B;
            folds++;
        }

        // Invert through circle C (center C.xy, radius² C.z)
        vec2 zc = z - C.xy;
        float d2 = dot(zc, zc);
        if (d2 < C.z) {
            z = C.xy + zc * C.z / d2;
            folds++;
        }

        if (folds == 0) break;
    }
    return z;
}
```

### Full UV Transform

```glsl
vec2 poincareUV(vec2 uv, vec2 translation, float rotation,
                int P, int Q, int R, float diskScale) {
    // Map to disk coordinates centered at origin
    vec2 z = (uv - 0.5) * 2.0 * diskScale;

    // Apply Euclidean rotation (also a hyperbolic isometry)
    float c = cos(rotation), s = sin(rotation);
    z = vec2(c * z.x - s * z.y, s * z.x + c * z.y);

    // Clip to unit disk - return invalid marker for outside
    if (dot(z, z) > 1.0) {
        return vec2(-1.0);
    }

    // Apply Möbius translation
    z = mobiusTranslate(translation, z);

    // Construct and fold into fundamental domain
    vec2 A, B; vec3 C;
    solveFundamentalDomain(P, Q, R, A, B, C);
    z = hyperbolicFold(z, A, B, C);

    // Map back to texture coords
    return z * 0.5 + 0.5;
}
```

### Parameter Summary

| Uniform | Type | Range | Effect |
|---------|------|-------|--------|
| `tileP` | int | 2-12 | Angle at origin vertex (π/P) |
| `tileQ` | int | 2-12 | Angle at second vertex |
| `tileR` | int | 2-12 | Angle at third vertex |
| `translation` | vec2 | -0.9 to 0.9 | Möbius translation in disk |
| `rotation` | float | radians | Euclidean rotation |
| `diskScale` | float | 0.5-2.0 | Disk size relative to screen |

---

## Phase 1: Config and Registration

**Goal**: Create config struct and register effect in the system.

**Build**:
- Create `src/config/poincare_disk_config.h` with `PoincareDiskConfig` struct containing:
  - `enabled` (bool), `tileP/Q/R` (int, defaults 4,4,4)
  - `translationX/Y` (float, default 0), `translationSpeed` (float, default 0)
  - `diskScale` (float, default 1.0), `rotationSpeed` (float, default 0)
  - `translationAmplitude`, `translationFreqX/Y` for Lissajous (float)
- Add `#include`, enum `TRANSFORM_POINCARE_DISK`, name case, default order in `effect_config.h`
- Add `PoincareDiskConfig poincareDisk` member to `EffectConfig`
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro in `preset.cpp`

**Done when**: Project compiles with new config struct accessible via `EffectConfig`.

---

## Phase 2: Shader Implementation

**Goal**: Create the fragment shader with hyperbolic tiling algorithm.

**Build**:
- Create `shaders/poincare_disk.fs` implementing:
  - Complex math helpers (cmul, cdiv, conj)
  - mobiusTranslate function
  - solveFundamentalDomain function
  - hyperbolicFold function
  - Main: compute transformed UV, sample texture, output transparent if outside disk
- Uniforms: `texture0`, `tileP`, `tileQ`, `tileR`, `translation`, `rotation`, `diskScale`

**Done when**: Shader compiles (syntax-check via loading in app).

---

## Phase 3: Pipeline Integration

**Goal**: Load shader, bind uniforms, add to transform dispatch.

**Build**:
- Add to `post_effect.h`:
  - `Shader poincareDiskShader`
  - Uniform location ints: `poincareDiskTileP/Q/R`, `poincareDiskTranslation`, `poincareDiskRotation`, `poincareDiskDiskScale`
  - Runtime: `float poincareTime`, `float currentPoincareTranslation[2]`, `float currentPoincareRotation`
- Add to `post_effect.cpp`:
  - LoadShader, shader validity check, GetShaderLocation calls, UnloadShader
  - Initialize time/state to 0 in clear function
- Add to `shader_setup.h`: declare `SetupPoincareDisk`
- Add to `shader_setup.cpp`:
  - Implement `SetupPoincareDisk` - bind all uniforms from config and runtime state
  - Add case to `GetTransformEffect` dispatch table
- Add to `render_pipeline.cpp`:
  - Time accumulation: `pe->poincareTime += dt * config.translationSpeed`
  - Rotation accumulation: `pe->currentPoincareRotation += dt * config.rotationSpeed`
  - Lissajous translation computation from amplitude/freq params

**Done when**: Effect renders (enable in code, verify tiling appears).

---

## Phase 4: Modulation and UI

**Goal**: Add modulatable parameters and ImGui controls.

**Build**:
- Add to `param_registry.cpp`:
  - `{"poincareDisk.translationX", {-0.9f, 0.9f}}`
  - `{"poincareDisk.translationY", {-0.9f, 0.9f}}`
  - `{"poincareDisk.diskScale", {0.5f, 2.0f}}`
  - `{"poincareDisk.rotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
  - Register targets in `ParamRegistryInit`
- Add to `imgui_effects.cpp`:
  - Static section state variable
  - DrawSectionBegin/End block with enabled checkbox
  - IntSliders for P, Q, R (range 2-12)
  - ModulatableSliders for translationX/Y, diskScale
  - SliderFloat for translationSpeed, rotationSpeed (using ModulatableSliderAngleDeg for rotation)
  - TreeNode for Lissajous animation params

**Done when**: UI controls appear and modulation routes work.

---

## Phase 5: Polish and Validation

**Goal**: Verify correctness, test edge cases, ensure preset save/load.

**Build**:
- Test classic tilings: (2,3,7), (3,3,4), (4,4,4), (5,4,2)
- Verify constraint `1/P + 1/Q + 1/R < 1` produces valid hyperbolic geometry
- Test diskScale < 1 (should show transparent outside disk)
- Test diskScale > 1 (should fill entire screen)
- Save/load preset, verify all params persist
- Test modulation routing on translation params

**Done when**: Effect works correctly across parameter ranges, presets save/load.
