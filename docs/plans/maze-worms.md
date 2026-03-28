# Maze Worms

Autonomous worm agents draw colored trails that fill the screen in maze-like patterns. Each worm advances one pixel per step, steering according to a configurable turning strategy (spiral, wall-follow, wall-hug, or mixed chirality). Trails decay over time via the shared TrailMap infrastructure so the screen never saturates. Dead worms respawn at open positions after a cooldown. The visual ranges from meditative calligraphy to churning graffiti texture.

**Research**: `docs/research/maze_worms.md`

## Design

### Types

```c
typedef enum {
  MAZE_WORM_TURN_SPIRAL = 0,      // angle += curvature / age
  MAZE_WORM_TURN_WALL_FOLLOW = 1, // angle -= curvature / sqrt(age); while(collision) angle += turnAngle
  MAZE_WORM_TURN_WALL_HUG = 2,    // scan from angle-1.6 for nearest wall, back off by gap
  MAZE_WORM_TURN_MIXED = 3,       // angle += curvature * age * chirality (chirality = +/-1 per agent)
} MazeWormTurningMode;

typedef struct MazeWormAgent {
  float x;             // position in pixels
  float y;             // position in pixels
  float angle;         // heading in radians
  float age;           // step count (increments each step, starts at 1)
  float active;        // 1.0 = alive, 0.0 = dead (awaiting respawn)
  float hue;           // gradient LUT coordinate [0,1]
  float _pad[2];       // padding to 32 bytes
} MazeWormAgent;
```

```c
typedef struct MazeWormsConfig {
  bool enabled = false;
  int wormCount = 50;                                    // Number of simultaneous agents (4-200)
  MazeWormTurningMode turningMode = MAZE_WORM_TURN_SPIRAL; // Agent steering strategy
  float curvature = 1.0f;                                // Turning strength (0.1-10.0)
  float turnAngle = 0.5f;                                // Wall-follow collision avoidance step (0.05-1.0 rad)
  float trailWidth = 1.5f;                               // Smoothstep circle radius in pixels (0.5-5.0)
  float softness = 0.8f;                                 // Edge falloff: 0=hard, 1=full glow (0.0-1.0)
  float decayHalfLife = 8.0f;                             // Trail persistence in seconds (0.5-30.0)
  int diffusionScale = 1;                                // Trail blur radius in pixels (0-5)
  float respawnCooldown = 0.5f;                           // Seconds before dead worm respawns (0.0-5.0)
  int stepsPerFrame = 2;                                  // Agent steps per frame (1-8)
  float collisionGap = 2.0f;                              // Lookahead beyond trail width (1.0-5.0 pixels)
  float boostIntensity = 1.0f;                            // Blend intensity (0.0-2.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;        // Compositing mode
  ColorConfig color;                                      // Hue distribution for agent coloring
} MazeWormsConfig;
```

```c
typedef struct MazeWorms {
  unsigned int agentBuffer;       // SSBO for agent data
  unsigned int computeProgram;    // Agent update compute shader
  TrailMap *trailMap;             // Shared trail infrastructure (diffusion + decay)
  ColorLUT *colorLUT;             // Gradient texture for agent coloring
  int agentCount;                 // Current agent count (tracks config changes)
  int width;
  int height;

  // Compute shader uniform locations
  int resolutionLoc;
  int turningModeLoc;
  int curvatureLoc;
  int turnAngleLoc;
  int trailWidthLoc;
  int softnessLoc;
  int collisionGapLoc;
  int timeLoc;
  int gradientLUTLoc;

  float time;                     // Animation time accumulator
  float *respawnTimers;           // CPU-side per-agent cooldown timers (seconds remaining)
  MazeWormsConfig config;         // Cached config for change detection
  bool supported;
} MazeWorms;
```

### Algorithm

The compute shader runs per-agent with `local_size_x = 1024`. Each dispatch processes one step. The CPU dispatches `stepsPerFrame` times per frame with a memory barrier between each dispatch.

