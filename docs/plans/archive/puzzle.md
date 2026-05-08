# Puzzle

Transform effect in the Novelty subcategory that converts the input image into a tiled jigsaw puzzle. Each cell holds a single locked piece with deterministic tabs and blanks that interlock with its neighbors. The input texture supplies the piece face (per-pixel in texture mode; per-cell flat fill in solid mode, like Voronoi). SDF-derived edge lighting gives the pieces a 3D pop. A `seed` uniform offsets the tab/blank hash so the puzzle layout can be shuffled or modulated. The reference's panning camera, cellSize pulse, and table-texture background are stripped — pieces are fully locked, the background is black, and the only animation comes from modulating the parameters.

**Research**: `docs/research/puzzle.md`

## Design

### Types

`src/effects/puzzle.h`:

```cpp
#ifndef PUZZLE_H
#define PUZZLE_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

struct PuzzleConfig {
  bool enabled = false;

  // Geometry
  float pieceCount = 12.0f;     // Pieces across screen height (4-40, integer slider)
  float seed = 0.0f;            // Hash offset for tab/blank pattern (0-100)

  // Color
  int fillMode = 0;             // 0 = Texture, 1 = Solid (cell-center color)

  // Lighting
  float edgeLight = 1.0f;       // Edge lighting strength (0-1)
};

#define PUZZLE_CONFIG_FIELDS                                           \
  enabled, pieceCount, seed, fillMode, edgeLight

typedef struct PuzzleEffect {
  Shader shader;
  int resolutionLoc;
  int pieceCountLoc;
  int seedLoc;
  int fillModeLoc;
  int edgeLightLoc;
} PuzzleEffect;

bool PuzzleEffectInit(PuzzleEffect *e);
void PuzzleEffectSetup(const PuzzleEffect *e,
                              const PuzzleConfig *cfg);
void PuzzleEffectUninit(const PuzzleEffect *e);
void PuzzleRegisterParams(PuzzleConfig *cfg);

PuzzleEffect *GetPuzzleEffect(PostEffect *pe);

#endif // PUZZLE_H
```

### Algorithm

The shader is a mechanical transcription of Bingle's reference (`docs/research/puzzle.md` "Reference Code") with the substitutions listed in the research doc's Algorithm section applied. Pieces are locked, so `getPieceMat` and the inverse-matrix transform collapse to a single subtraction. Inline the full GLSL below — agents implementing the shader task should copy this verbatim.

`shaders/puzzle.fs`:

```glsl
// Based on "Endless Puzzle" by Bingle
// https://www.shadertoy.com/view/N3sGD4
// License: CC BY-NC-SA 3.0 Unported
// Modified: stripped camera, cellSize pulse, rainbow tint, table texture, and
// vignette. Pieces are locked, input image supplies the piece face, edge
// lighting exposed as a uniform, seed shuffles the tab/blank pattern.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float pieceCount;     // Pieces across screen height (4-40)
uniform float seed;           // Hash offset for tab/blank pattern
uniform int fillMode;         // 0 = texture sample, 1 = solid cell-center color
uniform float edgeLight;      // Edge-lighting strength (0-1)

// Hash helpers (verbatim from reference: https://www.shadertoy.com/view/4djSRW).
vec3 hash33(vec3 p3) {
    p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float sdBox(in vec2 p, in vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float opSmoothUnion(float a, float b, float k) {
    k *= 4.0;
    float h = max(k - abs(a - b), 0.0);
    return min(a, b) - h * h * 0.25 / k;
}

float opSmoothSubtraction(float a, float b, float k) {
    return -opSmoothUnion(a, -b, k);
}

float pieceSDF(vec2 cell, vec2 p) {
    float dist = sdBox(p, vec2(0.5));
    const vec2[4] dirs = vec2[](vec2(1, 0), vec2(0, 1), vec2(-1, 0), vec2(0, -1));
    for (int i = 0; i < 4; i++) {
        bool dir = hash12(cell + dirs[i] * 0.5 + vec2(seed)) > 0.5;
        if (dir == (dirs[i].y < dirs[i].x)) {
            // Tab (sticks out toward neighbor).
            dist = opSmoothUnion(dist, distance(p, dirs[i] * 0.65) - 0.14, 0.025);
            dist = opSmoothUnion(dist, distance(p, dirs[i] * 0.5) - 0.1, 0.025);
        } else {
            // Blank (cuts in from neighbor).
            dist = opSmoothSubtraction(distance(p, dirs[i] * 0.35) - 0.15, dist, 0.02);
        }
    }
    return dist;
}

vec2 pieceSDFnorm(vec2 cell, vec2 p) {
    float e = 0.0001;
    float d = pieceSDF(cell, p);
    return vec2(d - pieceSDF(cell, p - vec2(e, 0)),
                d - pieceSDF(cell, p - vec2(0, e))) / e;
}

void main() {
    // Aspect-corrected centered world coords. Y axis spans pieceCount cells;
    // X axis spans pieceCount * (W/H) cells, so pieces stay square.
    vec2 aspect = vec2(resolution.x / resolution.y, 1.0);
    vec2 worldUV = (fragTexCoord - 0.5) * aspect * pieceCount;

    vec2 cell = floor(worldUV);

    // Background: black. The reference's table texture has no analog in a
    // transform pipeline.
    vec3 col = vec3(0.0);

    // 9-cell neighborhood loop (reference iterates i = 0..8).
    for (int i = 0; i < 9; i++) {
        vec2 testCell = cell + vec2(float(i % 3) - 1.0, float(i / 3) - 1.0);

        // Locked pieces: getPieceMat collapses to translate-only since
        // (cellSize - 1.0) zeroes out displacement and rotation.
        vec2 local = worldUV - (testCell + 0.5);

        float dist = pieceSDF(testCell, local);
        if (dist < 0.0) {
            // No rotation, so the inverse-transformed normal IS the raw SDF
            // gradient.
            vec2 norm = pieceSDFnorm(testCell, local);
            float edge = smoothstep(-0.075, 0.0, dist);

            // Sample input texture per fillMode.
            vec3 pieceCol;
            if (fillMode == 0) {
                // Texture mode: per-pixel sample from input at this fragment's UV.
                pieceCol = texture(texture0, fragTexCoord).rgb;
            } else {
                // Solid mode: per-cell sample at the cell-center mapped back
                // to UV. All pixels in this cell share one color.
                vec2 cellCenterUV = (testCell + 0.5) / (aspect * pieceCount) + 0.5;
                pieceCol = texture(texture0, cellCenterUV).rgb;
            }

            // Edge lighting: matches reference's mix(0.5, dot(...)*0.5+0.5, EDGELIGHT).
            float lit = dot(norm * edge * edge, vec2(1.0, 1.0)) * 0.5 + 0.5;
            pieceCol *= mix(0.5, lit, edgeLight);

            // Per-piece anti-alias blend. Reference uses iResolution.y/viewHeight
            // as pixels-per-world-unit; ours is resolution.y/pieceCount.
            float blend = min(-resolution.y * dist / pieceCount, 1.0);
            col = mix(col, pieceCol, blend);
        }
    }

    finalColor = vec4(col, 1.0);
}
```

Key implementation notes (already encoded in the GLSL above; called out here so agents catch them):

- The hash input `cell + dirs[i] * 0.5 + vec2(seed)` adds the seed offset to both neighboring cells consistently, since both cells compute the same shared-edge midpoint and add the same seed. Interlocking is preserved for any seed value.
- `dirs[i].y < dirs[i].x` flips polarity between neighboring shared edges: cell A's east edge condition is `0 < 1 = true`, cell B's west edge is `0 < -1 = false`, so the same hash value produces opposite tab/blank choices. One cell tabs, the other blanks - they interlock.
- The reference's `* 4.5` brightness multiplier and `mix(vec3(0.5), iChannel1, RAINBOW)` rainbow tint are removed entirely. The input texture is full-range; no scaling is needed.
- The reference's vignette is removed. The codebase has a separate Vignette transform users compose into the pipeline.
- Aspect correction is applied so cells are square regardless of screen aspect ratio. Without it, a wide screen would show stretched rectangular pieces.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `pieceCount` | float (int slider) | 4 - 40 | 12 | yes | `Pieces` |
| `seed` | float | 0.0 - 100.0 | 0.0 | yes | `Seed` |
| `fillMode` | int (combo) | 0 - 1 | 0 | no | `Fill Mode` ("Texture", "Solid") |
| `edgeLight` | float | 0.0 - 1.0 | 1.0 | yes | `Edge Light` |

