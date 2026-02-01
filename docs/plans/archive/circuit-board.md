# Circuit Board Warp

Warps input imagery along iteratively-folded triangle wave patterns, producing circuit-trace or maze-like distortion. The effect channels 90s PCB aesthetics through recursive UV folding.

## Specification

### Config Struct

```cpp
// src/config/circuit_board_config.h
#ifndef CIRCUIT_BOARD_CONFIG_H
#define CIRCUIT_BOARD_CONFIG_H

struct CircuitBoardConfig {
  bool enabled = false;
  float patternX = 7.0f;     // X component of pattern constant (1.0-10.0)
  float patternY = 5.0f;     // Y component of pattern constant (1.0-10.0)
  int iterations = 6;        // Recursion depth (3-12)
  float scale = 1.4f;        // Initial folding frequency (0.5-3.0)
  float offset = 0.16f;      // Initial offset between folds (0.05-0.5)
  float scaleDecay = 1.05f;  // Scale reduction per iteration (1.01-1.2)
  float strength = 0.5f;     // Warp intensity (0.0-1.0)
  float scrollSpeed = 0.0f;  // Animation scroll rate (0.0-2.0)
  bool chromatic = false;    // Per-channel iteration for RGB separation
};

#endif // CIRCUIT_BOARD_CONFIG_H
```

### Shader Algorithm

```glsl
// shaders/circuit_board.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 patternConst;  // (patternX, patternY)
uniform int iterations;
uniform float scale;
uniform float offset;
uniform float scaleDecay;
uniform float strength;
uniform float scrollOffset;  // CPU-accumulated scroll
uniform int chromatic;

vec2 triangleWave(vec2 p, vec2 c, float s) {
    return abs(fract((p + c) * s) - 0.5);
}

vec2 computeWarp(vec2 uv, vec2 c, int iters, float s, float off, float decay) {
    for (int i = 0; i < iters; i++) {
        uv = triangleWave(uv + off, c, s) + triangleWave(uv.yx, c, s);
        s /= decay;
        off /= decay;
        uv.y = -uv.y;
    }
    return uv;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 scrollUV = uv + vec2(scrollOffset * 0.5, scrollOffset * 0.33);

    if (chromatic == 1) {
        vec3 color;
        for (int c = 0; c < 3; c++) {
            float scaleOffset = float(c) * 0.1;
            vec2 warp = computeWarp(scrollUV, patternConst, iterations,
                                    scale + scaleOffset, offset, scaleDecay);
            vec2 finalUV = clamp(uv + strength * (warp - 0.5), 0.0, 1.0);
            color[c] = texture(texture0, finalUV)[c];
        }
        finalColor = vec4(color, 1.0);
    } else {
        vec2 warp = computeWarp(scrollUV, patternConst, iterations,
                                scale, offset, scaleDecay);
        vec2 finalUV = clamp(uv + strength * (warp - 0.5), 0.0, 1.0);
        finalColor = texture(texture0, finalUV);
    }
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| patternX | float | 1.0-10.0 | 7.0 | yes | Pattern X |
| patternY | float | 1.0-10.0 | 5.0 | yes | Pattern Y |
| iterations | int | 3-12 | 6 | no | Iterations |
| scale | float | 0.5-3.0 | 1.4 | yes | Scale |
| offset | float | 0.05-0.5 | 0.16 | yes | Offset |
| scaleDecay | float | 1.01-1.2 | 1.05 | no | Scale Decay |
| strength | float | 0.0-1.0 | 0.5 | yes | Strength |
| scrollSpeed | float | 0.0-2.0 | 0.0 | yes | Scroll Speed |
| chromatic | bool | - | false | no | Chromatic |

### Constants

- Enum value: `TRANSFORM_CIRCUIT_BOARD`
- Display name: `"Circuit Board"`
- Category: Warp (shader_setup_warp.cpp, imgui_effects_warp.cpp)

### Intentional Deviations from Research

<!-- Intentional deviation: Chromatic mode samples [c] instead of .r -->
**Chromatic channel sampling**: Research samples `.r` for all color components. Plan samples the corresponding channel `[c]`. This produces correct chromatic aberration with distinct R/G/B warp offsets.

<!-- Intentional deviation: Added clamp() to finalUV -->
**UV clamping**: Research mentions edge behavior as a consideration. Plan adds `clamp(finalUV, 0.0, 1.0)` to prevent out-of-bounds texture sampling at high strength values.

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create CircuitBoardConfig

**Files**: `src/config/circuit_board_config.h`
**Creates**: Config struct that all other files depend on

**Build**:
1. Create new header file with include guard `CIRCUIT_BOARD_CONFIG_H`
2. Define `CircuitBoardConfig` struct exactly as shown in Specification
3. All fields use default initializers

**Verify**: File exists and compiles when included.

---

### Wave 2: Integration (Parallel)

These 8 tasks modify different files with no overlap. Execute in parallel.

#### Task 2.1: Add include and member to EffectConfig

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.1

**Build**:
1. Add `#include "circuit_board_config.h"` in the include block (alphabetically near `chladni_warp_config.h`)
2. Add `CircuitBoardConfig circuitBoard;` member in `EffectConfig` struct (alphabetically near other warp configs)
3. Add `TRANSFORM_CIRCUIT_BOARD` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
4. Add `"Circuit Board"` to `TRANSFORM_EFFECT_NAMES` array at matching position
5. Add case in `IsTransformEnabled`: `case TRANSFORM_CIRCUIT_BOARD: return e->circuitBoard.enabled;`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Create shader file

