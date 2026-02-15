# Inline Shader Setup Functions

Move setup functions from `shader_setup_{category}` files into each effect's `.cpp`, eliminating 20 files. Also make `GetGeneratorScratchPass` table-driven by adding scratch-pass fields to `EffectDescriptor`.

**Motivation**: With self-registering effects, the category shader_setup files are pure boilerplate. Each setup function is a 1-3 line adapter that unpacks `PostEffect*` fields and delegates to the effect module's `*EffectSetup()`. Moving them into the effect `.cpp` makes each effect fully self-contained.

---

## Design

### EffectDescriptor Struct Changes

Add two fields to `EffectDescriptor` in `effect_descriptor.h` (after `setup`):

| Field | Type | Purpose | Default |
|-------|------|---------|---------|
| `getScratchShader` | `GetShaderFn` | Returns the generator's own shader for scratch-pass rendering | `nullptr` |
| `scratchSetup` | `RenderPipelineShaderSetupFn` | Content setup function (binds uniforms for scratch pass) | `nullptr` |

Both default to `nullptr` for non-generators. C++20 default member initializers mean existing macro calls that don't provide these fields compile unchanged.

### Macro Changes

| Macro | Change |
|-------|--------|
| `REGISTER_EFFECT` | No signature change. New struct fields default to nullptr. |
| `REGISTER_EFFECT_CFG` | Same — no change. |
| `REGISTER_GENERATOR` | Add 6th parameter `ScratchSetupFn`. Forward-declare it. Generate `GetScratchShader_##field` returning `&pe->field.shader`. Pass both to descriptor. |
| `REGISTER_GENERATOR_FULL` | Same as REGISTER_GENERATOR — add `ScratchSetupFn` parameter. |
| `REGISTER_SIM_BOOST` | No signature change. New struct fields default to nullptr. |
| Manual (bloom) | No change needed. New struct fields default to nullptr. |

### GetGeneratorScratchPass Becomes Table-Driven

Move `GeneratorPassInfo` typedef and `GetGeneratorScratchPass` declaration from `shader_setup_generators.h` to `shader_setup.h`. Rewrite the implementation in `shader_setup.cpp` as a 2-line table lookup using `EFFECT_DESCRIPTORS[type].getScratchShader(pe)` and `EFFECT_DESCRIPTORS[type].scratchSetup`. Eliminates the 40-line switch statement.

### render_pipeline.cpp Changes

- Remove `#include "shader_setup_generators.h"`
- Replace `SetupAttractorLines(pe)` (line 322) with `EFFECT_DESCRIPTORS[effectType].scratchSetup(pe)` — the descriptor stores `SetupAttractorLines` as the scratch setup function

### Setup Function Migration Pattern

For each effect in a category file, move its `Setup*` function from `shader_setup_<category>.cpp` into the effect's own `.cpp` file, placed above the `REGISTER_*` macro. The macro already forward-declares the setup function, so no separate header declaration is needed.

**Four call-signature variants exist** (the adapter unpacks different fields from `PostEffect`):

| Variant | Extra args from PostEffect | Examples |
|---------|---------------------------|----------|
| Basic | `(Effect*, Config*)` | ColorGrade, Pixelation, Toon |
| Animated | `+ deltaTime` | Kaleidoscope, Glitch, SineWarp |
| Resolution | `+ screenWidth, screenHeight` | GradientFlow |
| Full | `+ deltaTime, screenWidth, screenHeight, fftTexture` | ToneWarp, CorridorWarp |

**Generators have paired functions**: `SetupXxx` (content, renders into scratch or private textures) and `SetupXxxBlend` (composites via `BlendCompositorApply`). Both move into the effect `.cpp`.

### Include Changes

**Remove** from ~28 effect `.cpp` files: `#include "render/shader_setup_<category>.h"`

**Add** to ~17 generator `.cpp` files: `#include "render/blend_compositor.h"` (for `BlendCompositorApply`)

### Files Deleted (20 total)

