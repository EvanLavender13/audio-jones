# Perspective Tilt

Single-pass transform that projects the input texture onto a tilted 3D plane. Pitch and yaw sliders rotate the viewing angle while perspective division creates foreshortening — the far edge compresses, the near edge stretches. Roll adds in-plane rotation, FOV controls perspective intensity, and auto-zoom keeps the frame filled at any tilt angle.

**Research**: `docs/research/perspective_tilt.md`

## Design

### Types

**Config** (`PerspectiveTiltConfig`):

```
bool enabled     = false
float pitch      = 0.0f       // Forward/backward tilt in radians (-PI/2 to PI/2)
float yaw        = 0.0f       // Left/right tilt in radians (-PI/2 to PI/2)
float roll       = 0.0f       // In-plane rotation in radians (-PI to PI)
float fov        = 60.0f      // Perspective intensity in degrees (20-120)
bool autoZoom    = true        // Scale to fill frame at any tilt angle
```

**Effect** (`PerspectiveTiltEffect`):

```
Shader shader
int resolutionLoc
int pitchLoc
int yawLoc
int rollLoc
int fovLoc
int autoZoomLoc
```

No time accumulator — all params are static per frame.

### Algorithm

The shader implements this pipeline (from the research doc's "Projection Pipeline"):

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float pitch;    // radians
uniform float yaw;      // radians
uniform float roll;     // radians
uniform float fov;      // degrees
uniform int autoZoom;   // 1 = on, 0 = off

void main() {
    float aspect = resolution.x / resolution.y;

    // 1. Center and aspect-correct
    vec2 p = (fragTexCoord - 0.5) * vec2(aspect, 1.0);

    // 2. Lift to 3D ray — z encodes FOV
    //    Low FOV → large z → mild foreshortening
    //    High FOV → small z → dramatic perspective
    float zFov = 0.5 / tan(fov * 3.14159265 / 360.0);
    vec3 ray = vec3(p, zFov);

    // 3-5. Combined YXZ rotation (yaw, then pitch, then roll)
    //
    // Yaw (Y-axis) × Pitch (X-axis) matrix:
    //   R[0] = ( cos_y,              sin_y * sin_p,  sin_y * cos_p )
    //   R[1] = ( 0,                  cos_p,         -sin_p         )
    //   R[2] = (-sin_y,              cos_y * sin_p,  cos_y * cos_p )
    //
    float cp = cos(pitch), sp = sin(pitch);
    float cy = cos(yaw),   sy = sin(yaw);

    vec3 rotated = vec3(
        cy * ray.x + sy * sp * ray.y + sy * cp * ray.z,
                              cp * ray.y -      sp * ray.z,
       -sy * ray.x + cy * sp * ray.y + cy * cp * ray.z
    );

    // Roll (Z-axis) applied to rotated.xy
    float cr = cos(roll), sr = sin(roll);
    rotated.xy = vec2(
        cr * rotated.x - sr * rotated.y,
        sr * rotated.x + cr * rotated.y
    );

    // Back-face cull: if z <= 0 the plane faces away
    if (rotated.z <= 0.0) {
        finalColor = vec4(0.0);
        return;
    }

    // 6. Perspective divide
    vec2 tilted = rotated.xy / rotated.z;

    // 7. Auto-zoom: project all 4 corners through the same transform,
    //    find bounding extent, scale tilted so warped image fills the screen
    if (autoZoom == 1) {
        float zoom = 0.0;
        for (int i = 0; i < 4; i++) {
            vec2 corner = vec2(
                (i < 2 ? -1.0 : 1.0) * aspect * 0.5,
                (i == 0 || i == 3 ? -0.5 : 0.5)
            );
            vec3 cr3 = vec3(corner, zFov);
            vec3 rc = vec3(
                cy * cr3.x + sy * sp * cr3.y + sy * cp * cr3.z,
                                     cp * cr3.y -      sp * cr3.z,
               -sy * cr3.x + cy * sp * cr3.y + cy * cp * cr3.z
            );
            rc.xy = vec2(cr * rc.x - sr * rc.y, sr * rc.x + cr * rc.y);
            if (rc.z > 0.0) {
                vec2 proj = rc.xy / rc.z;
                zoom = max(zoom, max(abs(proj.x) / (aspect * 0.5),
                                     abs(proj.y) / 0.5));
            }
        }
        if (zoom > 0.0) tilted /= zoom;
    }

    // 8. Un-correct aspect and map back to texture UV
    vec2 uv = tilted / vec2(aspect, 1.0) + 0.5;

    // 9. Bounds check — output black outside [0,1]
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0);
        return;
    }

    finalColor = texture(texture0, uv);
}
```

**Key details**:
- Clamp effective pitch/yaw to ~85 degrees (±1.4835 rad) on the CPU before passing uniforms — prevents `ray.z → 0` division artifacts
- Auto-zoom at zero tilt gives zoom ≈ 1.0 (identity); at high tilt it crops aggressively
- Roll without pitch/yaw is just flat rotation — still functional but only interesting with tilt

<!-- Intentional deviation: auto-zoom normalizes each axis by its half-extent (aspect*0.5 for x, 0.5 for y) rather than raw max(|x|,|y|) as in research pseudocode. The research formula would produce incorrect zoom on non-square viewports. -->
<!-- Intentional deviation: roll is included in auto-zoom corner projection. Research pseudocode omits it, but "the same rotation" implies all rotations. Without roll, auto-zoom would under-compensate when roll is nonzero. -->

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| pitch | float | -PI/2..PI/2 rad (-90..90 deg) | 0.0 | Yes | "Pitch" (AngleDeg slider) |
| yaw | float | -PI/2..PI/2 rad (-90..90 deg) | 0.0 | Yes | "Yaw" (AngleDeg slider) |
| roll | float | -PI..PI rad (-180..180 deg) | 0.0 | Yes | "Roll" (AngleDeg slider) |
| fov | float | 20..120 deg | 60.0 | Yes | "FOV" (regular slider, "%.0f") |
| autoZoom | bool | on/off | true | No | "Auto Zoom" (checkbox) |

**Angle clamping**: In `PerspectiveTiltEffectSetup`, clamp pitch and yaw to ±1.4835f (≈85 degrees) before passing to shader. This is a safety clamp — the UI range already limits to ±PI/2.

### Constants

- Enum: `TRANSFORM_PERSPECTIVE_TILT`
- Display name: `"Perspective Tilt"`
- Category badge: `"OPT"`
- Section index: `7` (Optical)
- Flags: `EFFECT_FLAG_NONE`
- Macro: `REGISTER_EFFECT`

### References

- [2D Perspective Shader — Godot Shaders](https://godotshaders.com/shader/2d-perspective/) — YXZ rotation, FOV via tangent, auto-zoom via inset
- [Planar Projection Mapping — WebGL Fundamentals](https://webglfundamentals.org/webgl/lessons/webgl-planar-projection-mapping.html) — texture matrix with perspective division

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/perspective_tilt.h` (create)
**Creates**: `PerspectiveTiltConfig`, `PerspectiveTiltEffect` types, function declarations

