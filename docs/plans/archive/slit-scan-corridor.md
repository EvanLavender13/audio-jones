# Slit Scan Corridor

Simulates the 2001: A Space Odyssey stargate sequence using ping-pong render textures. Each frame, a 2-pixel vertical slit is sampled from the current scene and stamped at the center of an accumulation texture. Old content pushes outward with perspective-accelerated motion. The corridor **replaces** the scene — it is a transform, not a blend overlay. Two shaders: an accumulation shader (ping-pong) and a display shader (rotation + output).

**Research**: `docs/research/slit-scan-corridor.md`

## Design

### Types

**SlitScanCorridorConfig** (`src/effects/slit_scan_corridor.h`):

```
struct SlitScanCorridorConfig {
  bool enabled = false;

  // Slit sampling
  float slitPosition = 0.5f;    // Horizontal UV to sample (0.0-1.0)
  float slitWidth = 0.005f;     // Feathering radius (0.001-0.05)

  // Corridor dynamics
  float speed = 2.0f;           // Outward advance rate (0.1-10.0)
  float perspective = 3.0f;     // Foreshortening strength (0.5-8.0)
  float decayHalfLife = 3.0f;   // Trail brightness half-life seconds (0.1-10.0)
  float brightness = 1.0f;      // Fresh slit brightness (0.1-3.0)

  // Rotation (display-time only, NOT inside ping-pong)
  float rotationAngle = 0.0f;   // Static rotation radians (-PI..PI)
  float rotationSpeed = 0.0f;   // Rotation rate rad/s (-PI..PI)
};
```

**SlitScanCorridorEffect** (`src/effects/slit_scan_corridor.h`):

```
typedef struct SlitScanCorridorEffect {
  Shader shader;                // Accumulation (ping-pong)
  Shader displayShader;         // Rotation + output
  RenderTexture2D pingPong[2];
  int readIdx;
  float rotationAccum;

  // Accumulation shader locations
  int resolutionLoc;
  int sceneTextureLoc;
  int slitPositionLoc;
  int speedDtLoc;              // speed * deltaTime (precomputed)
  int perspectiveLoc;
  int slitWidthLoc;
  int decayFactorLoc;
  int brightnessLoc;

  // Display shader locations
  int dispCorridorMapLoc;
  int dispRotationLoc;
} SlitScanCorridorEffect;
```

### Algorithm

#### Accumulation Shader (`slit_scan_corridor.fs`)

Reads `texture0` (previous ping-pong frame) and `sceneTexture` (current scene). Outputs the updated accumulation.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // previous accumulation (ping-pong read)
uniform sampler2D sceneTexture;  // current scene for slit sampling
uniform vec2 resolution;
uniform float slitPosition;      // 0-1 horizontal UV
uniform float speedDt;           // speed * deltaTime (precomputed on CPU)
uniform float perspective;
uniform float slitWidth;
uniform float decayFactor;
uniform float brightness;