| Category | .cpp | .h |
|----------|------|----|
| artistic | `src/render/shader_setup_artistic.cpp` | `src/render/shader_setup_artistic.h` |
| cellular | `src/render/shader_setup_cellular.cpp` | `src/render/shader_setup_cellular.h` |
| color | `src/render/shader_setup_color.cpp` | `src/render/shader_setup_color.h` |
| generators | `src/render/shader_setup_generators.cpp` | `src/render/shader_setup_generators.h` |
| graphic | `src/render/shader_setup_graphic.cpp` | `src/render/shader_setup_graphic.h` |
| motion | `src/render/shader_setup_motion.cpp` | `src/render/shader_setup_motion.h` |
| optical | `src/render/shader_setup_optical.cpp` | `src/render/shader_setup_optical.h` |
| retro | `src/render/shader_setup_retro.cpp` | `src/render/shader_setup_retro.h` |
| symmetry | `src/render/shader_setup_symmetry.cpp` | `src/render/shader_setup_symmetry.h` |
| warp | `src/render/shader_setup_warp.cpp` | `src/render/shader_setup_warp.h` |

### CMakeLists.txt

Remove 10 entries from `RENDER_SOURCES`.

### Kept in shader_setup.cpp

Multi-pass helpers stay: `ApplyBloomPasses`, `ApplyAnamorphicStreakPasses`, `ApplyHalfResEffect`, `ApplyHalfResOilPaint`. Core non-transform setup functions stay: `SetupFeedback`, `SetupBlurH/V`, `SetupTrailBoost`, sim trail boosts, `SetupChromatic`, `SetupGamma`, `SetupClarity`.

---

## Tasks

### Wave 1: Move Setup Functions

All 6 tasks modify disjoint files — run in parallel.

#### Task 1.1: Update EffectDescriptor Struct and Generator Macros

**Files**: `src/config/effect_descriptor.h`
**Creates**: New struct fields and updated generator macro signatures that Wave 1 tasks depend on

**Do**:
- Add `getScratchShader` and `scratchSetup` fields to `EffectDescriptor` struct with `= nullptr` defaults (after the existing `setup` field)
- In `REGISTER_GENERATOR`: add 6th parameter `ScratchSetupFn`. Add forward declaration `void ScratchSetupFn(PostEffect *);`. Generate static function `GetScratchShader_##field` returning `&pe->field.shader`. Append `GetScratchShader_##field, ScratchSetupFn` to the EffectDescriptor initializer
- In `REGISTER_GENERATOR_FULL`: same changes as REGISTER_GENERATOR
- `REGISTER_EFFECT`, `REGISTER_EFFECT_CFG`, `REGISTER_SIM_BOOST`: no changes — the nullptr defaults handle the new fields

**Verify**: Compiles only after Wave 1 generators task also completes (macro signature change requires callers to update).

---

#### Task 1.2: Inline Generator Setup Functions

**Files**: All 17 generator `.cpp` files + `src/render/shader_setup_generators.cpp`
**Depends on**: Task 1.1 for updated macro signature

**Do**: For each generator (constellation, plasma, interference, solid_color, scan_bars, pitch_spiral, moire_generator, spectral_arcs, muons, filaments, slashes, glyph_field, arc_strobe, signal_frames, nebula, motherboard, attractor_lines):
1. Read the effect's `SetupXxx` and `SetupXxxBlend` functions from `shader_setup_generators.cpp`
2. Add both functions to the effect's `.cpp`, above the `REGISTER_GENERATOR*` macro call
3. Add `#include "render/blend_compositor.h"` for `BlendCompositorApply`
4. Update the `REGISTER_GENERATOR` or `REGISTER_GENERATOR_FULL` call to pass the content setup function as the new 6th argument (e.g., `SetupConstellation` for constellation)
5. Remove `#include "render/shader_setup_generators.h"` from the effect `.cpp`
6. Remove the corresponding functions from `shader_setup_generators.cpp`

After all moves: `shader_setup_generators.cpp` retains only `GetGeneratorScratchPass` (the switch) and its includes. The switch still compiles because it references functions that are now defined in effect `.cpp` files and declared via forward declarations in `shader_setup_generators.h`.

Follow `constellation.cpp` as reference for the overall file layout pattern.

**Verify**: Compiles at wave end.

---

#### Task 1.3: Inline Symmetry + Warp Setup Functions

**Files**: Effect `.cpp` files for symmetry and warp categories + `src/render/shader_setup_symmetry.cpp` + `src/render/shader_setup_warp.cpp`

**Do**: Read `shader_setup_symmetry.cpp` and `shader_setup_warp.cpp`. For each `Setup*` function:
1. Move the function body into the corresponding effect's `.cpp`, above its `REGISTER_EFFECT` macro call
2. Remove `#include "render/shader_setup_symmetry.h"` or `#include "render/shader_setup_warp.h"` from the effect `.cpp` (if present)
3. Remove the function from the category `.cpp` file

