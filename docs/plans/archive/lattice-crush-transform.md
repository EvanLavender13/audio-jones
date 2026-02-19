# Lattice Crush Transform

A retro transform effect that applies the Bit Crush generator's lattice walk algorithm to the input image. Pixels cluster into irregular staircase-shaped blocks that constantly reorganize, each block sampling one input color. Like pixelation but with chaotic, shifting cell boundaries instead of a regular grid.

**Research**: `docs/research/bit-crush-transform.md`, `docs/research/bit-crush-walk-modes.md` (walk modes deferred to follow-up)

## Design

### Types

**LatticeCrushConfig** (`src/effects/lattice_crush.h`):

```
enabled : bool = false
scale : float = 0.3      // Coordinate zoom (0.05-1.0)
cellSize : float = 8.0   // Grid quantization coarseness (2.0-32.0)
iterations : int = 32    // Walk steps (4-64)
speed : float = 1.0      // Animation rate (0.1-5.0)
mix : float = 1.0        // Blend crushed with original (0.0-1.0)
```

**LatticeCrushEffect** (`src/effects/lattice_crush.h`):

```
shader : Shader
time : float              // Accumulated animation time
resolutionLoc : int
centerLoc : int
scaleLoc : int
cellSizeLoc : int
iterationsLoc : int
timeLoc : int
mixLoc : int
```

### Algorithm

The shader reuses the Bit Crush generator's exact `r()` hash and lattice walk loop. Instead of gradient LUT + FFT brightness, it maps the final walk position back to UV space and samples `texture0`. UV is clamped to `[0,1]` to avoid out-of-bounds artifacts.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 center;
uniform float scale;
uniform float cellSize;
uniform int iterations;
uniform float time;
uniform float mixAmount;

// Same hash as bit_crush.fs
float r(vec2 p, float t) {
    return cos(t * cos(p.x * p.y));
}

void main() {
    // Center-relative, resolution-scaled coordinates (same as generator)
    vec2 p = (fragTexCoord - center) * resolution * scale;

    // Iterative lattice walk (identical to generator)
    for (int i = 0; i < iterations; i++) {
        vec2 ceilCell = ceil(p / cellSize);
        vec2 floorCell = floor(p / cellSize);
        p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
    }

    // Map final position back to UV and sample input (clamp edges)
    vec2 sampleUV = clamp(p / (resolution * scale) + center, 0.0, 1.0);
    vec3 crushed = texture(texture0, sampleUV).rgb;
    vec3 original = texture(texture0, fragTexCoord).rgb;

    finalColor = vec4(mix(original, crushed, mixAmount), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| scale | float | 0.05-1.0 | 0.3 | Yes | "Scale" |
| cellSize | float | 2.0-32.0 | 8.0 | Yes | "Cell Size" |
| iterations | int | 4-64 | 32 | No (SliderInt) | "Iterations" |
| speed | float | 0.1-5.0 | 1.0 | Yes | "Speed" |
| mix | float | 0.0-1.0 | 1.0 | Yes | "Mix" |

### Constants

- Enum: `TRANSFORM_LATTICE_CRUSH`
- Display name: `"Lattice Crush"`
- Category badge: `"RET"` (Retro)
- Section index: 6
- Flags: `EFFECT_FLAG_NONE`
- Config fields macro: `LATTICE_CRUSH_CONFIG_FIELDS`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/lattice_crush.h` (create)
**Creates**: `LatticeCrushConfig`, `LatticeCrushEffect` types, `LATTICE_CRUSH_CONFIG_FIELDS` macro, function declarations

**Do**: Create header following the Design section struct layouts. Declare `LatticeCrushEffectInit`, `LatticeCrushEffectSetup`, `LatticeCrushEffectUninit`, `LatticeCrushConfigDefault`, `LatticeCrushRegisterParams`. Setup signature: `(LatticeCrushEffect* e, const LatticeCrushConfig* cfg, float deltaTime)` — no FFT texture, no resolution params. Follow `bit_crush.h` as structural pattern but omit all FFT/LUT/blend fields.

**Verify**: Header compiles standalone.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create effect module

**Files**: `src/effects/lattice_crush.cpp` (create)
**Depends on**: Wave 1

**Do**: Implement all lifecycle functions. Follow `bit_crush.cpp` as pattern but much simpler — no FFT uniforms, no ColorLUT, no blend compositor. Init loads `shaders/lattice_crush.fs` and caches 7 uniform locations (resolution, center, scale, cellSize, iterations, time, mixAmount). Setup accumulates `time += speed * deltaTime`, binds all uniforms. Include inline `SetupLatticeCrush(PostEffect* pe)` bridge and `REGISTER_EFFECT(TRANSFORM_LATTICE_CRUSH, LatticeCrush, latticeCrush, "Lattice Crush", "RET", 6, EFFECT_FLAG_NONE, SetupLatticeCrush, NULL)` at bottom.

**Verify**: Compiles (with other Wave 2 tasks).

---

#### Task 2.2: Create fragment shader

**Files**: `shaders/lattice_crush.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the Algorithm section verbatim. The shader is complete in the Design section above.

**Verify**: No build step needed (runtime-compiled by GPU).

---

#### Task 2.3: Register in effect config

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/lattice_crush.h"` with the other effect includes. Add `TRANSFORM_LATTICE_CRUSH` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`. Add `TRANSFORM_LATTICE_CRUSH` to `TransformOrderConfig::order` array at end before closing brace. Add `LatticeCrushConfig latticeCrush;` to `EffectConfig` struct.

**Verify**: Compiles.

---

#### Task 2.4: Add effect member to PostEffect

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/lattice_crush.h"` with other effect includes. Add `LatticeCrushEffect latticeCrush;` member to `PostEffect` struct after the other effect members.

**Verify**: Compiles.

---

#### Task 2.5: Add to build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/lattice_crush.cpp` to `EFFECTS_SOURCES` list alphabetically.

**Verify**: CMake configure succeeds.

---

#### Task 2.6: Add UI controls

**Files**: `src/ui/imgui_effects_retro.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/lattice_crush.h"` at top. Add `static bool sectionLatticeCrush = false;` with other section flags. Create `DrawRetroLatticeCrush()` following the pattern of `DrawRetroPixelation()` — `DrawSectionBegin("Lattice Crush", ...)`, enabled checkbox with `MoveTransformToEnd`, then 5 sliders: `ModulatableSlider("Scale", ...)`, `ModulatableSlider("Cell Size", ...)`, `ImGui::SliderInt("Iterations", ...)`, `ModulatableSlider("Speed", ...)`, `ModulatableSlider("Mix", ...)`. Add call in `DrawRetroCategory()` with `ImGui::Spacing()` before it — place after Synthwave (last current entry).

**Verify**: Compiles.

---

#### Task 2.7: Add preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/lattice_crush.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LatticeCrushConfig, LATTICE_CRUSH_CONFIG_FIELDS)` with other per-config macros. Add `X(latticeCrush)` to `EFFECT_CONFIG_FIELDS` X-macro table (append at end of list).

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in Retro category UI panel
- [ ] Effect appears in transform order pipeline
- [ ] Enabling shows "Lattice Crush" with "RET" badge
- [ ] Scale, Cell Size, Iterations, Speed, Mix sliders work
- [ ] Mix at 0.0 shows original image, at 1.0 shows full crush
- [ ] UV clamping prevents artifacts at boundaries
- [ ] Preset save/load preserves all settings