**Do**: Follow `src/effects/heightfield_relief.h` structure. Define `PerspectiveTiltConfig` with fields from the Design section (pitch, yaw, roll as float with default 0.0f; fov as float with default 60.0f; autoZoom as bool with default true; enabled as bool with default false). Define `PERSPECTIVE_TILT_CONFIG_FIELDS` macro listing all serializable fields. Define `PerspectiveTiltEffect` typedef struct with shader and 6 uniform location ints (resolutionLoc, pitchLoc, yawLoc, rollLoc, fovLoc, autoZoomLoc). Declare 5 public functions: Init, Setup, Uninit, RegisterParams, ConfigDefault. Setup signature: `(PerspectiveTiltEffect* e, const PerspectiveTiltConfig* cfg)` — no deltaTime needed (no animation state). Add inline range comments on each config field.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker dependency yet).

---

### Wave 2: Implementation (all parallel — no file overlap)

#### Task 2.1: Create effect implementation

**Files**: `src/effects/perspective_tilt.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `src/effects/heightfield_relief.cpp` structure exactly. Includes: own header, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/effect_descriptor.h`, `render/post_effect.h`, `imgui.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.

- `Init`: Load `shaders/perspective_tilt.fs`, cache all 6 uniform locations, return false if shader fails.
- `Setup`: Get resolution via `GetScreenWidth()`/`GetScreenHeight()`. Clamp pitch and yaw to ±1.4835f before passing to shader. Pass fov as degrees (shader expects degrees). Pass autoZoom as int (1 or 0). Set all uniforms.
- `Uninit`: Unload shader.
- `RegisterParams`: Register all 4 float params: `perspectiveTilt.pitch` (±PI/2), `perspectiveTilt.yaw` (±PI/2), `perspectiveTilt.roll` (±PI), `perspectiveTilt.fov` (20-120).
- `ConfigDefault`: Return `PerspectiveTiltConfig{}`.

UI section (`// === UI ===`):
- `static void DrawPerspectiveTiltParams(EffectConfig*, const ModSources*, ImU32)`:
  - `ModulatableSliderAngleDeg` for pitch, yaw, roll (pitch/yaw range ±PI/2, roll range ±PI via ROTATION_OFFSET_MAX)
  - `ModulatableSlider` for fov with range 20-120, format `"%.0f"`
  - `ImGui::Checkbox` for autoZoom

