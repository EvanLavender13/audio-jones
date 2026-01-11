# KIFS Fix and Iterative Mirror Removal

Fix KIFS to use correct Cartesian `abs()` folding algorithm per `docs/research/kifs-correct.md`. Remove fabricated Iterative Mirror effect.

## Current State

**KIFS (broken):**
- `shaders/kifs.fs` - Uses polar kaleidoscope (`atan`/`mod` angle folding), not KIFS
- `src/config/kifs_config.h` - Has invalid `segments` parameter
- Referenced in: `effect_config.h`, `shader_setup.cpp/h`, `render_pipeline.cpp`, `post_effect.cpp/h`, `imgui_effects_transforms.cpp`, `preset.cpp`, `param_registry.cpp`

**Iterative Mirror (to delete):**
- `shaders/iterative_mirror.fs`
- `src/config/iterative_mirror_config.h`
- `TRANSFORM_ITERATIVE_MIRROR` in enum
- Referenced in same files as KIFS

## Technical Implementation

**Source**: `docs/research/kifs-correct.md`, [KIFS Flythrough - Shadertoy](https://www.shadertoy.com/view/XsKXzc)

**Core KIFS algorithm** (replaces polar folding):
```glsl
for (int i = 0; i < iterations; i++) {
    p = abs(p);                          // Fold into positive quadrant
    if (octantFold && p.x < p.y) {
        p.xy = p.yx;                     // Optional: fold into one octant
    }
    p = p * scale - kifsOffset;          // IFS contraction
    p = rotate2d(p, twistAngle);         // Per-iteration rotation
}
```

**Key differences from current:**
| Aspect | Current (wrong) | Correct |
|--------|-----------------|---------|
| Fold operation | `mod(angle, 2π/segments)` | `abs(p)` |
| Coordinate space | Polar | Cartesian |
| `segments` param | Controls wedge count | Does not exist |

**Parameters after fix:**
| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| iterations | int | 1-12 | Fold/scale/translate cycles |
| scale | float | 1.5-4.0 | Expansion factor per iteration |
| offsetX | float | 0.0-2.0 | X translation after fold |
| offsetY | float | 0.0-2.0 | Y translation after fold |
| rotationSpeed | float | rad/frame | Animation rotation rate |
| twistAngle | float | radians | Per-iteration rotation offset |
| octantFold | bool | - | Enable 8-way octant symmetry |

---

## Phase 1: Remove Iterative Mirror

**Goal**: Delete all traces of the fabricated Iterative Mirror effect.

**Delete files:**
- `shaders/iterative_mirror.fs`
- `src/config/iterative_mirror_config.h`

**Modify files:**
- `src/config/effect_config.h` - Remove `#include`, enum value, name case, struct member, default order entry
- `src/render/shader_setup.h` - Remove `iterativeMirrorShader` declaration
- `src/render/shader_setup.cpp` - Remove shader load/unload
- `src/render/post_effect.h` - Remove `iterativeMirrorAccumRotation` declaration
- `src/render/post_effect.cpp` - Remove apply function and accumulator
- `src/render/render_pipeline.cpp` - Remove case in transform switch
- `src/ui/imgui_effects_transforms.cpp` - Remove UI section
- `src/config/preset.cpp` - Remove serialization (read/write)
- `src/automation/param_registry.cpp` - Remove parameter registrations

**Done when**: Build succeeds with no references to `iterativeMirror` or `IterativeMirror`.

---

## Phase 2: Fix KIFS Config

**Goal**: Update KifsConfig struct to match correct algorithm.

**Modify `src/config/kifs_config.h`:**
- Remove `segments` field
- Add `octantFold` bool (default false)
- Update comment to reflect Cartesian abs() folding

**Done when**: Config struct matches research doc parameters.

---

## Phase 3: Fix KIFS Shader

**Goal**: Replace polar kaleidoscope with true KIFS algorithm.

**Rewrite `shaders/kifs.fs`:**
```glsl
#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;

uniform int iterations;      // 1-12
uniform float scale;         // 1.5-4.0
uniform vec2 kifsOffset;     // Translation after fold
uniform float rotation;      // Global rotation (accumulated)
uniform float twistAngle;    // Per-iteration rotation
uniform bool octantFold;     // Optional octant folding

out vec4 finalColor;

vec2 rotate2d(vec2 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

vec2 mirror(vec2 x) {
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

void main() {
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Initial rotation
    p = rotate2d(p, rotation);

    // KIFS iteration
    for (int i = 0; i < iterations; i++) {
        // Fold: reflect into positive quadrant
        p = abs(p);

        // Optional: fold into one octant
        if (octantFold && p.x < p.y) {
            p.xy = p.yx;
        }

        // IFS contraction: scale and translate toward offset
        p = p * scale - kifsOffset;

        // Per-iteration rotation
        p = rotate2d(p, twistAngle + float(i) * 0.1);
    }

    // Sample texture at transformed position
    vec2 newUV = mirror(p * 0.5 + 0.5);
    finalColor = texture(texture0, newUV);
}
```

**Done when**: Shader compiles and produces fractal crystalline patterns (not radial wedges).

---

## Phase 4: Update Shader Setup

**Goal**: Update uniform bindings for KIFS.

**Modify `src/render/shader_setup.cpp`:**
- Remove `segments` uniform binding
- Add `octantFold` uniform binding

**Modify `src/render/post_effect.cpp`:**
- Update KIFS apply function to pass new uniforms
- Remove segments, add octantFold

**Done when**: Shader receives correct uniforms.

---

## Phase 5: Update UI

**Goal**: Update KIFS controls in ImGui.

**Modify `src/ui/imgui_effects_transforms.cpp`:**
- Remove segments slider
- Add octantFold checkbox

**Done when**: UI shows correct KIFS controls without segments.

---

## Phase 6: Update Serialization and Registry

**Goal**: Update preset loading/saving and parameter automation.

**Modify `src/config/preset.cpp`:**
- Remove `segments` from KIFS JSON read/write
- Add `octantFold` to KIFS JSON read/write

**Modify `src/automation/param_registry.cpp`:**
- Remove KIFS segments registration
- Add KIFS octantFold registration (if bool params supported, otherwise skip)

**Done when**: Presets save/load correctly with new KIFS parameters.

---

## Phase 7: Migrate Presets

**Goal**: Update existing preset files.

**Scan presets for KIFS usage:**
- Remove `segments` field from any KIFS config
- Add `octantFold: false` as default

**Done when**: All presets load without warnings.

---

## Phase 8: Verification

**Goal**: Confirm everything works.

**Tests:**
- Build succeeds
- No runtime errors on startup
- KIFS produces crystalline fractal patterns (not radial wedges)
- Iterative Mirror no longer appears in UI or presets
- Presets with KIFS load correctly

**Done when**: Visual inspection confirms correct KIFS behavior.