```glsl
#version 430

layout(local_size_x = 1024) in;

struct MazeWormAgent {
    float x, y;
    float angle;
    float age;
    float active;
    float hue;
    float _pad[2];
};

layout(std430, binding = 0) buffer AgentBuffer {
    MazeWormAgent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;

uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform int turningMode;
uniform float curvature;
uniform float turnAngle;
uniform float trailWidth;
uniform float softness;
uniform float collisionGap;
uniform float time;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= agents.length()) return;

    MazeWormAgent agent = agents[idx];
    if (agent.active < 0.5) return;

    float a = agent.angle;

    // Turning strategy
    if (turningMode == 0) {
        // SPIRAL: angle += curvature / age
        a += curvature / agent.age;
    } else if (turningMode == 1) {
        // WALL_FOLLOW: angle -= curvature / sqrt(age); while(collision) angle += turnAngle
        a -= curvature / sqrt(agent.age);
        int maxAttempts = int(6.2832 / turnAngle) + 1;
        int attempts = 0;
        while (imageLoad(trailMap, ivec2(agent.x + (trailWidth + collisionGap) * cos(a),
                                         agent.y + (trailWidth + collisionGap) * sin(a))).w > 0.0
               && attempts < maxAttempts) {
            a += turnAngle;
            attempts++;
        }
        if (attempts >= maxAttempts) {
            agents[idx].active = 0.0;
            return;
        }
    } else if (turningMode == 2) {
        // WALL_HUG: scan from angle-1.6 for nearest wall, back off
        float a0 = a - 1.6;
        float aSearch = a0;
        int maxScan = int(6.2832 / turnAngle) + 1;
        int scans = 0;
        while (imageLoad(trailMap, ivec2(agent.x + (trailWidth + collisionGap) * cos(aSearch),
                                         agent.y + (trailWidth + collisionGap) * sin(aSearch))).w == 0.0
               && scans < maxScan) {
            aSearch += turnAngle;
            scans++;
        }
        a = max(a0, aSearch - 4.0 * turnAngle);
        if (imageLoad(trailMap, ivec2(agent.x + (trailWidth + collisionGap) * cos(a),
                                      agent.y + (trailWidth + collisionGap) * sin(a))).w > 0.0) {
            agents[idx].active = 0.0;
            return;
        }
    } else if (turningMode == 3) {
        // MIXED: angle += curvature * age * chirality
        // sign(sin(x)) for half-integer x produces clustered chirality groups per reference
        float chirality = sign(sin(float(idx) + 0.5));
        a += curvature * 0.001 * agent.age * chirality;
        if (imageLoad(trailMap, ivec2(agent.x + (trailWidth + collisionGap) * cos(a),
                                      agent.y + (trailWidth + collisionGap) * sin(a))).w > 0.0) {
            agents[idx].active = 0.0;
            return;
        }
    }

    // Collision check for SPIRAL mode (other modes check inline above)
    if (turningMode == 0) {
        if (imageLoad(trailMap, ivec2(agent.x + (trailWidth + collisionGap) * cos(a),
                                      agent.y + (trailWidth + collisionGap) * sin(a))).w > 0.0) {
            agents[idx].active = 0.0;
            return;
        }
    }

    // Move
    float newX = agent.x + cos(a);
    float newY = agent.y + sin(a);

    // Bounds check: deactivate if outside margins
    float margin = trailWidth + collisionGap;
    if (newX < margin || newX > resolution.x - margin ||
        newY < margin || newY > resolution.y - margin) {
        agents[idx].active = 0.0;
        return;
    }

    // Write smoothstep circle to trail texture
    int r = int(ceil(trailWidth));
    vec3 rgb = texture(gradientLUT, vec2(agent.hue, 0.5)).rgb;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            float dist = length(vec2(float(dx), float(dy)));
            float innerEdge = trailWidth * (1.0 - softness);
            float alpha = smoothstep(trailWidth, innerEdge, dist);
            if (alpha > 0.0) {
                ivec2 writePos = ivec2(newX + float(dx), newY + float(dy));
                if (writePos.x >= 0 && writePos.x < int(resolution.x) &&
                    writePos.y >= 0 && writePos.y < int(resolution.y)) {
                    vec4 existing = imageLoad(trailMap, writePos);
                    vec4 deposit = vec4(rgb * alpha, alpha);
                    imageStore(trailMap, writePos, existing + deposit);
                }
            }
        }
    }

    // Update agent state
    agents[idx].x = newX;
    agents[idx].y = newY;
    agents[idx].angle = mod(a, 6.2832);
    agents[idx].age = agent.age + 1.0;
}
```

### CPU-side Respawn

Each frame, after compute dispatch, the CPU handles respawn for dead agents:

1. Read trail texture once per frame via `glGetTexImage` into a CPU buffer (RGBA float, full resolution).
2. For each agent with `active == 0.0`:
   - Decrement `respawnTimers[i]` by `deltaTime`. If still positive, skip.
   - Try up to 16 random positions within the screen (minus margins).
   - For each candidate, check the CPU trail buffer at that pixel: if alpha < 0.01, position is clear.
   - If clear: set agent's x, y to the position, random angle, age = 1.0, active = 1.0, hue via `ColorConfigAgentHue()`.
   - If all 16 attempts fail, reset `respawnTimers[i]` to one frame (try again next frame).