**Files**: `shaders/circuit_board.fs`
**Depends on**: Task 1.1

**Build**:
1. Create shader file with exact algorithm from Specification
2. Uniforms: `patternConst` (vec2), `iterations` (int), `scale` (float), `offset` (float), `scaleDecay` (float), `strength` (float), `scrollOffset` (float), `chromatic` (int)

**Verify**: File exists.

#### Task 2.3: Add shader fields to PostEffect

**Files**: `src/render/post_effect.h`
**Depends on**: Task 1.1

**Build**:
1. Add `Shader circuitBoardShader;` in the shader block (near `chladniWarpShader`)
2. Add uniform location fields:
   - `int circuitBoardPatternConstLoc;`
   - `int circuitBoardIterationsLoc;`
   - `int circuitBoardScaleLoc;`
   - `int circuitBoardOffsetLoc;`
   - `int circuitBoardScaleDecayLoc;`
   - `int circuitBoardStrengthLoc;`
   - `int circuitBoardScrollOffsetLoc;`
   - `int circuitBoardChromaticLoc;`
3. Add `float circuitBoardScrollOffset;` in the accumulated time fields section (near `surfaceWarpScrollOffset`)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Load shader and get uniform locations

**Files**: `src/render/post_effect.cpp`
**Depends on**: Task 1.1

**Build**:
1. Add shader load: `pe->circuitBoardShader = LoadShader(0, "shaders/circuit_board.fs");` (near other warp shaders)
2. Add to validation check: `pe->circuitBoardShader.id != 0`
3. Get uniform locations:
   ```cpp
   pe->circuitBoardPatternConstLoc = GetShaderLocation(pe->circuitBoardShader, "patternConst");
   pe->circuitBoardIterationsLoc = GetShaderLocation(pe->circuitBoardShader, "iterations");
   pe->circuitBoardScaleLoc = GetShaderLocation(pe->circuitBoardShader, "scale");
   pe->circuitBoardOffsetLoc = GetShaderLocation(pe->circuitBoardShader, "offset");
   pe->circuitBoardScaleDecayLoc = GetShaderLocation(pe->circuitBoardShader, "scaleDecay");
   pe->circuitBoardStrengthLoc = GetShaderLocation(pe->circuitBoardShader, "strength");
   pe->circuitBoardScrollOffsetLoc = GetShaderLocation(pe->circuitBoardShader, "scrollOffset");
   pe->circuitBoardChromaticLoc = GetShaderLocation(pe->circuitBoardShader, "chromatic");
   ```
4. Add unload: `UnloadShader(pe->circuitBoardShader);`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Add SetupCircuitBoard function

**Files**: `src/render/shader_setup_warp.h`, `src/render/shader_setup_warp.cpp`
**Depends on**: Task 1.1

**Build**:
1. In `shader_setup_warp.h`: Add declaration `void SetupCircuitBoard(PostEffect *pe);`
2. In `shader_setup_warp.cpp`: Implement:
   ```cpp
   void SetupCircuitBoard(PostEffect *pe) {
     const CircuitBoardConfig *cb = &pe->effects.circuitBoard;

     pe->circuitBoardScrollOffset += pe->currentDeltaTime * cb->scrollSpeed;

     float patternConst[2] = {cb->patternX, cb->patternY};
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardPatternConstLoc,
                    patternConst, SHADER_UNIFORM_VEC2);
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardIterationsLoc,
                    &cb->iterations, SHADER_UNIFORM_INT);
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardScaleLoc,
                    &cb->scale, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardOffsetLoc,
                    &cb->offset, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardScaleDecayLoc,
                    &cb->scaleDecay, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardStrengthLoc,
                    &cb->strength, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardScrollOffsetLoc,
                    &pe->circuitBoardScrollOffset, SHADER_UNIFORM_FLOAT);
     int chromatic = cb->chromatic ? 1 : 0;
     SetShaderValue(pe->circuitBoardShader, pe->circuitBoardChromaticLoc,
                    &chromatic, SHADER_UNIFORM_INT);
   }
   ```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Add transform dispatch case

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Task 1.1

**Build**:
1. Add case in `GetTransformEffect` switch:
   ```cpp
   case TRANSFORM_CIRCUIT_BOARD:
     return {&pe->circuitBoardShader, SetupCircuitBoard,
             &pe->effects.circuitBoard.enabled};
   ```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.7: Add UI panel