void main() {
    vec2 uv = fragTexCoord;
    float dx = uv.x - 0.5;
    float d = abs(dx);

    // Perspective-accelerated inward sample
    // Near center (d~0): sampleD ~ d (barely moves)
    // Near edges (d~0.5): sampleD << d (rushes past)
    float sampleD = d / (1.0 + speedDt * d * perspective);
    float signDx = sign(dx);
    // Handle dx == 0: sign(0) = 0, but sampleD = 0 too so sampleU = 0.5
    vec2 sampleUV = vec2(0.5 + signDx * sampleD, uv.y);

    // Sample old accumulation with decay
    vec4 old = texture(texture0, sampleUV) * decayFactor;

    // Stamp fresh slit at center
    float halfPixel = 0.5 / resolution.x;
    float slitMask = smoothstep(slitWidth, 0.0, d);

    // Left of center: left pixel of slit; right of center: right pixel
    float slitU = (dx < 0.0)
        ? (slitPosition - halfPixel)
        : (slitPosition + halfPixel);
    vec4 slitColor = texture(sceneTexture, vec2(slitU, uv.y)) * brightness;

    // Vertical vignette (top/bottom fade to black — no floor/ceiling)
    float vy = abs(uv.y - 0.5) * 2.0;
    slitColor *= smoothstep(1.0, 0.7, vy);

    finalColor = mix(old, slitColor, slitMask);
}
```

#### Display Shader (`slit_scan_corridor_display.fs`)

Reads the accumulated corridor texture (with optional rotation) and outputs it directly as the new scene. This is NOT a blend — it replaces the input.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // scene (bound by RenderPass but unused)
uniform sampler2D corridorMap;   // corridor ping-pong result
uniform float rotation;

void main() {
    vec2 centered = fragTexCoord - vec2(0.5);
    float c = cos(rotation), s = sin(rotation);
    vec2 rotated = vec2(c * centered.x + s * centered.y,
                        -s * centered.x + c * centered.y) + vec2(0.5);
    finalColor = texture(corridorMap, rotated);
}
```

#### Render Flow (C++ side, prose)

1. **scratchSetup**: Bind all accumulation shader uniforms. Compute `speedDt = speed * deltaTime` and `decayFactor = exp(-0.693147 * deltaTime / max(halfLife, 0.001))` on CPU. Accumulate `rotationAccum += rotationSpeed * deltaTime`.

2. **render**: One internal pass:
   - **Ping-pong pass**: `BeginTextureMode(pingPong[writeIdx])` → `BeginShaderMode(shader)` → bind `sceneTexture` via `SetShaderValueTexture(shader, sceneTextureLoc, pe->currentSceneTexture)` → `RenderUtilsDrawFullscreenQuad(pingPong[readIdx].texture)` → swap indices.

3. **setup** (for the display RenderPass): Bind `corridorMap` = `pingPong[readIdx].texture` and `rotation` = `rotationAngle + rotationAccum` on the display shader. The pipeline's `RenderPass` draws this shader, replacing the scene with the corridor output.

#### Scene Texture Access

Add `Texture2D currentSceneTexture` to `PostEffect` (same pattern as `currentDeltaTime` — a temporary set by the pipeline). In `render_pipeline.cpp`, set `pe->currentSceneTexture = src->texture` before the `render(pe)` call in the `EFFECT_DESCRIPTORS[effectType].render != nullptr` branch.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| slitPosition | float | 0.0-1.0 | 0.5 | Yes | "Slit Position" |
| slitWidth | float | 0.001-0.05 | 0.005 | Yes | "Slit Width" |
| speed | float | 0.1-10.0 | 2.0 | Yes | "Speed" |
| perspective | float | 0.5-8.0 | 3.0 | Yes | "Perspective" |
| decayHalfLife | float | 0.1-10.0 | 3.0 | Yes | "Decay" |
| brightness | float | 0.1-3.0 | 1.0 | Yes | "Brightness" |
| rotationAngle | float | -PI..PI | 0.0 | Yes | "Rotation" (ModulatableSliderAngleDeg) |
| rotationSpeed | float | -PI..PI | 0.0 | Yes | "Rotation Speed" (ModulatableSliderSpeedDeg) |

### Constants

- Enum: `TRANSFORM_SLIT_SCAN_CORRIDOR`
- Display name: `"Slit Scan Corridor"`
- Badge: `"MOT"`, Section: `3`
- Flags: `EFFECT_FLAG_NEEDS_RESIZE` (no BLEND — this replaces the scene)
- Registration: Manual `EffectDescriptorRegister()` (need MOT/3 and custom render)

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Config header and effect_config integration

**Files**: `src/effects/slit_scan_corridor.h` (CREATE), `src/config/effect_config.h` (MODIFY)
**Creates**: `SlitScanCorridorConfig`, `SlitScanCorridorEffect` types, enum value

**Do**:

1. Create `src/effects/slit_scan_corridor.h`:
   - `SlitScanCorridorConfig` struct with all fields from the Design > Types section (NO blendMode, NO blendIntensity)
   - `#define SLIT_SCAN_CORRIDOR_CONFIG_FIELDS` macro listing all serializable fields
   - `SlitScanCorridorEffect` struct with all fields from the Design > Types section
   - Declare: `Init(Effect*, Config*, int w, int h)`, `Setup(Effect*, Config*, float deltaTime)`, `Render(Effect*, PostEffect*)`, `Resize(Effect*, int w, int h)`, `Uninit(Effect*)`, `RegisterParams(Config*)`, `ConfigDefault(void)`
   - Note: `Render` takes `PostEffect*` (needs `currentSceneTexture`)
   - Includes: `"raylib.h"`, `<stdbool.h>`

2. Modify `src/config/effect_config.h`:
   - Add `#include "effects/slit_scan_corridor.h"` with other effect includes (alphabetical)
   - Add `TRANSFORM_SLIT_SCAN_CORRIDOR,` before `TRANSFORM_EFFECT_COUNT`
   - Add `TRANSFORM_SLIT_SCAN_CORRIDOR` to end of `TransformOrderConfig::order` array
   - Add `SlitScanCorridorConfig slitScanCorridor;` to `EffectConfig` struct