After all moves: both category `.cpp` files should be empty (only `#include` lines remain).

**Verify**: Compiles at wave end.

---

#### Task 1.4: Inline Cellular + Motion Setup Functions

**Files**: Effect `.cpp` files for cellular and motion categories + `src/render/shader_setup_cellular.cpp` + `src/render/shader_setup_motion.cpp`

**Do**: Same pattern as Task 1.3. Read both category `.cpp` files. Move each `Setup*` function into its effect's `.cpp`. Remove category header includes from effect files. Empty the category `.cpp` files.

**Verify**: Compiles at wave end.

---

#### Task 1.5: Inline Artistic + Graphic + Retro Setup Functions

**Files**: Effect `.cpp` files for artistic, graphic, and retro categories + `src/render/shader_setup_artistic.cpp` + `src/render/shader_setup_graphic.cpp` + `src/render/shader_setup_retro.cpp`

**Do**: Same pattern as Task 1.3. Read all three category `.cpp` files. Move each `Setup*` function into its effect's `.cpp`. Remove category header includes from effect files. Empty the category `.cpp` files.

**Verify**: Compiles at wave end.

---

#### Task 1.6: Inline Optical + Color Setup Functions

**Files**: Effect `.cpp` files for optical and color categories + `src/render/shader_setup_optical.cpp` + `src/render/shader_setup_color.cpp`

**Do**: Same pattern as Task 1.3. Read both category `.cpp` files. Move each `Setup*` function into its effect's `.cpp`. Remove category header includes from effect files. Empty the category `.cpp` files.

Note: `bloom.cpp` has manual registration (not a `REGISTER_EFFECT` macro). `SetupBloom` is forward-declared at the registration site. Moving the function definition above the registration makes the forward declaration redundant but harmless.

**Verify**: Compiles at wave end.

---

### Wave 2: Table-Driven + Cleanup

All 3 tasks modify disjoint files — run in parallel.

#### Task 2.1: Table-Driven GetGeneratorScratchPass + Pipeline Cleanup

**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`, `src/render/render_pipeline.cpp`
**Depends on**: Wave 1 complete (generators registered with scratchSetup)

**Do**:
- `shader_setup.h`: Add `GeneratorPassInfo` typedef (copied from `shader_setup_generators.h`) and `GetGeneratorScratchPass` declaration
- `shader_setup.cpp`: Add table-driven `GetGeneratorScratchPass` that returns `{*d.getScratchShader(pe), d.scratchSetup}` from `EFFECT_DESCRIPTORS[type]`
- `render_pipeline.cpp`: Remove `#include "shader_setup_generators.h"`. Replace `SetupAttractorLines(pe)` (line 322) with `EFFECT_DESCRIPTORS[effectType].scratchSetup(pe)`

**Verify**: Compiles.

---

#### Task 2.2: Delete Files + Update CMakeLists

**Files**: `CMakeLists.txt` + 20 files to delete

**Do**:
- Remove 10 entries from `RENDER_SOURCES` in `CMakeLists.txt`: `shader_setup_symmetry.cpp`, `shader_setup_warp.cpp`, `shader_setup_cellular.cpp`, `shader_setup_motion.cpp`, `shader_setup_artistic.cpp`, `shader_setup_graphic.cpp`, `shader_setup_retro.cpp`, `shader_setup_optical.cpp`, `shader_setup_color.cpp`, `shader_setup_generators.cpp`
- Delete all 20 files (10 `.cpp` + 10 `.h`) listed in the Files Deleted table above

**Verify**: Compiles.

---

#### Task 2.3: Update Add-Effect Skill

**Files**: `.claude/skills/add-effect`

**Do**: Remove any steps that reference `shader_setup_<category>.cpp` or `shader_setup_<category>.h` files. Setup functions are now defined inline in the effect `.cpp` above the `REGISTER_*` macro — update the skill to reflect this.

**Verify**: Read the skill and confirm no references to deleted files remain.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no errors
- [ ] No `shader_setup_<category>` files remain in `src/render/`
- [ ] `GetGeneratorScratchPass` is table-driven (no switch statement)
- [ ] `render_pipeline.cpp` has no `#include "shader_setup_generators.h"`
- [ ] All effects define their `Setup*` function in their own `.cpp`
