# Lint Cleanup & Prevention

Clear ~500 lint warnings across clang-tidy, cppcheck, and lizard. Delete dead code, apply mechanical fixes, modernize casts, fix real bugs, then add pre-commit clang-tidy so new warnings can't land.

**Research**: `docs/research/lint-cleanup.md`

## Design

### Strategy

Work in 5 waves: infrastructure first, then dead code deletion, then mechanical fixes grouped by directory (parallel), then targeted bug fixes, then prevention. Each wave depends on the previous to avoid edit conflicts on shared files.

### Warning Categories and Fix Rules

| Category | Fix |
|----------|-----|
| `unusedFunction` (ConfigDefault) | Delete declaration from `.h`, definition from `.cpp` |
| `unusedFunction` (TrailMapDraw) | Delete declaration from `.h`, definition from `.cpp` |
| `unusedFunction` (other) | Delete declaration from `.h`, definition from `.cpp` |
| `misc-const-correctness` / `constVariable` / `constParameterPointer` | Add `const` qualifier |
| `readability-braces-around-statements` | Wrap body in `{}` |
| `readability-isolate-declaration` | Split to one declaration per line |
| `passedByValue` | Change `Type param` to `const Type& param` |
| `cstyleCast` | `(Type*)expr` → `static_cast<Type*>(expr)` |
| `dangerousTypeCast` | Use `reinterpret_cast` with comment explaining offset arithmetic |
| `misc-unused-parameters` | Add `(void)param;` for callback signatures, remove param if signature is ours |
| `memsetClassFloat` | Replace `memset(x, 0, sizeof(X))` with `*x = X{}` |
| `bugprone-integer-division` | Cast numerator to float before division |
| `bugprone-incorrect-roundings` | Use `lround()` instead of `(int)(x + 0.5)` |
| `bugprone-implicit-widening-of-multiplication-result` | Cast to target type before multiply |
| `uninitvar` | Initialize the variable |
| `cert-err33-c` | Check return value or cast to `(void)` |
| `readability-implicit-bool-conversion` | Use explicit `!= nullptr` or `!= 0` |
| `readability-avoid-nested-conditional-operator` | Refactor to if/else |
| `unreadVariable` (nlohmann macro) | Suppress in cppcheck config |
| `unreadVariable` (real — `padY`) | Delete or use the variable |
| `constParameterCallback` | Suppress in cppcheck — structural false positive (typedef forces non-const, many callbacks genuinely mutate) |
| `readability-function-size` | Skip — most are UI functions, expected verbosity per conventions |
| Lizard complexity | Skip — same reason |

### Cppcheck Suppressions

Add to `scripts/lint.sh` cppcheck invocation:
```
--suppress=unreadVariable:*effect_serialization.cpp
--suppress=constParameterCallback
```

### Conventions Changes

Remove from conventions.md:
- `<Name>ConfigDefault()` from the effect module functions list

Add to conventions.md Code Style section:
- Variables that are not reassigned must be declared `const`
- All `if`/`else`/`for`/`while` bodies must be wrapped in `{}`
- Use `static_cast` / `reinterpret_cast` instead of C-style casts
- Pass struct parameters by `const T&`, not by value

### Add-Effect Skill Changes

Remove from `.claude/skills/add-effect/SKILL.md`:
- The `ConfigDefault` declaration from the Phase 1 header template
- The `ConfigDefault` definition from the Phase 1 source template
- The `ConfigDefault` mention from the File Summary table

### Pre-commit Hook Extension

Append to existing `.git/hooks/pre-commit` (after the clang-format section):

