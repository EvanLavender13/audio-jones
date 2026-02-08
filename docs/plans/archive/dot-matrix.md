# Dot Matrix

LED billboard / neon dot display effect. Tiles the scene into a uniform grid and renders each cell as a soft-glow dot colored by the input texture at that cell's center. Bright pixels produce vivid glowing circles; dark pixels fade to nothing against a black background.

**Research**: `docs/research/dot_matrix.md`

## Design

### Types

**DotMatrixConfig** (in `dot_matrix.h`):
```
enabled       bool    false
dotScale      float   32.0    // Grid resolution (4.0-80.0)
softness      float   1.2     // Glow falloff tightness (0.2-4.0)
brightness    float   3.0     // Dot intensity multiplier (0.5-8.0)
rotationSpeed float   0.0     // Grid rotation rate (rad/s)
rotationAngle float   0.0     // Static rotation offset (rad)
```

**DotMatrixEffect** (in `dot_matrix.h`):
```
shader        Shader
resolutionLoc int
dotScaleLoc   int
softnessLoc   int
brightnessLoc int
rotationLoc   int
rotation      float   // accumulated rotation
```

### Algorithm

Single-pass fragment shader. Same grid-quantization + rotation pattern as `halftone.fs`.

1. **Grid setup**: Scale UV by `dotScale * aspect_ratio`, apply rotation matrix, `floor()` to snap to cell
2. **Texture sampling**: Unrotate cell center via `transpose(m) * cell` (same as halftone), sample input texture, compute luma
3. **Dot shape**: Remap cell-local coords to [-1,1], compute `dist = length(localUV)`, apply inverse-cube glow kernel: `softness / pow(1.0 + softness * dist, 3.0)`, clamp to [0,1]
4. **Compositing**: `result = texColor * luma * glow * brightness` on black background

The glow kernel replaces halftone's `smoothstep` circle — produces a natural light-like falloff that reads as "glowing."

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| dotScale | float | 4.0–80.0 | 32.0 | Yes | Scale |
| softness | float | 0.2–4.0 | 1.2 | Yes | Softness |
| brightness | float | 0.5–8.0 | 3.0 | Yes | Brightness |
| rotationSpeed | float | -ROTATION_SPEED_MAX–ROTATION_SPEED_MAX | 0.0 | Yes | Spin |
| rotationAngle | float | -ROTATION_OFFSET_MAX–ROTATION_OFFSET_MAX | 0.0 | Yes | Angle |

### Constants

- Enum name: `TRANSFORM_DOT_MATRIX`
- Display name: `"Dot Matrix"`
- Category: `TRANSFORM_CATEGORY_CELLULAR` → `{"CELL", 2}`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create effect module header and source

**Files**: `src/effects/dot_matrix.h` (create), `src/effects/dot_matrix.cpp` (create)

**Creates**: `DotMatrixConfig`, `DotMatrixEffect` types, lifecycle functions

**Do**: Follow `halftone.h`/`halftone.cpp` as the structural template. Header declares both structs and 5 functions (Init, Setup, Uninit, ConfigDefault, RegisterParams). Source implements all five. Setup accumulates `rotation += rotationSpeed * deltaTime`, combines with `rotationAngle`, binds all uniforms. RegisterParams registers all 5 params via `ModEngineRegisterParam`.

**Verify**: `cmake.exe --build build` compiles (will fail on missing shader — expected).

---

### Wave 2: Integration (all parallel — no file overlap)

#### Task 2.1: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)

**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/dot_matrix.h"` with other includes
2. Add `TRANSFORM_DOT_MATRIX` to enum before `TRANSFORM_EFFECT_COUNT`
3. Add `"Dot Matrix"` to `TRANSFORM_EFFECT_NAMES` array at matching index
4. Add `TRANSFORM_DOT_MATRIX` to `TransformOrderConfig::order` initializer
5. Add `DotMatrixConfig dotMatrix;` member to `EffectConfig` struct
6. Add `case TRANSFORM_DOT_MATRIX: return e->dotMatrix.enabled;` to `IsTransformEnabled`