**Verify**: `cmake.exe --build build` compiles (types exist but nothing uses them yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect module implementation

**Files**: `src/effects/slit_scan_corridor.cpp` (CREATE)

**Do**:

Follow the Attractor Lines pattern (`src/effects/attractor_lines.cpp`) for ping-pong lifecycle. Key differences:

- **Init** loads TWO shaders: `slit_scan_corridor.fs` (accumulation) and `slit_scan_corridor_display.fs` (display). Cache locations for both. Allocate ping-pong via `RenderUtilsInitTextureHDR`. Cascade cleanup on failure.
- **Setup** computes `speedDt = speed * deltaTime`, `decayFactor = expf(-0.693147f * deltaTime / fmaxf(halfLife, 0.001f))`. Accumulates `rotationAccum += rotationSpeed * deltaTime`. Binds all accumulation shader uniforms via `SetShaderValue`.
- **Render** takes `(SlitScanCorridorEffect*, const SlitScanCorridorConfig*, PostEffect*)` — needs `pe->currentSceneTexture` for slit sampling. One internal pass (ping-pong accumulation) as described in Design > Algorithm > Render Flow.
- **Uninit** unloads both shaders and ping-pong textures.
- **Resize** unloads and reinits ping-pong.

Bridge functions at bottom of file:
- `SetupSlitScanCorridor(PostEffect*)` — calls `SlitScanCorridorEffectSetup` (binds accumulation uniforms)
- `SetupSlitScanCorridorDisplay(PostEffect*)` — binds `corridorMap` (pingPong[readIdx].texture) and `rotation` on the display shader
- `RenderSlitScanCorridor(PostEffect*)` — calls `SlitScanCorridorEffectRender`

**Manual registration** at bottom (NOT a macro — need MOT/3 + custom render):
```
static bool Init_slitScanCorridor(PostEffect *pe, int w, int h) { ... }
static void Uninit_slitScanCorridor(PostEffect *pe) { ... }
static void Resize_slitScanCorridor(PostEffect *pe, int w, int h) { ... }
static void Register_slitScanCorridor(EffectConfig *cfg) { ... }
static Shader *GetShader_slitScanCorridor(PostEffect *pe) {
    return &pe->slitScanCorridor.displayShader;
}
static Shader *GetScratchShader_slitScanCorridor(PostEffect *pe) {
    return &pe->slitScanCorridor.shader;
}
static bool reg_slitScanCorridor = EffectDescriptorRegister(
    TRANSFORM_SLIT_SCAN_CORRIDOR,
    EffectDescriptor{TRANSFORM_SLIT_SCAN_CORRIDOR,
        "Slit Scan Corridor", "MOT", 3,
        offsetof(EffectConfig, slitScanCorridor.enabled),
        EFFECT_FLAG_NEEDS_RESIZE,
        Init_slitScanCorridor, Uninit_slitScanCorridor,
        Resize_slitScanCorridor, Register_slitScanCorridor,
        GetShader_slitScanCorridor, SetupSlitScanCorridorDisplay,
        GetScratchShader_slitScanCorridor, SetupSlitScanCorridor,
        RenderSlitScanCorridor});
```

Note: `getShader` returns the **display shader** (not the blend compositor). The pipeline's final `RenderPass` uses this shader to output the corridor as the new scene.

Includes: own header, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `<math.h>`, `<stddef.h>`

**Verify**: Compiles (with Wave 2 tasks 2.2-2.6 also complete).

---

#### Task 2.2: Shaders

**Files**: `shaders/slit_scan_corridor.fs` (CREATE), `shaders/slit_scan_corridor_display.fs` (CREATE)

**Do**: Implement the two shaders exactly as specified in the Design > Algorithm section. No modifications needed — copy the GLSL verbatim from the plan.

**Verify**: Files exist. (Shaders compile at runtime, not build time.)

---

#### Task 2.3: PostEffect and render pipeline integration

**Files**: `src/render/post_effect.h` (MODIFY), `src/render/render_pipeline.cpp` (MODIFY)

**Do**:

1. In `post_effect.h` — **already done by user**:
   - `#include "effects/slit_scan_corridor.h"` ✓
   - `SlitScanCorridorEffect slitScanCorridor;` member ✓
   - `Texture2D currentSceneTexture;` ✓

2. In `render_pipeline.cpp`, in `RenderPipelineApplyOutput`, in the transform loop, add one line before the `render != nullptr` branch executes:
   ```
   } else if (EFFECT_DESCRIPTORS[effectType].render != nullptr) {
       pe->currentSceneTexture = src->texture;  // ← ADD THIS LINE
       EFFECT_DESCRIPTORS[effectType].scratchSetup(pe);
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Build system

**Files**: `CMakeLists.txt` (MODIFY)

**Do**: Add `src/effects/slit_scan_corridor.cpp` to `EFFECTS_SOURCES` (alphabetical).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: UI panel

**Files**: `src/ui/imgui_effects_motion.cpp` (MODIFY)

**Do**:

Follow the existing motion effect UI pattern (e.g., `DrawMotionRelativisticDoppler`).

1. Add `static bool sectionSlitScanCorridor = false;` with other section bools
2. Add `DrawMotionSlitScanCorridor` function:
   - `DrawSectionBegin("Slit Scan Corridor", categoryGlow, &sectionSlitScanCorridor, e->slitScanCorridor.enabled)`
   - Enabled checkbox with `MoveTransformToEnd` on fresh enable (`TRANSFORM_SLIT_SCAN_CORRIDOR`)
   - Sliders (all `ModulatableSlider` unless noted):
     - `"Slit Position"` — `slitScanCorridor.slitPosition`, `"%.2f"`
     - `"Slit Width"` — `slitScanCorridor.slitWidth`, `"%.3f"` (ModulatableSliderLog — small range)
     - `"Speed"` — `slitScanCorridor.speed`, `"%.1f"`
     - `"Perspective"` — `slitScanCorridor.perspective`, `"%.1f"`
     - `"Decay"` — `slitScanCorridor.decayHalfLife`, `"%.1f"`
     - `"Brightness"` — `slitScanCorridor.brightness`, `"%.2f"`
     - `"Rotation"` — `ModulatableSliderAngleDeg`, `slitScanCorridor.rotationAngle`
     - `"Rotation Speed"` — `ModulatableSliderSpeedDeg`, `slitScanCorridor.rotationSpeed`
   - NO blend mode combo, NO blend intensity slider (this is a direct transform, not a blend)
3. Add call in `DrawMotionCategory`: `ImGui::Spacing(); DrawMotionSlitScanCorridor(e, modSources, categoryGlow);`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (MODIFY)

**Do**:

1. Add `#include "effects/slit_scan_corridor.h"` (alphabetical)
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SlitScanCorridorConfig, SLIT_SCAN_CORRIDOR_CONFIG_FIELDS)` with other config macros
3. Add `X(slitScanCorridor) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (alphabetical)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Effect appears in Motion category with "MOT" badge
- [x] Effect can be reordered via drag-drop
- [x] Enabling replaces the scene with a corridor of light from the scene slit
- [x] Speed/perspective/fog controls produce visible changes
- [x] Rotation rotates the corridor output without compounding drift
- [x] Preset save/load preserves all settings