```bash
# --- clang-tidy on staged C++ files ---
if command -v clang-tidy.exe &> /dev/null; then
    CLANG_TIDY="clang-tidy.exe"
elif command -v clang-tidy &> /dev/null; then
    CLANG_TIDY="clang-tidy"
else
    exit 0
fi

COMPILE_DB="build/compile_commands.json"
if [ ! -f "$COMPILE_DB" ]; then
    echo "Warning: compile_commands.json not found, skipping clang-tidy"
    exit 0
fi

STAGED_CPP=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.cpp$')
if [ -n "$STAGED_CPP" ]; then
    TIDY_FAILED=0
    for FILE in $STAGED_CPP; do
        if [ -f "$FILE" ]; then
            OUTPUT=$($CLANG_TIDY -p build "$FILE" 2>&1 | grep 'warning:')
            if [ -n "$OUTPUT" ]; then
                echo "$OUTPUT"
                TIDY_FAILED=1
            fi
        fi
    done

    if [ $TIDY_FAILED -ne 0 ]; then
        echo ""
        echo "clang-tidy: warnings found in staged files. Fix or commit with --no-verify to skip."
        exit 1
    fi
fi
```

---

## Tasks

### Wave 1: Infrastructure & Documentation

#### Task 1.1: Update add-effect skill template

**Files**: `.claude/skills/add-effect/SKILL.md`
**Creates**: Updated skill template without dead ConfigDefault pattern

**Do**:
- In the Phase 1 header template, delete the line: `{EffectName}Config {EffectName}ConfigDefault(void);`
- In the Phase 1 source template, delete the `ConfigDefault` function definition (the 3 lines: function signature, return statement, closing brace)
- In the File Summary table row for `.cpp`, remove "ConfigDefault, " from the Changes column
- Change "5-6 public functions" to "4-5 public functions" if that phrasing appears

**Verify**: Skill template no longer mentions ConfigDefault.

---

#### Task 1.2: Update conventions

**Files**: `docs/conventions.md`

**Do**:
- In the **Effect Module Functions** list, delete the bullet for `<Name>ConfigDefault()`
- In the **Code Style** section, add a new subsection **Const-Correctness & Modern C++** with these rules:
  - Variables that are not reassigned must be declared `const`
  - All `if`/`else`/`for`/`while` bodies must be wrapped in `{}`
  - Use `static_cast` / `reinterpret_cast` instead of C-style casts
  - Pass struct parameters by `const T&`, not by value

**Verify**: `ConfigDefault` no longer appears in conventions.md. New rules are present.

---

#### Task 1.3: Update cppcheck config

**Files**: `scripts/lint.sh`

**Do**:
- Add these flags to the cppcheck invocation (after the existing `--suppress` lines):
  - `--suppress=unreadVariable:*effect_serialization.cpp`
  - `--suppress=constParameterCallback`
- These suppress: 126 nlohmann macro false positives and 49 structural false positives from the `RenderPipelineShaderSetupFn` typedef

**Verify**: `scripts/lint.sh` has the new suppress flags.

---

### Wave 2: Dead Code Deletion

#### Task 2.1: Delete ConfigDefault from effects (A-M)