**Verify**: Compiles.

#### Task 2.2: Create fragment shader

**Files**: `shaders/dot_matrix.fs` (create)

**Depends on**: Wave 1

**Do**: Implement the algorithm from the Design section. Use `halftone.fs` as structural reference for the grid + rotation pattern. Key differences from halftone: aspect-ratio correction via `ratio = vec2(1.0, resolution.x / resolution.y)`, inverse-cube glow kernel instead of smoothstep, compositing on black instead of white. Uniforms: `resolution`, `dotScale`, `softness`, `brightness`, `rotation`.

**Verify**: File exists and parses as valid GLSL.

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)

**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/dot_matrix.h"` in `post_effect.h`
2. Add `DotMatrixEffect dotMatrix;` member to `PostEffect` struct
3. Add `DotMatrixEffectInit(&pe->dotMatrix)` call in `PostEffectInit()`
4. Add `DotMatrixEffectUninit(&pe->dotMatrix)` call in `PostEffectUninit()`
5. Add `DotMatrixRegisterParams(&pe->effects.dotMatrix)` call in `PostEffectRegisterParams()`

**Verify**: Compiles.

#### Task 2.4: Shader setup (cellular category)

**Files**: `src/render/shader_setup_cellular.h` (modify), `src/render/shader_setup_cellular.cpp` (modify), `src/render/shader_setup.cpp` (modify)

**Depends on**: Wave 1

**Do**:
1. Declare `SetupDotMatrix(PostEffect* pe)` in `shader_setup_cellular.h`
2. Implement `SetupDotMatrix` in `shader_setup_cellular.cpp` — delegates to `DotMatrixEffectSetup(&pe->dotMatrix, &pe->effects.dotMatrix, pe->currentDeltaTime)`
3. Add dispatch case in `shader_setup.cpp`: `case TRANSFORM_DOT_MATRIX: return {&pe->dotMatrix.shader, SetupDotMatrix, &pe->effects.dotMatrix.enabled};`

**Verify**: Compiles.

#### Task 2.5: UI panel (cellular category)

**Files**: `src/ui/imgui_effects_cellular.cpp` (modify)

**Depends on**: Wave 1

**Do**:
1. Add `static bool sectionDotMatrix = false;` with other section bools
2. Create `DrawCellularDotMatrix()` function. Pattern: checkbox with `MoveTransformToEnd`, then `ModulatableSlider` for Scale (dotScale), Softness, Brightness, plus `ModulatableSliderSpeedDeg` for Spin (rotationSpeed) and `ModulatableSliderAngleDeg` for Angle (rotationAngle)
3. Call from `DrawCellularCategory()` with `ImGui::Spacing()` separator
4. Add `case TRANSFORM_DOT_MATRIX:` returning `{"CELL", 2}` in `GetTransformCategory()` in `imgui_effects.cpp`

Note: Task touches `imgui_effects.cpp` for the category case — no other task in this wave touches that file.

**Verify**: Compiles.

#### Task 2.6: Preset serialization

**Files**: `src/config/preset.cpp` (modify)

**Depends on**: Wave 1

**Do**:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DotMatrixConfig, enabled, dotScale, softness, brightness, rotationSpeed, rotationAngle)` macro
2. Add `if (e.dotMatrix.enabled) { j["dotMatrix"] = e.dotMatrix; }` in `to_json`
3. Add `e.dotMatrix = j.value("dotMatrix", e.dotMatrix);` in `from_json`

**Verify**: Compiles.

#### Task 2.7: Build system

**Files**: `CMakeLists.txt` (modify)

**Depends on**: Wave 1

**Do**: Add `src/effects/dot_matrix.cpp` to `EFFECTS_SOURCES`.

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform pipeline with "CELL" badge
- [ ] Enabling adds it to the pipeline list via drag-drop reorder
- [ ] UI sliders modify dot appearance in real-time
- [ ] Rotation (speed + static angle) works correctly
- [ ] Dark regions vanish, bright regions produce vivid glow dots
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to all 5 registered parameters