UI section ordering (Signal Stack convention, matching LEGO Bricks):
1. **Geometry**: Pieces, Seed
2. **Color**: Fill Mode
3. **Lighting**: Edge Light

Modulatable params register with `ModEngineRegisterParam("puzzle.<field>", ...)` in `PuzzleRegisterParams()`.

### Constants

- Enum name: `TRANSFORM_PUZZLE`
- Display name: `"Puzzle"`
- Category badge: `"NOV"`
- Section index: `14` (Novelty, same as LEGO Bricks)
- Flags: `EFFECT_FLAG_NONE`
- Field name on `EffectConfig`: `puzzle`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create effect header

**Files**: `src/effects/puzzle.h`
**Creates**: `PuzzleConfig`, `PuzzleEffect`, `PUZZLE_CONFIG_FIELDS` macro, public function declarations.

**Do**: Create the file exactly as written in the Design section's Types subsection. Same structure as `src/effects/lego_bricks.h` (Config struct with inline range comments, `*_CONFIG_FIELDS` macro, Effect struct with shader handle and one `int *Loc` per uniform, four public function declarations + `Get<Name>Effect`). No animation accumulator field — pieces are static.

**Verify**: `cmake.exe --build build` compiles (header alone is includable; the `.cpp` referencing it does not exist yet, so this task may not link if the build picks up the header without its source. Acceptable if the only error is "missing definition for `PuzzleEffectInit`" etc. — that resolves in Wave 2.)

#### Task 1.2: Create fragment shader

**Files**: `shaders/puzzle.fs`
**Creates**: The puzzle-piece shader.

**Do**: Implement the GLSL exactly as written in the Design section's Algorithm subsection. Begin the file with the attribution comment block shown there (Bingle, Shadertoy URL, license, modification summary). Do not paraphrase, simplify, or "improve" the SDF math — copy the reference functions (`hash33`, `hash12`, `sdBox`, `opSmoothUnion`, `opSmoothSubtraction`, `pieceSDF`, `pieceSDFnorm`) verbatim. The `main()` function uses centered, aspect-corrected coordinates (per the codebase convention that `fragTexCoord` is bottom-left and must not be used raw for spatial math).

**Verify**: Shader compiles when loaded by raylib at runtime. Static check: `glslangValidator shaders/puzzle.fs` if available, otherwise rely on runtime load.

---

### Wave 2: Implementation

All Wave 2 tasks depend on Wave 1's header existing. They have no file overlap with each other and run in parallel.

#### Task 2.1: Implement effect module

**Files**: `src/effects/puzzle.cpp`
**Depends on**: Wave 1 (header + shader).

**Do**: Same structure as `src/effects/lego_bricks.cpp`:
- Include block: `puzzle.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_descriptor.h`, `imgui.h`, `render/post_effect.h`, `ui/modulatable_slider.h`, `<stddef.h>`. Keep the order alphabetical within each include group; clang-format will sort.
- `PuzzleEffectInit`: load `shaders/puzzle.fs`, return false if `shader.id == 0`, then `GetShaderLocation` for each uniform (`resolution`, `pieceCount`, `seed`, `fillMode`, `edgeLight`). Return true.
- `PuzzleEffectSetup`: bind `resolution` (vec2 from `GetScreenWidth/Height`), then each uniform from `cfg`. `fillMode` uses `SHADER_UNIFORM_INT`; the rest use `SHADER_UNIFORM_FLOAT`.
- `PuzzleEffectUninit`: `UnloadShader(e->shader)`.
- `PuzzleRegisterParams`: register `puzzle.pieceCount` (4-40), `puzzle.seed` (0-100), `puzzle.edgeLight` (0-1). Do NOT register `fillMode` (combo, not modulatable).
- `GetPuzzleEffect`: cast `pe->effectStates[TRANSFORM_PUZZLE]`.
- Bridge function `SetupPuzzle(PostEffect *pe)` (non-static) that calls `PuzzleEffectSetup(GetPuzzleEffect(pe), &pe->effects.puzzle)`.
- `// === UI ===` divider.
- Static `DrawPuzzleParams(EffectConfig *e, const ModSources *ms, ImU32 glow)`: `(void)glow;` then three `ImGui::SeparatorText` sections — "Geometry" (Pieces via `ModulatableSliderInt`, Seed via `ModulatableSlider` "%.2f"), "Color" (Fill Mode via `ImGui::Combo` with items `"Texture\0Solid\0"`), "Lighting" (Edge Light via `ModulatableSlider` "%.2f"). Use `##puzzle` suffix on all widget IDs to avoid collisions.
- Registration macro at the bottom, wrapped in `// clang-format off` / `// clang-format on`:

  ```cpp
  REGISTER_EFFECT(TRANSFORM_PUZZLE, Puzzle, puzzle,
                  "Puzzle", "NOV", 14, EFFECT_FLAG_NONE,
                  SetupPuzzle, NULL, DrawPuzzleParams)
  ```