**Files**: `src/effects/anamorphic_streak.{h,cpp}`, `src/effects/arc_strobe.{h,cpp}`, `src/effects/ascii_art.{h,cpp}`, `src/effects/attractor_lines.{h,cpp}`, `src/effects/bit_crush.{h,cpp}`, `src/effects/bloom.{h,cpp}`, `src/effects/bokeh.{h,cpp}`, `src/effects/byzantine.{h,cpp}`, `src/effects/chladni.{h,cpp}`, `src/effects/chladni_warp.{h,cpp}`, `src/effects/chromatic_aberration.{h,cpp}`, `src/effects/circuit_board.{h,cpp}`, `src/effects/color_grade.{h,cpp}`, `src/effects/constellation.{h,cpp}`, `src/effects/corridor_warp.{h,cpp}`, `src/effects/cross_hatching.{h,cpp}`, `src/effects/crt.{h,cpp}`, `src/effects/curl_advection.{h,cpp}`, `src/effects/data_traffic.{h,cpp}`, `src/effects/density_wave_spiral.{h,cpp}`, `src/effects/digital_shard.{h,cpp}`, `src/effects/disco_ball.{h,cpp}`, `src/effects/domain_warp.{h,cpp}`, `src/effects/dot_matrix.{h,cpp}`, `src/effects/droste_zoom.{h,cpp}`, `src/effects/false_color.{h,cpp}`, `src/effects/faraday.{h,cpp}`, `src/effects/filaments.{h,cpp}`, `src/effects/fireworks.{h,cpp}`, `src/effects/flip_book.{h,cpp}`, `src/effects/flux_warp.{h,cpp}`, `src/effects/fracture_grid.{h,cpp}`, `src/effects/galaxy.{h,cpp}`, `src/effects/glitch.{h,cpp}`, `src/effects/glyph_field.{h,cpp}`, `src/effects/gradient_flow.{h,cpp}`, `src/effects/halftone.{h,cpp}`, `src/effects/heightfield_relief.{h,cpp}`, `src/effects/hex_rush.{h,cpp}`, `src/effects/hue_remap.{h,cpp}`, `src/effects/impressionist.{h,cpp}`, `src/effects/infinite_zoom.{h,cpp}`, `src/effects/ink_wash.{h,cpp}`, `src/effects/interference_warp.{h,cpp}`, `src/effects/iris_rings.{h,cpp}`, `src/effects/kaleidoscope.{h,cpp}`, `src/effects/kifs.{h,cpp}`, `src/effects/kuwahara.{h,cpp}`, `src/effects/laser_dance.{h,cpp}`, `src/effects/lattice_crush.{h,cpp}`, `src/effects/lattice_fold.{h,cpp}`, `src/effects/lego_bricks.{h,cpp}`, `src/effects/lens_space.{h,cpp}`, `src/effects/light_medley.{h,cpp}`, `src/effects/mandelbox.{h,cpp}`, `src/effects/matrix_rain.{h,cpp}`, `src/effects/mobius.{h,cpp}`, `src/effects/moire_generator.{h,cpp}`, `src/effects/moire_interference.{h,cpp}`, `src/effects/motherboard.{h,cpp}`, `src/effects/multi_scale_grid.{h,cpp}`, `src/effects/muons.{h,cpp}`
**Depends on**: Wave 1 complete

**Do**: In each `.h` file, delete the `ConfigDefault` function declaration line (e.g., `BloomConfig BloomConfigDefault(void);`) and its one-line comment above if present (e.g., `// Returns default config`). In each `.cpp` file, delete the `ConfigDefault` function definition (typically 3 lines: signature, `return {Name}Config{};`, closing brace).

**Verify**: `grep -r "ConfigDefault" src/effects/a*.h src/effects/b*.h ... src/effects/m*.h` returns no matches.

---

#### Task 2.2: Delete ConfigDefault from effects (N-Z)

**Files**: `src/effects/nebula.{h,cpp}`, `src/effects/neon_lattice.{h,cpp}`, `src/effects/oil_paint.{h,cpp}`, `src/effects/palette_quantization.{h,cpp}`, `src/effects/pencil_sketch.{h,cpp}`, `src/effects/perspective_tilt.{h,cpp}`, `src/effects/phi_blur.{h,cpp}`, `src/effects/phyllotaxis.{h,cpp}`, `src/effects/pitch_spiral.{h,cpp}`, `src/effects/pixelation.{h,cpp}`, `src/effects/plaid.{h,cpp}`, `src/effects/plasma.{h,cpp}`, `src/effects/poincare_disk.{h,cpp}`, `src/effects/prism_shatter.{h,cpp}`, `src/effects/radial_ifs.{h,cpp}`, `src/effects/radial_pulse.{h,cpp}`, `src/effects/radial_streak.{h,cpp}`, `src/effects/rainbow_road.{h,cpp}`, `src/effects/relativistic_doppler.{h,cpp}`, `src/effects/ripple_tank.{h,cpp}`, `src/effects/risograph.{h,cpp}`, `src/effects/scan_bars.{h,cpp}`, `src/effects/scrawl.{h,cpp}`, `src/effects/shake.{h,cpp}`, `src/effects/shard_crush.{h,cpp}`, `src/effects/shell.{h,cpp}`, `src/effects/signal_frames.{h,cpp}`, `src/effects/sine_warp.{h,cpp}`, `src/effects/slashes.{h,cpp}`, `src/effects/slit_scan_corridor.{h,cpp}`, `src/effects/solid_color.{h,cpp}`, `src/effects/spark_flash.{h,cpp}`, `src/effects/spectral_arcs.{h,cpp}`, `src/effects/spectral_rings.{h,cpp}`, `src/effects/spin_cage.{h,cpp}`, `src/effects/spiral_walk.{h,cpp}`, `src/effects/surface_warp.{h,cpp}`, `src/effects/synthwave.{h,cpp}`, `src/effects/texture_warp.{h,cpp}`, `src/effects/tone_warp.{h,cpp}`, `src/effects/toon.{h,cpp}`, `src/effects/triangle_fold.{h,cpp}`, `src/effects/triskelion.{h,cpp}`, `src/effects/twist_tunnel.{h,cpp}`, `src/effects/voronoi.{h,cpp}`, `src/effects/vortex.{h,cpp}`, `src/effects/watercolor.{h,cpp}`, `src/effects/wave_ripple.{h,cpp}`, `src/effects/wave_warp.{h,cpp}`, `src/effects/woodblock.{h,cpp}`
**Depends on**: Wave 1 complete