3. Upload modified agents to SSBO via `rlUpdateShaderBuffer`.

The trail buffer is allocated once in `MazeWormsInit` and reused each frame. The `respawnTimers` array is allocated alongside it.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| wormCount | int | 4-200 | 50 | No | Worm Count |
| turningMode | enum | 0-3 | 0 (SPIRAL) | No | Turning Mode |
| curvature | float | 0.1-10.0 | 1.0 | Yes | Curvature |
| turnAngle | float | 0.05-1.0 | 0.5 | Yes | Turn Angle |
| trailWidth | float | 0.5-5.0 | 1.5 | Yes | Trail Width |
| softness | float | 0.0-1.0 | 0.8 | Yes | Softness |
| decayHalfLife | float | 0.5-30.0 | 8.0 | Yes | Decay Half-Life |
| diffusionScale | int | 0-5 | 1 | No | Diffusion |
| respawnCooldown | float | 0.0-5.0 | 0.5 | Yes | Respawn Cooldown |
| stepsPerFrame | int | 1-8 | 2 | No | Steps/Frame |
| collisionGap | float | 1.0-5.0 | 2.0 | Yes | Collision Gap |
| boostIntensity | float | 0.0-2.0 | 1.0 | Yes | Boost Intensity |
| blendMode | enum | BlendMode | SCREEN | No | Blend Mode |

### Constants

- Enum name: `TRANSFORM_MAZE_WORMS`
- Display name: `"Maze Worms"`
- Category badge: `"SIM"` (section 9)
- Flags: `EFFECT_FLAG_SIM_BOOST`
- Config field: `mazeWorms`
- Compute shader path: `shaders/maze_worm_agents.glsl`

---

## Tasks

### Wave 1: Header + Shader

#### Task 1.1: MazeWorms header

**Files**: `src/simulation/maze_worms.h` (create)
**Creates**: `MazeWormsConfig`, `MazeWormAgent`, `MazeWorms` structs, `MazeWormTurningMode` enum, public function declarations, `MAZE_WORMS_CONFIG_FIELDS` macro

**Do**: Follow `physarum.h` as the structural template. Define all types from the Design section. Include `bounds_mode.h` is NOT needed (no bounds modes - agents deactivate at borders). Include `render/color_config.h` for `ColorConfig`, `render/blend_mode.h` for `EffectBlendMode`. Public functions:
- `MazeWormsSupported()` - check OpenGL 4.3
- `MazeWormsInit(int width, int height, const MazeWormsConfig *config)` - returns `MazeWorms*`
- `MazeWormsUninit(MazeWorms *mw)`
- `MazeWormsUpdate(MazeWorms *mw, float deltaTime)` - no accumTexture or fftTexture params (not used)
- `MazeWormsProcessTrails(MazeWorms *mw, float deltaTime)`
- `MazeWormsResize(MazeWorms *mw, int width, int height)`
- `MazeWormsReset(MazeWorms *mw)`
- `MazeWormsRegisterParams(MazeWormsConfig *cfg)`
- `MazeWormsApplyConfig(MazeWorms *mw, const MazeWormsConfig *newConfig)`

Forward-declare `TrailMap` with `typedef struct TrailMap TrailMap;` and `ColorLUT` with `typedef struct ColorLUT ColorLUT;`.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

#### Task 1.2: Compute shader

**Files**: `shaders/maze_worm_agents.glsl` (create)
**Creates**: The agent update compute shader

**Do**: Implement the Algorithm section verbatim. Add the attribution comment block at the top before `#version 430`:
```
// Based on "maze worms / graffitis 3" series by FabriceNeyret2
// https://www.shadertoy.com/view/XdjcRD
// License: CC BY-NC-SA 3.0 Unported
// Modified: Ported from Shadertoy fragment feedback to compute shader with SSBO agents and imageStore trail writes
```

**Verify**: File exists at `shaders/maze_worm_agents.glsl`.

---

### Wave 2: Implementation + Integration

#### Task 2.1: MazeWorms implementation

