# Codebase Structure

> Last sync: 2026-05-03 | Commit: 633041d0

## Codebase Size

| Language | Files | Code | Comments |
|----------|-------|------|----------|
| C++ (.cpp) | 208 | 33,535 | 1,744 |
| C++ Headers (.h) | 219 | 11,458 | 2,161 |
| GLSL (.fs/.glsl) | 178 | 15,091 | 2,346 |
| **Total** | **605** | **60,084** | **6,251** |

## Directory Layout

```
AudioJones/
├── src/                # C++ source code
│   ├── analysis/       # FFT, beat detection, audio features
│   ├── audio/          # WASAPI loopback capture
│   ├── automation/     # LFO, modulation routing, mod buses, param registry
│   ├── config/         # Shared configs, preset serialization, effect descriptor table
│   ├── effects/        # Effect modules (149 .cpp / 149 .h pairs)
│   ├── render/         # Drawables, shaders, post-processing
│   ├── simulation/     # GPU agent simulations
│   ├── ui/             # ImGui panels, widgets, sliders
│   └── main.cpp        # Application entry, frame loop
├── shaders/            # GLSL fragment (.fs) and compute (.glsl)
├── presets/            # JSON preset files (44 presets)
├── playlists/          # Playlist JSON files
├── fonts/              # UI fonts (Roboto-Medium.ttf, font_atlas.png)
├── scripts/            # Utility scripts (gen_font_atlas.py, lint.sh)
├── docs/               # Documentation and plans
│   ├── plans/archive/  # Completed plans (20 files)
│   ├── research/       # Effect research docs (27 files)
│   └── research/archive/ # Completed research docs (20 files)
├── build/              # CMake build output (not committed)
├── .claude/            # Claude agent configs and skills
│   ├── agents/         # Specialized agent prompts (2 agents)
│   └── skills/         # Skill definitions (13 skills)
└── .vscode/            # VS Code settings
```

## Directory Purposes

**`src/analysis/`:**
- Purpose: Audio signal processing from raw PCM to usable features
- Contains: FFT processor, beat detector, band energies, spectral features, smoothing helpers
- Key files: `fft.cpp`, `beat.cpp`, `bands.cpp`, `analysis_pipeline.cpp`, `audio_features.cpp`, `smoothing.h`

**`src/audio/`:**
- Purpose: Windows loopback audio capture via miniaudio/WASAPI
- Contains: Audio device enumeration, ring buffer, capture lifecycle
- Key files: `audio.cpp`, `audio.h`, `audio_config.h`

**`src/automation/`:**
- Purpose: Parameter modulation system (LFOs, mod buses, routing, live control)
- Contains: LFO generators, mod bus mixing, mod source aggregation, param bounds registry, easing functions
- Key files: `lfo.cpp`, `modulation_engine.cpp`, `param_registry.cpp`, `drawable_params.cpp`, `mod_sources.cpp`, `mod_bus.cpp`, `easing.h`

**`src/config/`:**
- Purpose: Shared configuration structs and preset I/O
- Contains: Master effect config, drawable config, feedback/LFO/modulation/mod bus configs, JSON serialization, effect descriptor table, attractor type definitions, playlist system, platonic solid geometry
- Key files: `effect_config.h`, `drawable_config.h`, `preset.cpp`, `lfo_config.h`, `modulation_config.h`, `mod_bus_config.h`, `dual_lissajous_config.h`, `effect_descriptor.h`, `effect_descriptor.cpp`, `effect_serialization.cpp`, `effect_serialization.h`, `procedural_warp_config.h`, `random_walk_config.h`, `feedback_flow_config.h`, `attractor_types.h`, `constants.h`, `app_configs.h`, `band_config.h`, `playlist.cpp`, `playlist.h`, `platonic_solids.h`

**`src/effects/`:**
- Purpose: Self-contained effect modules, each owning config, shader resources, and lifecycle
- Contains: 149 effect `.cpp` files paired with 149 `.h` files; each provides config struct, Init/Setup/Uninit functions, param registration, and colocated UI
- Categories (17): Symmetry, Warp, Cellular, Motion, Painterly, Print, Retro, Optical, Color, Simulation, Geometric, Filament, Texture, Field, Novelty, Scatter, Cymatics
- Add new effects here as paired `<name>.cpp` and `<name>.h` files; register via the `REGISTER_EFFECT*` / `REGISTER_GENERATOR*` / `REGISTER_SIM_BOOST` macros at the bottom of the `.cpp` file