**Do**: Same pattern as Task 2.1. Delete `ConfigDefault` declaration (and its comment) from each `.h`, delete definition from each `.cpp`.

**Verify**: `grep -r "ConfigDefault" src/effects/n*.h ... src/effects/w*.h` returns no matches.

---

#### Task 2.3: Delete TrailMapDraw + SpatialHashResize

**Files**: `src/simulation/physarum.{h,cpp}`, `src/simulation/boids.{h,cpp}`, `src/simulation/curl_flow.{h,cpp}`, `src/simulation/attractor_flow.{h,cpp}`, `src/simulation/spatial_hash.{h,cpp}`
**Depends on**: Wave 1 complete

**Do**:
- In each of the 4 simulation files (`physarum`, `boids`, `curl_flow`, `attractor_flow`): delete the `*BeginTrailMapDraw` and `*EndTrailMapDraw` function declarations from the `.h` and definitions from the `.cpp`
- In `spatial_hash.h`: delete `SpatialHashResize` declaration
- In `spatial_hash.cpp`: delete `SpatialHashResize` definition

**Verify**: `grep -r "BeginTrailMapDraw\|EndTrailMapDraw\|SpatialHashResize" src/simulation/` returns no matches (except comments if any).

---

#### Task 2.4: Delete other dead functions

**Files**: `src/analysis/smoothing.h`, `src/analysis/beat.{h,cpp}`, `src/ui/imgui_widgets.{h,cpp}`, `src/render/drawable.{h,cpp}`, `src/automation/modulation_engine.{h,cpp}`, `src/ui/modulatable_drawable_slider.{h,cpp}`, `src/config/preset.{h,cpp}`, `src/render/profiler.{h,cpp}`, `src/ui/ui_units.h`
**Depends on**: Wave 1 complete

**Do**: Delete declaration and definition of each dead function:

| Function | Header | Source |
|----------|--------|--------|
| `ApplyEnvelopeDefault` | `src/analysis/smoothing.h` | (inline in header) |
| `BeatDetectorGetBeat` | `src/analysis/beat.h` | `src/analysis/beat.cpp` |
| `BeatDetectorGetIntensity` | `src/analysis/beat.h` | `src/analysis/beat.cpp` |
| `DrawGlow` | `src/ui/imgui_widgets.h` | `src/ui/imgui_widgets.cpp` |
| `DrawableHasType` | `src/render/drawable.h` | `src/render/drawable.cpp` |
| `DrawableValidate` | `src/render/drawable.h` | `src/render/drawable.cpp` |
| `IntensityToggleButton` | `src/ui/imgui_widgets.h` | `src/ui/imgui_widgets.cpp` |
| `ModEngineGetOffset` | `src/automation/modulation_engine.h` | `src/automation/modulation_engine.cpp` |
| `ModEngineHasRoute` | `src/automation/modulation_engine.h` | `src/automation/modulation_engine.cpp` |
| `ModulatableDrawableSliderPercent` | `src/ui/modulatable_drawable_slider.h` | `src/ui/modulatable_drawable_slider.cpp` |
| `PresetDefault` | `src/config/preset.h` | `src/config/preset.cpp` |
| `ProfilerUninit` | `src/render/profiler.h` | `src/render/profiler.cpp` |
| `SliderDrawInterval` | `src/ui/ui_units.h` | (inline in header) |
| `SliderFloatWithTooltip` | `src/ui/imgui_widgets.h` | `src/ui/imgui_widgets.cpp` |
| `SpatialHashResize` | (handled in Task 2.3) | (handled in Task 2.3) |

