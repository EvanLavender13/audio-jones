# Codebase Structure

> Last sync: 2026-03-07 | Commit: 696adb30

## Codebase Size

| Language | Files | Code | Comments |
|----------|-------|------|----------|
| C++ (.cpp) | 154 | 22,809 | 1,273 |
| C++ Headers (.h) | 165 | 7,905 | 1,605 |
| GLSL (.fs/.glsl) | 127 | 10,059 | 1,716 |
| **Total** | **446** | **40,773** | **4,594** |

## Directory Layout

```
AudioJones/
├── src/                # C++ source code
│   ├── analysis/       # FFT, beat detection, audio features
│   ├── audio/          # WASAPI loopback capture
│   ├── automation/     # LFO, modulation routing, param registry
│   ├── config/         # Shared configs (15 headers), preset serialization, effect descriptor table
│   ├── effects/        # Effect modules (98 .cpp files, 98 .h files)
│   ├── render/         # Drawables, shaders, post-processing
│   ├── simulation/     # GPU agent simulations (physarum, boids, curl, particle_life, attractor)
│   ├── ui/             # ImGui panels, widgets, sliders
│   └── main.cpp        # Application entry, frame loop
├── shaders/            # GLSL fragment (.fs) and compute (.glsl)
├── presets/            # JSON preset files (27 presets)
├── fonts/              # UI fonts (Roboto-Medium.ttf, font_atlas.png)
├── scripts/            # Utility scripts (gen_font_atlas.py)
├── docs/               # Documentation and plans
│   ├── plans/          # Active feature plans (0 files)
│   ├── plans/archive/  # Completed plans (2 files)
│   ├── research/       # Effect research docs (14 files)
│   └── research/archive/ # Completed research docs (175 files)
├── build/              # CMake build output (not committed)
├── .claude/            # Claude agent configs and skills
│   ├── agents/         # Specialized agent prompts (2 agents)
│   └── skills/         # Skill definitions (12 skills)
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
- Purpose: Parameter modulation system (LFOs, routing, live control)
- Contains: LFO generators, mod source aggregation, param bounds registry, easing functions
- Key files: `lfo.cpp`, `modulation_engine.cpp`, `param_registry.cpp`, `drawable_params.cpp`, `mod_sources.cpp`, `easing.h`

**`src/config/`:**
- Purpose: Shared configuration structs and preset I/O
- Contains: Master effect config, drawable config, feedback/LFO/modulation configs, JSON serialization, effect descriptor table, attractor type definitions
- Key files: `effect_config.h`, `drawable_config.h`, `preset.cpp`, `lfo_config.h`, `modulation_config.h`, `dual_lissajous_config.h`, `effect_descriptor.h`, `effect_descriptor.cpp`, `effect_serialization.cpp`, `procedural_warp_config.h`, `random_walk_config.h`, `feedback_flow_config.h`, `attractor_types.h`, `constants.h`, `app_configs.h`, `band_config.h`

**`src/effects/`:**
- Purpose: Self-contained effect modules, each owning config, shader resources, and lifecycle
- Contains: 98 effect .cpp files across 17 categories, each providing config struct, Init/Setup/Uninit functions, param registration, and colocated UI
- Categories: Symmetry, Warp, Cellular, Motion, Painterly, Print, Retro, Optical, Color, Simulation, Geometric, Filament, Texture, Field, Novelty, Scatter, Cymatics

**`src/render/`:**
- Purpose: GPU rendering, shader management, post-processing pipeline
- Contains: Drawable types, shader uniform binding, render passes, blend compositor, color LUT, gradient system, noise texture generation
- Key files: `render_pipeline.cpp`, `post_effect.cpp`, `drawable.cpp`, `blend_compositor.cpp`, `shader_setup.cpp`, `color_config.cpp`, `color_lut.cpp`, `gradient.cpp`, `profiler.cpp`, `render_utils.cpp`, `draw_utils.cpp`, `shape.cpp`, `spectrum_bars.cpp`, `waveform.cpp`, `thick_line.cpp`, `noise_texture.cpp`

**`src/simulation/`:**
- Purpose: GPU compute shader agent simulations with colocated UI
- Contains: Physarum slime mold, boids flocking, curl flow, attractors, particle life
- Key files: `physarum.cpp`, `boids.cpp`, `curl_flow.cpp`, `particle_life.cpp`, `attractor_flow.cpp`, `trail_map.cpp`, `spatial_hash.cpp`, `shader_utils.cpp`, `bounds_mode.h`

**`src/ui/`:**
- Purpose: Dear ImGui interface panels and custom widgets
- Contains: Effect panels (dispatch-based), modulatable sliders, gradient editor, drawable controls, loading screen
- Key files: `imgui_panels.cpp`, `imgui_effects.cpp`, `imgui_effects_dispatch.cpp`, `imgui_effects_dispatch.h`, `modulatable_slider.cpp`, `modulatable_drawable_slider.cpp`, `gradient_editor.cpp`, `drawable_type_controls.cpp`, `imgui_widgets.cpp`, `loading_screen.cpp`, `ui_units.h`, `theme.h`

**`shaders/`:**
- Purpose: GLSL shader source files
- Contains: 119 fragment shaders (`.fs`) for post-effects and generators, 8 compute shaders (`.glsl`) for simulations
- Categories mirror `src/effects/` and `src/simulation/` module names

**`presets/`:**
- Purpose: User-saveable visualization configurations
- Contains: JSON files with effect settings, drawables, LFO routes (27 presets)

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

**Core Logic:**
- `src/render/render_pipeline.cpp`: Frame rendering orchestration
- `src/render/post_effect.cpp`: Shader initialization, pass management
- `src/render/shader_setup.cpp`: Uniform binding for all effect categories
- `src/automation/modulation_engine.cpp`: LFO-to-parameter routing
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
- Purpose: Completed feature plans (2 archived)
- Generated: No
- Committed: Yes

**`docs/research/`:**
- Purpose: Effect research and algorithm documentation (14 active)
- Generated: No
- Committed: Yes

**`docs/research/archive/`:**
- Purpose: Completed effect research docs (175 archived)
- Generated: No
- Committed: Yes

**`.claude/`:**
- Purpose: Claude Code agent configurations and skills
- Generated: No
- Committed: Yes

---

*Run `/sync-docs` to regenerate.*
