# Codebase Structure

> Last sync: 2026-04-26 | Commit: d3121df1

## Codebase Size

| Language | Files | Code | Comments |
|----------|-------|------|----------|
| C++ (.cpp) | 204 | 31,959 | 1,695 |
| C++ Headers (.h) | 215 | 11,101 | 2,088 |
| GLSL (.fs/.glsl) | 173 | 14,591 | 2,285 |
| **Total** | **592** | **57,651** | **6,068** |

## Directory Layout

```
AudioJones/
├── src/                # C++ source code
│   ├── analysis/       # FFT, beat detection, audio features
│   ├── audio/          # WASAPI loopback capture
│   ├── automation/     # LFO, modulation routing, mod buses, param registry
│   ├── config/         # Shared configs (18 headers), preset serialization, effect descriptor table
│   ├── effects/        # Effect modules (145 .cpp files, 145 .h files)
│   ├── render/         # Drawables, shaders, post-processing
│   ├── simulation/     # GPU agent simulations (physarum, boids, curl, particle_life, attractor, maze_worms)
│   ├── ui/             # ImGui panels, widgets, sliders
│   └── main.cpp        # Application entry, frame loop
├── shaders/            # GLSL fragment (.fs) and compute (.glsl)
├── presets/            # JSON preset files (40 presets)
├── playlists/          # Playlist JSON files
├── fonts/              # UI fonts (Roboto-Medium.ttf, font_atlas.png)
├── scripts/            # Utility scripts (gen_font_atlas.py)
├── docs/               # Documentation and plans
│   ├── plans/          # Active feature plans (0 files)
│   ├── plans/archive/  # Completed plans (14 files)
│   ├── research/       # Effect research docs (22 files)
│   └── research/archive/ # Completed research docs (14 files)
├── build/              # CMake build output (not committed)
├── .claude/            # Claude agent configs and skills
│   ├── agents/         # Specialized agent prompts (2 agents)
│   └── skills/         # Skill definitions (13 skills)
└── .vscode/            # VS Code settings
```

## Directory Purposes

**`src/analysis/`:**
- Purpose: Audio signal processing from raw PCM to usable features
- Contains: FFT processor, beat detector, band energies, spectral features
- Key files: `fft.cpp`, `beat.cpp`, `bands.cpp`, `analysis_pipeline.cpp`, `audio_features.cpp`, `smoothing.h`

**`src/audio/`:**
- Purpose: Windows loopback audio capture via miniaudio/WASAPI
- Contains: Audio device enumeration, ring buffer, capture lifecycle
- Key files: `audio.cpp`, `audio.h`, `audio_config.h`

**`src/automation/`:**
- Purpose: Parameter modulation system (LFOs, mod buses, routing, live control)
- Contains: LFO generators, mod bus mixing, mod source aggregation, param bounds registry, easing functions
- Key files: `lfo.cpp`, `modulation_engine.cpp`, `param_registry.cpp`, `drawable_params.cpp`, `mod_sources.cpp`, `mod_bus.cpp`, `mod_bus.h`, `easing.h`

**`src/config/`:**
- Purpose: Shared configuration structs and preset I/O
- Contains: Master effect config, drawable config, feedback/LFO/modulation/mod bus configs, JSON serialization, effect descriptor table, attractor type definitions, playlist system, platonic solid geometry
- Key files: `effect_config.h`, `drawable_config.h`, `preset.cpp`, `lfo_config.h`, `modulation_config.h`, `modulation_config.cpp`, `mod_bus_config.h`, `dual_lissajous_config.h`, `effect_descriptor.h`, `effect_descriptor.cpp`, `effect_serialization.cpp`, `procedural_warp_config.h`, `random_walk_config.h`, `feedback_flow_config.h`, `attractor_types.h`, `constants.h`, `app_configs.h`, `band_config.h`, `playlist.cpp`, `playlist.h`, `platonic_solids.h`

**`src/effects/`:**
- Purpose: Self-contained effect modules, each owning config, shader resources, and lifecycle
- Contains: 145 effect `.cpp` files across 17 categories, each providing config struct, Init/Setup/Uninit functions, param registration, and colocated UI
- Categories: Symmetry, Warp, Cellular, Motion, Painterly, Print, Retro, Optical, Color, Simulation, Geometric, Filament, Texture, Field, Novelty, Scatter, Cymatics
- Add new effects here as paired `<name>.cpp` and `<name>.h` files; register via the `REGISTER_EFFECT*` / `REGISTER_GENERATOR*` / `REGISTER_SIM_BOOST` macros at the bottom of the `.cpp` file