**Files**: `src/ui/imgui_effects_warp.cpp`
**Depends on**: Task 1.1

**Build**:
1. Add static bool: `static bool sectionCircuitBoard = false;`
2. Add draw function:
   ```cpp
   static void DrawWarpCircuitBoard(EffectConfig *e, const ModSources *modSources,
                                    const ImU32 categoryGlow) {
     if (DrawSectionBegin("Circuit Board", categoryGlow, &sectionCircuitBoard)) {
       const bool wasEnabled = e->circuitBoard.enabled;
       ImGui::Checkbox("Enabled##circuitboard", &e->circuitBoard.enabled);
       if (!wasEnabled && e->circuitBoard.enabled) {
         MoveTransformToEnd(&e->transformOrder, TRANSFORM_CIRCUIT_BOARD);
       }
       if (e->circuitBoard.enabled) {
         ModulatableSlider("Pattern X##circuitboard", &e->circuitBoard.patternX,
                           "circuitBoard.patternX", "%.1f", modSources);
         ModulatableSlider("Pattern Y##circuitboard", &e->circuitBoard.patternY,
                           "circuitBoard.patternY", "%.1f", modSources);
         ImGui::SliderInt("Iterations##circuitboard", &e->circuitBoard.iterations,
                          3, 12);
         ModulatableSlider("Scale##circuitboard", &e->circuitBoard.scale,
                           "circuitBoard.scale", "%.2f", modSources);
         ModulatableSlider("Offset##circuitboard", &e->circuitBoard.offset,
                           "circuitBoard.offset", "%.3f", modSources);
         ImGui::SliderFloat("Scale Decay##circuitboard", &e->circuitBoard.scaleDecay,
                            1.01f, 1.2f, "%.3f");
         ModulatableSlider("Strength##circuitboard", &e->circuitBoard.strength,
                           "circuitBoard.strength", "%.2f", modSources);
         ModulatableSlider("Scroll Speed##circuitboard", &e->circuitBoard.scrollSpeed,
                           "circuitBoard.scrollSpeed", "%.2f", modSources);
         ImGui::Checkbox("Chromatic##circuitboard", &e->circuitBoard.chromatic);
       }
       DrawSectionEnd();
     }
   }
   ```
3. Call from `DrawWarpCategory`: Add `DrawWarpCircuitBoard(e, modSources, categoryGlow);` with `ImGui::Spacing();` before it (place alphabetically or at end)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.8: Add preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Task 1.1

**Build**:
1. Add `#include "circuit_board_config.h"` if not already pulled in via effect_config.h
2. Add `NLOHMANN_DEFINE_TYPE_INTRUSIVE` macro for `CircuitBoardConfig`:
   ```cpp
   NLOHMANN_DEFINE_TYPE_INTRUSIVE(CircuitBoardConfig, enabled, patternX, patternY,
                                  iterations, scale, offset, scaleDecay, strength,
                                  scrollSpeed, chromatic)
   ```
3. In `to_json(EffectConfig)`: Add `if (e.circuitBoard.enabled) { j["circuitBoard"] = e.circuitBoard; }`
4. In `from_json(EffectConfig)`: Add `e.circuitBoard = j.value("circuitBoard", e.circuitBoard);`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 3: Parameter Registration

#### Task 3.1: Register modulatable parameters

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add to `PARAM_TABLE`:
   ```cpp
   {"circuitBoard.patternX", {1.0f, 10.0f}},
   {"circuitBoard.patternY", {1.0f, 10.0f}},
   {"circuitBoard.scale", {0.5f, 3.0f}},
   {"circuitBoard.offset", {0.05f, 0.5f}},
   {"circuitBoard.strength", {0.0f, 1.0f}},
   {"circuitBoard.scrollSpeed", {0.0f, 2.0f}},
   ```
2. In `ParamRegistryInit`, add pointer registrations:
   ```cpp
   RegisterParam(reg, "circuitBoard.patternX", &e->circuitBoard.patternX);
   RegisterParam(reg, "circuitBoard.patternY", &e->circuitBoard.patternY);
   RegisterParam(reg, "circuitBoard.scale", &e->circuitBoard.scale);
   RegisterParam(reg, "circuitBoard.offset", &e->circuitBoard.offset);
   RegisterParam(reg, "circuitBoard.strength", &e->circuitBoard.strength);
   RegisterParam(reg, "circuitBoard.scrollSpeed", &e->circuitBoard.scrollSpeed);
   ```

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Application starts without shader load errors
- [ ] Circuit Board appears in Warp category UI panel
- [ ] Enabling effect produces visible circuit-trace distortion
- [ ] Iterations slider changes detail level
- [ ] Strength slider scales warp intensity
- [ ] Scroll Speed animates the pattern
- [ ] Chromatic checkbox produces RGB separation
- [ ] Pattern X/Y sliders change pattern character
- [ ] Parameters respond to LFO modulation
- [ ] Preset save/load preserves settings