Bridge function: `void SetupPerspectiveTilt(PostEffect* pe)` — non-static, calls `PerspectiveTiltEffectSetup(&pe->perspectiveTilt, &pe->effects.perspectiveTilt)`.

Registration: `REGISTER_EFFECT(TRANSFORM_PERSPECTIVE_TILT, PerspectiveTilt, perspectiveTilt, "Perspective Tilt", "OPT", 7, EFFECT_FLAG_NONE, SetupPerspectiveTilt, NULL, DrawPerspectiveTiltParams)`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Create fragment shader

**Files**: `shaders/perspective_tilt.fs` (create)
**Depends on**: Wave 1 complete (for uniform naming agreement only — no #include)

**Do**: Implement the Algorithm section from the Design above. The full shader is provided inline — implement it verbatim. Add attribution comment block at the top before `#version`:

```
// Inspired by "2D Perspective" by Whirl
// https://godotshaders.com/shader/2d-perspective/
// Adapted for AudioJones: GLSL 330, resolution uniform, autoZoom as int toggle
```

Uniforms: `sampler2D texture0`, `vec2 resolution`, `float pitch`, `float yaw`, `float roll`, `float fov`, `int autoZoom`.

**Verify**: File parses (no syntax errors visible).

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/perspective_tilt.h"` in alphabetical position (after `pencil_sketch.h`, before `phi_blur.h`)
2. Add `TRANSFORM_PERSPECTIVE_TILT,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add `PerspectiveTiltConfig perspectiveTilt;` to `EffectConfig` struct with comment `// Perspective Tilt (3D plane projection with pitch/yaw/roll)`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/perspective_tilt.h"` in alphabetical position (after `pencil_sketch.h`, before `phi_blur.h`)
2. Add `PerspectiveTiltEffect perspectiveTilt;` to `PostEffect` struct (after the Optical effects group, near `HeightfieldReliefEffect`)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Add to CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/perspective_tilt.cpp` to `EFFECTS_SOURCES` in alphabetical position (after `pencil_sketch.cpp`, before `phi_blur.cpp`).

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Register serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/perspective_tilt.h"` with the other effect includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PerspectiveTiltConfig, PERSPECTIVE_TILT_CONFIG_FIELDS)` with the other per-config JSON macros
3. Add `X(perspectiveTilt) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Optical section of transform list
- [ ] "OPT" badge displays correctly
- [ ] Pitch/yaw sliders tilt the view with foreshortening
- [ ] Roll rotates the tilted plane
- [ ] FOV slider changes perspective intensity (low = subtle, high = dramatic)
- [ ] Auto Zoom checkbox toggles frame-filling behavior
- [ ] Extreme tilt (near ±90 deg) doesn't produce artifacts (85-deg clamp)
- [ ] All four float params respond to modulation routing
- [ ] Preset save/load preserves all settings