**Files**: `src/simulation/maze_worms.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `physarum.cpp` as the structural template. Implement all public functions declared in the header.

Key differences from Physarum:
- `MazeWormsInit`: Allocate `MazeWorms` via `calloc`. Load compute shader via `SimLoadShaderSource` + `rlCompileShader` + `rlLoadComputeShaderProgram`. Create `TrailMap` via `TrailMapInit`. Create `ColorLUT` via `ColorLUTInit(&config->color)`. Initialize agents with random positions, random angles, `age = 1.0`, `active = 1.0`, hue via `ColorConfigAgentHue`. Upload SSBO via `rlLoadShaderBuffer`. Allocate `respawnTimers` array (calloc, `wormCount * sizeof(float)`). Allocate CPU trail readback buffer (`width * height * 4 * sizeof(float)`). Cache all uniform locations.
- `MazeWormsUpdate`: Loop `stepsPerFrame` times: `rlEnableShader`, set uniforms, bind `ColorLUT` texture to unit 0 via `glActiveTexture(GL_TEXTURE0)` + `glBindTexture(GL_TEXTURE_2D, ColorLUTGetTexture(mw->colorLUT).id)` + `glUniform1i(mw->gradientLUTLoc, 0)`, `rlBindShaderBuffer(agentBuffer, 0)`, `rlBindImageTexture(trailMap primary texture id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false)`, `rlComputeShaderDispatch(ceil(agentCount/1024), 1, 1)`, `glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT)`, `rlDisableShader`. After all steps, call the CPU respawn routine.
- CPU respawn routine: `glGetTexImage` to read trail texture into CPU buffer. Read agent data from SSBO via `rlReadShaderBuffer`. Loop agents: if `active == 0.0`, decrement timer, try random positions, check trail buffer alpha, reactivate if clear. Upload modified agents via `rlUpdateShaderBuffer`.
- `MazeWormsProcessTrails`: Delegate to `TrailMapProcess(mw->trailMap, deltaTime, cfg->decayHalfLife, cfg->diffusionScale)`.
- `MazeWormsApplyConfig`: Detect `wormCount` changes (reallocate SSBO, respawnTimers, readback buffer). Update `ColorLUT` via `ColorLUTUpdate(mw->colorLUT, &newConfig->color)`. Detect color changes (reinitialize agent hues).
- `MazeWormsReset`: Clear trail map, reinitialize all agents.
- `MazeWormsUninit`: Free SSBO, trail map, `ColorLUTUninit(mw->colorLUT)`, compute program, respawnTimers, readback buffer, struct.

Colocated UI section (`// === UI ===`):
- `static void DrawMazeWormsParams(EffectConfig*, const ModSources*, ImU32)`:
  - SeparatorText "Simulation"
  - `ImGui::SliderInt` for Worm Count (4-200)
  - `ImGui::Combo` for Turning Mode (Spiral/Wall Follow/Wall Hug/Mixed)
  - `ModulatableSlider` for Curvature, Turn Angle, Trail Width, Softness, Collision Gap
  - `ImGui::SliderInt` for Steps/Frame (1-8)
  - SeparatorText "Trail"
  - `ModulatableSlider` for Decay Half-Life, Respawn Cooldown
  - `ImGui::SliderInt` for Diffusion (0-5)
  - SeparatorText "Output"
  - `ModulatableSlider` for Boost Intensity (0.0-2.0)
  - `ImGui::Combo` for Blend Mode (use `DrawBlendModeCombo` from `render/blend_mode.h`)
  - `ImGuiDrawColorMode` for color config

Bridge function and registration:
```cpp
void SetupMazeWormsTrailBoost(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, TrailMapGetTexture(pe->mazeWorms->trailMap),
      pe->effects.mazeWorms.boostIntensity, pe->effects.mazeWorms.blendMode);
}

// clang-format off
REGISTER_SIM_BOOST(TRANSFORM_MAZE_WORMS, mazeWorms, "Maze Worms",
                   SetupMazeWormsTrailBoost, MazeWormsRegisterParams, DrawMazeWormsParams)
// clang-format on
```

