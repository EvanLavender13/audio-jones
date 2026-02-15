# Colocated Serialization Macros

Move JSON field lists from the centralized `effect_serialization.cpp` into per-header `*_CONFIG_FIELDS` preprocessor macros, so adding a config field only requires editing one file.

**Research**: `docs/research/colocated-serialization-macros.md`

## Design

### Approach

Each config header defines a `#define` macro listing its serializable fields. The centralized `effect_serialization.cpp` expands these macros inside the NLOHMANN call. The `<nlohmann/json.hpp>` header stays in one translation unit — zero compile-time impact.

### Macro Convention

- Name: `<UPPER_SNAKE_NAME>_CONFIG_FIELDS` matching the struct (e.g., `BloomConfig` -> `BLOOM_CONFIG_FIELDS`)
- Placement: immediately after the config struct closing `};`, before the Effect struct or function declarations
- Content: exact copy of the current field list from `effect_serialization.cpp` (no audit, no changes)

### Scope

**Migrate** (87 configs across ~84 headers):
- 4 shared configs in `src/config/` headers
- 7 simulation configs in `src/simulation/` headers
- 76 effect configs across 73 files in `src/effects/` (moire_generator.h has 2 configs: `MoireLayerConfig` + `MoireGeneratorConfig`)

**No configs in `src/render/`** need migration — `ColorConfig` (the only render-located config) uses custom `to_json`/`from_json` and is excluded.

**Keep as-is in effect_serialization.cpp** (no macro indirection):
- `Color`, `GradientStop` — `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` (non-defaulted, different macro)
- `ColorConfig` — manual `to_json`/`from_json` with switch on mode
- `EffectConfig` — master serializer with `EFFECT_CONFIG_FIELDS` batch macro and custom logic
- `TransformOrderConfig` — custom string-based order serialization

**Naming distinction**: The existing `EFFECT_CONFIG_FIELDS(X)` macro (line 571) lists `EffectConfig` *member* names (`sineWarp`, `kaleidoscope`, etc.) — it is unrelated to the new per-effect `*_CONFIG_FIELDS` macros which list each config *struct's fields*. Do not confuse them.

### Example

Before:
```cpp
// bloom.h
struct BloomConfig {
  bool enabled = false;
  float threshold = 0.8f;
  float knee = 0.5f;
  float intensity = 0.5f;
  int iterations = 5;
};

// effect_serialization.cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig, enabled, threshold,
                                                knee, intensity, iterations)
```

After:
```cpp
// bloom.h
struct BloomConfig {
  bool enabled = false;
  float threshold = 0.8f;
  float knee = 0.5f;
  float intensity = 0.5f;
  int iterations = 5;
};

#define BLOOM_CONFIG_FIELDS enabled, threshold, knee, intensity, iterations

// effect_serialization.cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig, BLOOM_CONFIG_FIELDS)
```

---

## Tasks

### Wave 1: Add field-list macros to all headers

All 5 tasks run in parallel — no file overlap.

Each task follows the same pattern:
1. Read `src/config/effect_serialization.cpp` to find the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` invocation for each config
2. Copy the field list (everything after the type name)
3. Add `#define <UPPER_SNAKE>_CONFIG_FIELDS <fields>` after the config struct's closing `};` in the header
4. Leave one blank line before and after the `#define`

#### Task 1.1: Shared config headers

**Files** (modify):
- `src/config/dual_lissajous_config.h` — `DUAL_LISSAJOUS_CONFIG_FIELDS` for `DualLissajousConfig`
- `src/config/effect_config.h` — `FLOW_FIELD_CONFIG_FIELDS` for `FlowFieldConfig`
- `src/config/feedback_flow_config.h` — `FEEDBACK_FLOW_CONFIG_FIELDS` for `FeedbackFlowConfig`
- `src/config/procedural_warp_config.h` — `PROCEDURAL_WARP_CONFIG_FIELDS` for `ProceduralWarpConfig`

**Do**: Read `effect_serialization.cpp` lines 131-185 for field lists. Add macro after each struct. For `DualLissajousConfig`, place the macro between the struct and the `DualLissajousUpdate` inline function. For `FlowFieldConfig` in `effect_config.h`, place after the `FlowFieldConfig` struct — NOT after `EffectConfig` or `TransformOrderConfig`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Simulation config headers