Note: `ProfilerUninit` being unused may indicate a missing cleanup call. Check `ProfilerInit` — if the profiler allocates resources, add a `ProfilerUninit` call at shutdown rather than deleting it. If it only zero-inits a struct, deletion is fine.

**Verify**: Build succeeds. None of these function names appear in `grep -r` output (except comments).

---

### Wave 3: Mechanical Fixes

All tasks in this wave read `tidy.log` to find their warnings. For each file with warnings in their directory scope, apply ALL applicable fixes from the Design section's fix rules table.

#### Task 3.1: Fix src/effects/ warnings

**Files**: All `src/effects/*.cpp` files listed in `tidy.log`
**Depends on**: Wave 2 complete

**Do**: Read `tidy.log` and fix all clang-tidy and cppcheck warnings in `src/effects/` files:
- Add `const` to variables and pointer parameters flagged by `misc-const-correctness` or `constParameterPointer` or `constVariable`
- Add `{}` around single-statement `if`/`else`/`for`/`while` bodies (`readability-braces-around-statements`)
- Split multi-declarations to one per line (`readability-isolate-declaration`)
- Add `(void)glow;` or `(void)categoryGlow;` for unused callback params (`misc-unused-parameters`) — these are callback signatures we can't change

**Verify**: Build succeeds.

---

#### Task 3.2: Fix src/render/ warnings

**Files**: `src/render/blend_compositor.cpp`, `src/render/color_lut.cpp`, `src/render/drawable.cpp`, `src/render/noise_texture.cpp`, `src/render/post_effect.cpp`, `src/render/render_pipeline.cpp`, `src/render/render_utils.cpp`, `src/render/shader_setup.cpp`, `src/render/shape.cpp`, `src/render/waveform.cpp`
**Depends on**: Wave 2 complete

**Do**: Read `tidy.log` and fix all warnings in `src/render/` files:
- Add `const` qualifiers (`misc-const-correctness`, `constVariable`, `constParameterPointer`)
- Add `{}` around single-statement bodies (`readability-braces-around-statements`)
- Convert C-style casts to `static_cast<Type*>()` (`cstyleCast`) — for `malloc`/`calloc` void* returns, use `static_cast`
- Convert dangerous offset casts to `reinterpret_cast` with comment (`dangerousTypeCast`) — e.g., `(bool *)((char *)&pe->effects + d.enabledOffset)` becomes `reinterpret_cast<bool*>(reinterpret_cast<char*>(&pe->effects) + d.enabledOffset)`
- Change `Texture2D texture` params to `const Texture2D& texture` (`passedByValue`)
- Reduce `variableScope` where flagged
- Fix `bugprone-incorrect-roundings` in `drawable.cpp:134` — use `lround()` (include `<cmath>`)
- Fix `bugprone-implicit-widening-of-multiplication-result` in `noise_texture.cpp:18` — cast to target type before multiply

**Verify**: Build succeeds.

---

#### Task 3.3: Fix src/simulation/ warnings

**Files**: `src/simulation/trail_map.cpp`, `src/simulation/spatial_hash.cpp`, `src/simulation/particle_life.cpp`, `src/simulation/physarum.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/attractor_flow.cpp`, `src/simulation/boids.cpp`
**Depends on**: Wave 2 complete