Includes: `"maze_worms.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/effect_descriptor.h"`, `"external/glad.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/color_config.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"rlgl.h"`, `"shader_utils.h"`, `"trail_map.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<math.h>`, `<stdlib.h>`, `<string.h>`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "simulation/maze_worms.h"` in the includes (near `physarum.h`)
2. Add `TRANSFORM_MAZE_WORMS,` to `TransformEffectType` enum, after `TRANSFORM_BOIDS` (with the other simulations)
3. Add `TRANSFORM_MAZE_WORMS` to the `TransformOrderConfig::order` initialization (it auto-initializes from enum order, so just placing the enum correctly is sufficient)
4. Add `MazeWormsConfig mazeWorms;` member to `EffectConfig` struct, after the Boids config comment block

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do** in `post_effect.h`:
1. Add `typedef struct MazeWorms MazeWorms;` forward declaration (near other simulation forward declarations)
2. Add `MazeWorms *mazeWorms;` pointer member in `PostEffect` struct (after `Boids *boids;`)

**Do** in `post_effect.cpp`:
1. Add `#include "simulation/maze_worms.h"` in includes
2. In `PostEffectInit`: add `pe->mazeWorms = MazeWormsInit(screenWidth, screenHeight, NULL);` after the other simulation inits
3. In `PostEffectUninit`: add `MazeWormsUninit(pe->mazeWorms);` after other simulation uninits
4. In `PostEffectResize`: add `MazeWormsResize(pe->mazeWorms, width, height);` after other simulation resizes
5. In `PostEffectClearFeedback`: add `if (pe->effects.mazeWorms.enabled) { MazeWormsReset(pe->mazeWorms); }` after other simulation resets

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "simulation/maze_worms.h"` in includes
2. Add `static void ApplyMazeWormsPass(PostEffect *pe, float deltaTime)` function following the exact pattern of `ApplyPhysarumPass`:
   - NULL-check `pe->mazeWorms`
   - Always call `MazeWormsApplyConfig(pe->mazeWorms, &pe->effects.mazeWorms)`
   - If enabled: call `MazeWormsUpdate(pe->mazeWorms, deltaTime)` then `MazeWormsProcessTrails(pe->mazeWorms, deltaTime)`
3. Add `ApplyMazeWormsPass(pe, deltaTime);` to `ApplySimulationPasses`, after `ApplyBoidsPass`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Serialization + build system

**Files**: `src/config/effect_serialization.cpp` (modify), `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do** in `effect_serialization.cpp`:
1. Add `#include "simulation/maze_worms.h"` in includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MazeWormsConfig, MAZE_WORMS_CONFIG_FIELDS)` near other simulation config macros
3. Add `X(mazeWorms) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (near `physarum`, `boids`)

**Do** in `CMakeLists.txt`:
1. Add `src/simulation/maze_worms.cpp` to `SIMULATION_SOURCES` (after `boids.cpp`)

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Maze Worms appears in Simulation category (section 9) with "SIM" badge
- [ ] All four turning modes produce distinct visual patterns
- [ ] Worms die on trail collision and border contact
- [ ] Dead worms respawn at open positions after cooldown
- [ ] Trail decays over time (adjustable via Decay Half-Life)
- [ ] stepsPerFrame > 1 produces visibly faster growth
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters

---

## Implementation Notes

Deviations from the original plan during implementation:

- **GPU-side respawn** instead of CPU-side readback. Respawn logic runs in the compute shader using hash-based random positions and `imageLoad` to check for open space. Avoids `glGetTexImage` GPU sync.
- **`active` renamed to `alive`** in the GLSL agent struct. `active` is a GLSL reserved word.
- **`respawnTimer` field** added to agent struct (replaced one `_pad` float). GPU respawn needs per-agent cooldown tracking.
- **`TrailMapClear` changed to use `BLANK`** instead of `BLACK`. Trail alpha=1.0 from BLACK caused all agents to detect phantom collisions on init.
- **Collision threshold 0.1** instead of 0.0. Diffusion spreads small alpha values; a threshold prevents false collisions from haze.
- **Border sensing in `isWall`**. The original Shadertoy draws border walls on the first frame. Our `isWall` function treats screen margins as walls directly, giving wall-follow and wall-hug something to navigate at screen edges.
- **Softness parameter removed**. At 1-3px trail widths there is no room for meaningful edge falloff. Every implementation either changed perceived thickness or was indistinguishable from trail width.
- **Wall Hug uses same logic as Wall Follow** with reversed curve and dodge directions. The original Shadertoy wall-hug scan algorithm requires persistent (zero-decay) trails to function. Our decay-based system leaves the screen mostly empty, so the scan-based approach failed. Mirrored wall-follow produces consistent results at any decay setting.
- **Collision gap controls forward probe reach only**. The gap is enforced by the single-point directional probe (`trailWidth + 1.0 + collisionGap`). It creates spacing when worms approach trails head-on but does not prevent side-by-side proximity. Multi-point and invisible-alpha approaches were attempted and removed because they caused self-collision or broke wall-following.
- **Curvature min lowered to 0.01** (from 0.1) so dual spiral (mixed) worms can make gentler arcs and live longer.
- **Diffusion defaults to 0** (from 1). Non-zero diffusion spreads alpha and causes false collisions.
- **Turn Angle** uses `ModulatableSliderAngleDeg` with standard `ROTATION_OFFSET_MAX` range. Only visible in UI when Wall Follow or Wall Hug is selected.
- **`ColorLUT` gradient texture** for agent coloring instead of `hsv2rgb` in the compute shader, following the CurlFlow/AttractorFlow pattern.
- **No debug overlay**. Not added per project policy.