**`src/render/`:**
- Purpose: GPU rendering, shader management, post-processing pipeline
- Contains: Drawable types, shader uniform binding, render passes, blend compositor, color LUT, gradient system, noise texture generation
- Key files: `render_pipeline.cpp`, `post_effect.cpp`, `drawable.cpp`, `blend_compositor.cpp`, `shader_setup.cpp`, `color_config.cpp`, `color_lut.cpp`, `gradient.cpp`, `profiler.cpp`, `render_utils.cpp`, `draw_utils.cpp`, `render_context.h`, `shape.cpp`, `spectrum_bars.cpp`, `waveform.cpp`, `thick_line.cpp`, `noise_texture.cpp`

**`src/simulation/`:**
- Purpose: GPU compute shader agent simulations with colocated UI
- Contains: Physarum slime mold, boids flocking, curl flow, attractors, particle life, maze worms
- Key files: `physarum.cpp`, `boids.cpp`, `curl_flow.cpp`, `particle_life.cpp`, `attractor_flow.cpp`, `maze_worms.cpp`, `trail_map.cpp`, `spatial_hash.cpp`, `shader_utils.cpp`, `bounds_mode.h`

**`src/ui/`:**
- Purpose: Dear ImGui interface panels and custom widgets
- Contains: Effect panels (dispatch-based), modulatable sliders, gradient editor, drawable controls, playlist UI, mod bus UI, loading screen
- Key files: `imgui_panels.cpp`, `imgui_effects.cpp`, `imgui_effects_dispatch.cpp`, `imgui_effects_dispatch.h`, `imgui_analysis.cpp`, `imgui_audio.cpp`, `imgui_bus.cpp`, `imgui_drawables.cpp`, `imgui_lfo.cpp`, `imgui_presets.cpp`, `modulatable_slider.cpp`, `modulatable_drawable_slider.cpp`, `gradient_editor.cpp`, `drawable_type_controls.cpp`, `imgui_widgets.cpp`, `imgui_playlist.cpp`, `loading_screen.cpp`, `ui_units.h`, `theme.h`

**`shaders/`:**
- Purpose: GLSL shader source files
- Contains: 164 fragment shaders (`.fs`) for post-effects and generators, 9 compute shaders (`.glsl`) for simulations
- Categories mirror `src/effects/` and `src/simulation/` module names
- Add new effect shaders as `<effect_name>.fs`; multi-pass effects use suffixes like `_prefilter`, `_downsample`, `_upsample`, `_composite`. Add new simulation compute shaders as `<name>_agents.glsl`

**`presets/`:**
- Purpose: User-saveable visualization configurations
- Contains: JSON files with effect settings, drawables, LFO routes (40 presets)

**`playlists/`:**
- Purpose: Ordered sequences of presets for automated playback
- Contains: JSON playlist files

**`scripts/`:**
- Purpose: Utility scripts for asset generation
- Contains: Font atlas generator
- Key files: `gen_font_atlas.py`

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Application initialization, main loop, teardown

**Configuration:**
- `src/config/effect_config.h`: Master effect config struct, transform ordering
- `src/config/drawable_config.h`: Drawable types (waveform, spectrum, shape)
- `src/config/preset.cpp`: JSON serialization/deserialization
- `src/config/effect_descriptor.h`: Effect descriptor table indexed by TransformEffectType
- `src/config/effect_descriptor.cpp`: Descriptor table implementation with name, category, flags
- `src/config/effect_serialization.cpp`: Per-effect config JSON round-trip via config fields macros
- `src/config/mod_bus_config.h`: Mod bus configuration struct
- `src/config/playlist.cpp`: Playlist loading, saving, and sequencing

**Core Logic:**
- `src/render/render_pipeline.cpp`: Frame rendering orchestration
- `src/render/post_effect.cpp`: Shader initialization, pass management
- `src/render/shader_setup.cpp`: Uniform binding for all effect categories
- `src/automation/modulation_engine.cpp`: LFO-to-parameter routing
- `src/automation/mod_bus.cpp`: Mod bus mixing and evaluation
- `src/ui/imgui_effects_dispatch.cpp`: Effect UI dispatch (routes each effect type to its draw function)

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
- Purpose: Completed feature plans (14 archived)
- Generated: No
- Committed: Yes

**`docs/research/`:**
- Purpose: Effect research and algorithm documentation (22 active)
- Generated: No
- Committed: Yes

**`docs/research/archive/`:**
- Purpose: Completed effect research docs (14 archived)
- Generated: No
- Committed: Yes

**`.claude/`:**
- Purpose: Claude Code agent configurations and skills
- Generated: No
- Committed: Yes

---

*Run `/sync-docs` to regenerate.*