The setup bridge function is non-static (referenced by name in the macro). The UI draw function is static. Do not add a named field to `PostEffect` — the descriptor's slot array handles state via `pe->effectStates[TRANSFORM_PUZZLE]`.

**Verify**: `cmake.exe --build build` compiles cleanly.

#### Task 2.2: Wire effect config

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 (header).

**Do**: Three additions, all alphabetical within their group:
1. Add `#include "effects/puzzle.h"` between `effects/dream_zoom.h` and `effects/escher_droste.h` (alphabetical; clang-format will re-sort if misplaced).
2. Add `TRANSFORM_PUZZLE,` to the `TransformEffectType` enum. Position is not significant for ordering (the constructor auto-populates `TransformOrderConfig::order` by iterating `0..COUNT`), so placement among other effects only affects internal numbering. Append it just before `TRANSFORM_ACCUM_COMPOSITE` (the last entry before `TRANSFORM_EFFECT_COUNT`), matching the convention used by recently-added effects like `TRANSFORM_PILLAR_GRID`.
3. Add `PuzzleConfig puzzle;` member to the `EffectConfig` struct, with a one-line preceding comment: `// Puzzle (jigsaw piece tiling with SDF tabs/blanks)`. Place it near the other Novelty effects (after `LegoBricksConfig legoBricks;` is fine; exact position does not matter for serialization).

`TransformOrderConfig::order` is populated by the constructor automatically — no manual array entry needed.

**Verify**: `cmake.exe --build build` compiles cleanly. Effect appears in the transform reorder UI.

#### Task 2.3: Wire preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 (header).

**Do**: Three additions:
1. Add `#include "effects/puzzle.h"` in alphabetical order with the other effect includes (between `dream_zoom.h` and `escher_droste.h`).
2. Add a `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PuzzleConfig, PUZZLE_CONFIG_FIELDS)` line in the "Effect configs A-G" block (alphabetical placement; just before `EscherDrosteConfig` is correct).
3. Add `X(puzzle)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro list. Place it on its own line near the other recent additions (the bottom of the macro is fine — order is not significant for serialization, only for human readability).

**Verify**: `cmake.exe --build build` compiles cleanly. Save a preset with the effect enabled and reload it; values round-trip correctly.

---

## Final Verification

- [ ] Build succeeds with no warnings (`cmake.exe --build build`).
- [ ] `Puzzle` appears in the Novelty section of the transform UI with the `NOV` badge.
- [ ] Toggling enabled adds it to the active pipeline list.
- [ ] All five params adjust in real time and visually match expectations:
  - `pieceCount`: 4 = giant pieces, 40 = mosaic.
  - `seed`: changing shuffles the tab/blank layout while preserving interlocking.
  - `fillMode`: Texture shows the input image through pieces; Solid gives Voronoi-style flat fills.
  - `edgeLight`: 0 = flat; 1 = full 3D pop.
- [ ] Pieces tile correctly across the screen with no visible gaps beyond hairline seams.
- [ ] No artifacts at screen edges (the 9-cell loop covers neighbors past the visible cell).
- [ ] Preset save/load round-trips all five params + `enabled`.
- [ ] Modulating `pieceCount`, `seed`, or `edgeLight` produces continuous visual change without crashes.
- [ ] Pieces remain square at any aspect ratio (verify by resizing the window).