**Do**: Read `tidy.log` and fix all warnings in `src/simulation/` files:
- Add `const` qualifiers
- Convert C-style casts to `static_cast`
- Change struct params passed by value to `const T&`
- Replace `memset(x, 0, sizeof(X))` with `*x = X{}` (`memsetClassFloat`) — in trail_map.cpp and other files using memset on structs with floats
- Fix `concurrency-mt-unsafe` in `curl_advection.cpp` if applicable (check what functions are flagged — likely `rand()`, replace with thread-safe alternative if trivial, otherwise add `// NOLINT` with justification)

**Verify**: Build succeeds.

---

#### Task 3.4: Fix src/config/ + src/analysis/ + src/automation/ warnings

**Files**: `src/config/modulation_config.cpp`, `src/config/playlist.cpp`, `src/config/preset.cpp`, `src/config/effect_serialization.cpp`, `src/config/random_walk_config.h`, `src/analysis/analysis_pipeline.cpp`, `src/analysis/audio_features.cpp`, `src/automation/param_registry.cpp`
**Depends on**: Wave 2 complete

**Do**: Read `tidy.log` and fix all warnings in these files:
- Add `const` qualifiers
- Add `{}` around single-statement bodies
- Fix `readability-avoid-nested-conditional-operator` in `effect_serialization.cpp:696` — refactor nested ternary to if/else
- Fix `readability-implicit-bool-conversion` in `effect_serialization.cpp:698`
- Convert C-style casts in `param_registry.cpp` — the offset-based cast `(float *)((char *)effects + offset)` becomes `reinterpret_cast<float*>(reinterpret_cast<char*>(effects) + offset)`
- Fix `bugprone-integer-division` in `analysis_pipeline.cpp:89` — cast numerator to float
- Fix `cert-err33-c` warnings — check return values or cast to `(void)`

**Verify**: Build succeeds.

---

#### Task 3.5: Fix src/ui/ + src/main.cpp warnings

**Files**: `src/ui/loading_screen.cpp`, `src/ui/imgui_effects_dispatch.cpp`, `src/ui/imgui_effects.cpp`, `src/ui/imgui_drawables.cpp`, `src/ui/imgui_playlist.cpp`, `src/main.cpp`
**Depends on**: Wave 2 complete

**Do**: Read `tidy.log` and fix all warnings in these files:
- Add `const` qualifiers
- Add `{}` around single-statement bodies
- Fix `unreadVariable` for `padY` in `imgui_playlist.cpp:172` — delete or use the variable
- Fix `readability-implicit-bool-conversion` where flagged

**Verify**: Build succeeds.

---

### Wave 4: Prevention

#### Task 4.1: Extend pre-commit hook

**Files**: `.git/hooks/pre-commit`
**Depends on**: Wave 3 complete

**Do**: Append the clang-tidy section from the Design to the existing pre-commit hook, after the clang-format section. The hook should:
1. Find clang-tidy executable (try `clang-tidy.exe` then `clang-tidy`)
2. Skip gracefully if not found or if `build/compile_commands.json` doesn't exist
3. Run clang-tidy on staged `.cpp` files only
4. Fail the commit if any warnings are found
5. Print the warnings so the developer can see what to fix

**Verify**: Stage a `.cpp` file with a const-correctness issue, run `git commit` — hook should block. Fix the issue, commit should succeed.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] `./scripts/lint.sh` produces zero clang-tidy warnings
- [ ] `./scripts/lint.sh` cppcheck output has no `constParameterCallback` or `unreadVariable:*effect_serialization.cpp` warnings
- [ ] `grep -r "ConfigDefault" src/` returns no matches
- [ ] `grep -r "BeginTrailMapDraw\|EndTrailMapDraw" src/` returns no matches
- [ ] Pre-commit hook blocks commits with new clang-tidy warnings
- [ ] `.claude/skills/add-effect/SKILL.md` does not mention ConfigDefault
- [ ] `docs/conventions.md` does not mention ConfigDefault, includes new const/braces/cast rules