**`src/render/`:**
- Purpose: GPU rendering, shader management, post-processing pipeline
- Contains: Drawable types, shader uniform binding, render passes, blend compositor, color LUT, gradient system, noise texture generation, descriptor-driven `PostEffect` state slots
- Key files: `render_pipeline.cpp`, `post_effect.cpp`, `drawable.cpp`, `blend_compositor.cpp`, `shader_setup.cpp`, `color_config.cpp`, `color_lut.cpp`, `gradient.cpp`, `profiler.cpp`, `render_utils.cpp`, `draw_utils.cpp`, `render_context.h`, `shape.cpp`, `spectrum_bars.cpp`, `waveform.cpp`, `thick_line.cpp`, `noise_texture.cpp`, `blend_mode.h`

**`src/simulation/`:**
- Purpose: GPU compute shader agent simulations with colocated UI
- Contains: Physarum slime mold, boids flocking, curl flow, attractor flow, particle life, maze worms, shared trail map and spatial hash helpers
- Key files: `physarum.cpp`, `boids.cpp`, `curl_flow.cpp`, `particle_life.cpp`, `attractor_flow.cpp`, `maze_worms.cpp`, `trail_map.cpp`, `spatial_hash.cpp`, `shader_utils.cpp`, `bounds_mode.h`

**`src/ui/`:**
- Purpose: Dear ImGui interface panels and custom widgets
- Contains: Effect panel dispatch, modulatable sliders, gradient editor, drawable controls, playlist UI, mod bus UI, loading screen, theme constants
- Key files: `imgui_panels.cpp`, `imgui_effects.cpp`, `imgui_effects_dispatch.cpp`, `imgui_analysis.cpp`, `imgui_audio.cpp`, `imgui_bus.cpp`, `imgui_drawables.cpp`, `imgui_lfo.cpp`, `imgui_presets.cpp`, `imgui_playlist.cpp`, `imgui_widgets.cpp`, `modulatable_slider.cpp`, `modulatable_drawable_slider.cpp`, `gradient_editor.cpp`, `drawable_type_controls.cpp`, `loading_screen.cpp`, `ui_units.h`, `theme.h`

**`shaders/`:**
- Purpose: GLSL shader source files
- Contains: 169 fragment shaders (`.fs`) for post-effects and generators, 9 compute shaders (`.glsl`) for simulations
- Categories mirror `src/effects/` and `src/simulation/` module names
- Add new effect shaders as `<effect_name>.fs`; multi-pass effects use suffixes like `_prefilter`, `_downsample`, `_upsample`, `_composite`. Add new simulation compute shaders as `<name>_agents.glsl`

**`presets/`:**
- Purpose: User-saveable visualization configurations
- Contains: JSON files with effect settings, drawables, LFO routes (44 presets)

**`playlists/`:**
- Purpose: Ordered sequences of presets for automated playback
- Contains: JSON playlist files

**`scripts/`:**
- Purpose: Utility scripts for asset generation and tooling
- Key files: `gen_font_atlas.py`, `lint.sh`

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Application initialization, main loop, teardown

**Configuration:**
- `src/config/effect_config.h`: Master effect config struct, transform ordering
- `src/config/drawable_config.h`: Drawable types (waveform, spectrum, shape)
- `src/config/preset.cpp`: JSON serialization/deserialization for `Preset`, `LFOConfig`, `ModBusConfig`
- `src/config/effect_descriptor.h`: Effect descriptor table indexed by `TransformEffectType`; defines `REGISTER_EFFECT*`, `REGISTER_GENERATOR*`, `REGISTER_SIM_BOOST` macros and `STANDARD_GENERATOR_OUTPUT`
- `src/config/effect_descriptor.cpp`: Descriptor table implementation with name, category badge, section index, flags
- `src/config/effect_serialization.cpp`: Per-effect config JSON round-trip via `<NAME>_CONFIG_FIELDS` macros
- `src/config/mod_bus_config.h`: Mod bus configuration struct
- `src/config/playlist.cpp`: Playlist loading, saving, and sequencing