---

## Implementation Notes

Changes made during implementation that deviate from the original plan:

### Removed: Decay

- `decayHalfLife` and `decayFactor` removed entirely — content persists at full brightness in the accumulation buffer
- User requirement: "I DONT WANT DECAY"
- Alpha forced to 1.0 in accumulation shader to prevent bleed-through

### Added: Fog

- `fogStrength` parameter (0.1-5.0, default 1.0) added to display shader
- Depth fog: center (small dist) = far/dim, edges (large dist) = near/bright
- Formula: `fog = pow(clamp(dist * 2.0, 0.0, 1.0), fogStrength)`

### Display Shader: Flat Walls Instead of 1/dist Pinch

- Original plan used corridor_warp-style `perspectiveStrength / dist` projection — created a vanishing-point pinch at center
- User wanted the 2001 stargate look: flat parallel walls, no pinch
- Replaced with gentle edge widening: `scale = 1.0 + perspective * dist * 2.0`
- `perspective=0`: flat walls (pure stargate), `perspective>0`: subtle vertical spread near edges
- Walls extend to fill full screen (clamp UV instead of black cutoff via wallEdge)

### Perspective Range

- Changed from 0.5-8.0 to 0.0-10.0 (default 0.3)
- 0.0 now valid (flat walls)

### Accumulation Shader: Linear Push

- Original plan had quadratic formula `sampleD = d / (1.0 + speedDt * d * perspective)` — too slow near center
- Replaced with linear: `shift = speedDt * (1.0 + d * perspective)`, `sampleD = max(halfPixel, d - shift)`
- Minimum half-pixel offset prevents left/right sides from reading across center boundary (fixes X-pattern color bleed)

### Slit Width: Source Band Separation

- Original plan: slitWidth controlled feathering radius of the injection smoothstep
- Final implementation: slitWidth (UV 0.001-1.0) controls how far apart the two source sample points are
- Injection zone is always ~2px narrow (fixed), independent of slitWidth
- Left wall samples from `slitPosition - slitWidth/2`, right wall from `slitPosition + slitWidth/2`
- Smooth remap within injection zone: `t = clamp(dx / injectionWidth, -1.0, 1.0)`
- `fract()` wraps source sampling for seamless position + width overflow

### Pipeline: Transform with Custom Render

- Added `RenderTexture2D *currentRenderDest` to PostEffect struct
- Pipeline sets `currentRenderDest = &pe->pingPong[writeIdx]` before calling render
- Render function writes directly to `currentRenderDest` (no intermediate RenderPass for non-blend effects)
- `render != nullptr` branch in render_pipeline.cpp now skips the final RenderPass unless EFFECT_FLAG_BLEND is set

### Removed from Plan

- Vertical vignette (top/bottom fade) — not needed with flat wall approach
- `corridorMap` texture binding on display shader — display shader reads from `texture0` (the ping-pong result drawn via RenderUtilsDrawFullscreenQuad)
- `dispCorridorMapLoc` — not needed
- `GetScratchShader` / `scratchShader` distinction — not needed since render writes directly