**Files** (modify):
- `src/simulation/physarum.h` — `PHYSARUM_CONFIG_FIELDS`
- `src/simulation/curl_flow.h` — `CURL_FLOW_CONFIG_FIELDS`
- `src/simulation/attractor_flow.h` — `ATTRACTOR_FLOW_CONFIG_FIELDS`
- `src/simulation/boids.h` — `BOIDS_CONFIG_FIELDS`
- `src/simulation/curl_advection.h` — `CURL_ADVECTION_CONFIG_FIELDS`
- `src/simulation/cymatics.h` — `CYMATICS_CONFIG_FIELDS`
- `src/simulation/particle_life.h` — `PARTICLE_LIFE_CONFIG_FIELDS`

**Do**: Read `effect_serialization.cpp` lines 135-176 for field lists. Add macro after each config struct.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.3: Effect configs A-G

**Files** (modify, all in `src/effects/`):
- `anamorphic_streak.h` — `ANAMORPHIC_STREAK_CONFIG_FIELDS`
- `arc_strobe.h` — `ARC_STROBE_CONFIG_FIELDS`
- `ascii_art.h` — `ASCII_ART_CONFIG_FIELDS`
- `attractor_lines.h` — `ATTRACTOR_LINES_CONFIG_FIELDS`
- `bloom.h` — `BLOOM_CONFIG_FIELDS`
- `bokeh.h` — `BOKEH_CONFIG_FIELDS`
- `chladni_warp.h` — `CHLADNI_WARP_CONFIG_FIELDS`
- `circuit_board.h` — `CIRCUIT_BOARD_CONFIG_FIELDS`
- `color_grade.h` — `COLOR_GRADE_CONFIG_FIELDS`
- `constellation.h` — `CONSTELLATION_CONFIG_FIELDS`
- `corridor_warp.h` — `CORRIDOR_WARP_CONFIG_FIELDS`
- `cross_hatching.h` — `CROSS_HATCHING_CONFIG_FIELDS`
- `crt.h` — `CRT_CONFIG_FIELDS`
- `density_wave_spiral.h` — `DENSITY_WAVE_SPIRAL_CONFIG_FIELDS`
- `disco_ball.h` — `DISCO_BALL_CONFIG_FIELDS`
- `domain_warp.h` — `DOMAIN_WARP_CONFIG_FIELDS`
- `dot_matrix.h` — `DOT_MATRIX_CONFIG_FIELDS`
- `droste_zoom.h` — `DROSTE_ZOOM_CONFIG_FIELDS`
- `false_color.h` — `FALSE_COLOR_CONFIG_FIELDS`
- `filaments.h` — `FILAMENTS_CONFIG_FIELDS`
- `glitch.h` — `GLITCH_CONFIG_FIELDS`
- `glyph_field.h` — `GLYPH_FIELD_CONFIG_FIELDS`
- `gradient_flow.h` — `GRADIENT_FLOW_CONFIG_FIELDS`

**Do**: Read `effect_serialization.cpp` to find each config's field list. Add macro after each config struct.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.4: Effect configs H-N

**Files** (modify, all in `src/effects/`):
- `halftone.h` — `HALFTONE_CONFIG_FIELDS`
- `heightfield_relief.h` — `HEIGHTFIELD_RELIEF_CONFIG_FIELDS`
- `hue_remap.h` — `HUE_REMAP_CONFIG_FIELDS`
- `impressionist.h` — `IMPRESSIONIST_CONFIG_FIELDS`
- `infinite_zoom.h` — `INFINITE_ZOOM_CONFIG_FIELDS`
- `ink_wash.h` — `INK_WASH_CONFIG_FIELDS`
- `interference.h` — `INTERFERENCE_CONFIG_FIELDS`
- `interference_warp.h` — `INTERFERENCE_WARP_CONFIG_FIELDS`
- `kaleidoscope.h` — `KALEIDOSCOPE_CONFIG_FIELDS`
- `kifs.h` — `KIFS_CONFIG_FIELDS`
- `kuwahara.h` — `KUWAHARA_CONFIG_FIELDS`
- `lattice_fold.h` — `LATTICE_FOLD_CONFIG_FIELDS`
- `lego_bricks.h` — `LEGO_BRICKS_CONFIG_FIELDS`
- `mandelbox.h` — `MANDELBOX_CONFIG_FIELDS`
- `matrix_rain.h` — `MATRIX_RAIN_CONFIG_FIELDS`
- `mobius.h` — `MOBIUS_CONFIG_FIELDS`
- `moire_generator.h` — `MOIRE_LAYER_CONFIG_FIELDS` (for `MoireLayerConfig`) AND `MOIRE_GENERATOR_CONFIG_FIELDS` (for `MoireGeneratorConfig`)
- `moire_interference.h` — `MOIRE_INTERFERENCE_CONFIG_FIELDS`
- `motherboard.h` — `MOTHERBOARD_CONFIG_FIELDS`
- `muons.h` — `MUONS_CONFIG_FIELDS`
- `multi_scale_grid.h` — `MULTI_SCALE_GRID_CONFIG_FIELDS`
- `nebula.h` — `NEBULA_CONFIG_FIELDS`
- `neon_glow.h` — `NEON_GLOW_CONFIG_FIELDS`