**Core Logic:**
- `src/render/render_pipeline.cpp`: Frame rendering orchestration
- `src/render/post_effect.cpp`: Shader initialization, descriptor-driven state slot management, pass execution
- `src/render/shader_setup.cpp`: Uniform binding entry point per effect category
- `src/automation/modulation_engine.cpp`: LFO-to-parameter routing, mod source evaluation
- `src/automation/mod_bus.cpp`: Mod bus mixing and evaluation
- `src/ui/imgui_effects_dispatch.cpp`: Effect UI dispatch (iterates `EFFECT_DESCRIPTORS[]` and invokes each effect's colocated `Draw<Name>Params` callback)

## Where to Add New Code

**Transform effect:**
- Source: `src/effects/<name>.cpp` and `src/effects/<name>.h`
- Shader: `shaders/<name>.fs` (multi-pass effects use `_prefilter`/`_downsample`/`_upsample`/`_composite` suffixes)
- Registration: `REGISTER_EFFECT(...)` or `REGISTER_EFFECT_CFG(...)` macro at the bottom of the `.cpp` (wrapped in `// clang-format off` / `// clang-format on`)
- Descriptor row is generated by the registration macro; do not hand-edit `EFFECT_DESCRIPTORS[]`
- Add a serialization line in `src/config/effect_serialization.cpp` referencing the `<NAME>_CONFIG_FIELDS` macro from the effect's header
- Embed the config field in `EffectConfig` (`src/config/effect_config.h`)
- Colocated UI: `// === UI ===` section in the same `.cpp` exposing `static void Draw<Name>Params(EffectConfig*, const ModSources*, ImU32)`; pass it as the last argument to the registration macro

**Generator effect:**
- Source: `src/effects/<name>.cpp` and `src/effects/<name>.h` (same path as transform effects)
- Shader: `shaders/<name>.fs`
- Registration: `REGISTER_GENERATOR(...)` (auto-sets `"GEN"` badge and `EFFECT_FLAG_BLEND`) or `REGISTER_GENERATOR_FULL(...)` for generators with sized init / resize / custom render
- Output UI: place `STANDARD_GENERATOR_OUTPUT(<field>)` immediately above the registration macro to generate the standard Color/Output section, then pass `DrawOutput_<field>` as the `DrawOutputFnArg`
- Otherwise identical to transform effects (config field in `EffectConfig`, serialization entry, colocated `Draw<Name>Params`)

**Simulation:**
- Source: `src/simulation/<name>.cpp` and `src/simulation/<name>.h`
- Compute shader: `shaders/<name>_agents.glsl` (plus optional render fragment shader `shaders/<name>.fs`)
- Registration: `REGISTER_SIM_BOOST(Type, field, displayName, SetupFn, RegisterFn, DrawParamsFnArg)` at the bottom of the `.cpp`
- Colocated UI: `// === UI ===` section with `static void Draw<Name>Params()`
- Add the simulation's enum entry alongside other transform types so the descriptor table picks it up

**Shader (any kind):**
- Fragment effect shader: `shaders/<effect_name>.fs`
- Multi-pass naming: `<effect>_prefilter.fs`, `<effect>_downsample.fs`, `<effect>_upsample.fs`, `<effect>_composite.fs`
- Compute shader: `shaders/<name>.glsl` (simulation agents typically use `<name>_agents.glsl`)

**Config struct:**
- Effect-specific config: define inline in the effect's `.h` as `<Name>Config` and embed as a member of `EffectConfig` (`src/config/effect_config.h`)
- Reusable config shared across effects: `src/config/<name>_config.h` (e.g., `dual_lissajous_config.h`, `procedural_warp_config.h`, `random_walk_config.h`, `feedback_flow_config.h`)
- Color/visual configs that belong to the render layer: `src/render/color_config.h`
- Always provide a `<NAME>_CONFIG_FIELDS` macro listing serializable fields, then register the type with `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` in `src/config/effect_serialization.cpp` (effect configs) or `src/config/preset.cpp` (automation configs like LFOs and mod buses)

**Modulatable parameter:**
- Effect-local params: register inside `<Name>RegisterParams()` with `ModEngineRegisterParam("<effect>.<field>", &cfg->field, min, max)`
- Global params (feedback, flow field, etc.): add a row to `PARAM_TABLE[]` in `src/automation/param_registry.cpp`

**Preset / playlist file:**
- Preset: `presets/<name>.json` (saved by the in-app preset panel; manual edits should match the schema produced by `src/config/preset.cpp`)
- Playlist: `playlists/<name>.json`

**Documentation:**
- Architecture / structure / conventions: `docs/architecture.md`, `docs/structure.md`, `docs/conventions.md` (regenerated by `/sync-docs`)
- Effect catalog entry for new effects: `docs/effects.md` (use the `/write-effect-description` skill)
- Active feature plans: `docs/plans/<slug>.md`
- Completed feature plans: `docs/plans/archive/`
- Effect research: `docs/research/<slug>.md`
- Completed research: `docs/research/archive/`

## Special Directories

**`build/`:**
- Purpose: CMake build artifacts, compiled binaries
- Generated: Yes
- Committed: No

**`docs/plans/`:**
- Purpose: Active feature plans under development (0 active)
- Generated: No
- Committed: Yes

**`docs/plans/archive/`:**
- Purpose: Completed feature plans (20 archived)
- Generated: No
- Committed: Yes

**`docs/research/`:**
- Purpose: Effect research and algorithm documentation (27 active)
- Generated: No
- Committed: Yes

**`docs/research/archive/`:**
- Purpose: Completed effect research docs (20 archived)
- Generated: No
- Committed: Yes

**`.claude/`:**
- Purpose: Claude Code agent configurations and skills
- Generated: No
- Committed: Yes

---

*Run `/sync-docs` to regenerate.*