**Do**: Read `effect_serialization.cpp` to find each config's field list. Add macro after each config struct. For `moire_generator.h`, add TWO macros — `MOIRE_LAYER_CONFIG_FIELDS` after `MoireLayerConfig` struct and `MOIRE_GENERATOR_CONFIG_FIELDS` after `MoireGeneratorConfig` struct.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.5: Effect configs O-Z

**Files** (modify, all in `src/effects/`):
- `oil_paint.h` — `OIL_PAINT_CONFIG_FIELDS`
- `palette_quantization.h` — `PALETTE_QUANTIZATION_CONFIG_FIELDS`
- `pencil_sketch.h` — `PENCIL_SKETCH_CONFIG_FIELDS`
- `phi_blur.h` — `PHI_BLUR_CONFIG_FIELDS`
- `phyllotaxis.h` — `PHYLLOTAXIS_CONFIG_FIELDS`
- `pitch_spiral.h` — `PITCH_SPIRAL_CONFIG_FIELDS`
- `pixelation.h` — `PIXELATION_CONFIG_FIELDS`
- `plasma.h` — `PLASMA_CONFIG_FIELDS`
- `poincare_disk.h` — `POINCARE_DISK_CONFIG_FIELDS`
- `radial_ifs.h` — `RADIAL_IFS_CONFIG_FIELDS`
- `radial_pulse.h` — `RADIAL_PULSE_CONFIG_FIELDS`
- `radial_streak.h` — `RADIAL_STREAK_CONFIG_FIELDS`
- `relativistic_doppler.h` — `RELATIVISTIC_DOPPLER_CONFIG_FIELDS`
- `scan_bars.h` — `SCAN_BARS_CONFIG_FIELDS`
- `shake.h` — `SHAKE_CONFIG_FIELDS`
- `signal_frames.h` — `SIGNAL_FRAMES_CONFIG_FIELDS`
- `sine_warp.h` — `SINE_WARP_CONFIG_FIELDS`
- `slashes.h` — `SLASHES_CONFIG_FIELDS`
- `solid_color.h` — `SOLID_COLOR_CONFIG_FIELDS`
- `spectral_arcs.h` — `SPECTRAL_ARCS_CONFIG_FIELDS`
- `surface_warp.h` — `SURFACE_WARP_CONFIG_FIELDS`
- `synthwave.h` — `SYNTHWAVE_CONFIG_FIELDS`
- `texture_warp.h` — `TEXTURE_WARP_CONFIG_FIELDS`
- `tone_warp.h` — `TONE_WARP_CONFIG_FIELDS`
- `toon.h` — `TOON_CONFIG_FIELDS`
- `triangle_fold.h` — `TRIANGLE_FOLD_CONFIG_FIELDS`
- `voronoi.h` — `VORONOI_CONFIG_FIELDS`
- `watercolor.h` — `WATERCOLOR_CONFIG_FIELDS`
- `wave_ripple.h` — `WAVE_RIPPLE_CONFIG_FIELDS`

**Do**: Read `effect_serialization.cpp` to find each config's field list. Add macro after each config struct.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Update effect_serialization.cpp

#### Task 2.1: Replace field lists with macro references

**Files** (modify): `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**: For each `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` invocation (lines 131-507), replace the inline field list with the corresponding `*_CONFIG_FIELDS` macro. Each invocation becomes a single line:

```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig, BLOOM_CONFIG_FIELDS)
```

Do NOT modify:
- Lines 11-12: `Color` and `GradientStop` (non-defaulted macro)
- Lines 14-130: `ColorConfig` (custom to_json/from_json)
- Lines 509-628: `TransformOrderConfig`, `EFFECT_CONFIG_FIELDS` batch macro, `EffectConfig` to_json/from_json

The file must include all headers that define the `*_CONFIG_FIELDS` macros. Check existing includes — `dual_lissajous_config.h` is already included. Add any missing includes for simulation and effect headers.

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `effect_serialization.cpp` compiles — all macros resolve correctly
- [ ] Existing presets load correctly (JSON round-trip unchanged)
- [ ] No `#define` macros placed inside include guards incorrectly
