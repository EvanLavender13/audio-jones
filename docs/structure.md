# Codebase Structure

> Last sync: 2026-02-20 | Commit: 6b8481f

## Codebase Size

| Language | Files | Code |
|----------|-------|------|
| C++ (.cpp) | 150 | 21,550 |
| C++ Headers (.h) | 149 | 6,917 |
| GLSL | 111 | 8,238 |
| **Total** | 410 | 36,705 |

## Directory Layout

```
AudioJones/
├── src/                # C++ source code
│   ├── analysis/       # FFT, beat detection, audio features
│   ├── audio/          # WASAPI loopback capture
│   ├── automation/     # LFO, modulation routing, param registry
│   ├── config/         # Shared configs (15 headers), preset serialization, effect descriptor table
│   ├── effects/        # Effect modules (83 .cpp/.h pairs)
│   ├── render/         # Drawables, shaders, post-processing
│   ├── simulation/     # GPU agent simulations (physarum, boids, curl, particle_life)
│   ├── ui/             # ImGui panels, widgets, sliders
│   └── main.cpp        # Application entry, frame loop
├── shaders/            # GLSL fragment (.fs) and compute (.glsl)
├── presets/            # JSON preset files (24 presets)
├── fonts/              # UI fonts (Roboto-Medium.ttf, font_atlas.png)
├── scripts/            # Utility scripts (gen_font_atlas.py)
├── docs/               # Documentation and plans
│   ├── plans/          # Active feature plans (1 file)
│   ├── plans/archive/  # Completed plans (398 files)
│   ├── research/       # Effect research docs (12 files)
│   └── research/archive/ # Completed research docs (128 files)
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
- Key files: `fft.cpp`, `beat.cpp`, `bands.cpp`, `analysis_pipeline.cpp`, `audio_features.cpp`

**`src/audio/`:**
- Purpose: Windows loopback audio capture via miniaudio/WASAPI
- Contains: Audio device enumeration, ring buffer, capture lifecycle
- Key files: `audio.cpp`, `audio_config.h`

**`src/automation/`:**
- Purpose: Parameter modulation system (LFOs, routing, live control)
- Contains: LFO generators, mod source aggregation, param bounds registry
- Key files: `lfo.cpp`, `modulation_engine.cpp`, `param_registry.cpp`, `drawable_params.cpp`, `mod_sources.cpp`

**`src/config/`:**
- Purpose: Shared configuration structs and preset I/O
- Contains: Master effect config, drawable config, feedback/LFO/modulation configs, JSON serialization, effect descriptor table, attractor type definitions
- Key files: `effect_config.h`, `drawable_config.h`, `preset.cpp`, `lfo_config.h`, `modulation_config.h`, `dual_lissajous_config.h`, `effect_descriptor.h`, `effect_descriptor.cpp`, `procedural_warp_config.h`, `random_walk_config.h`, `feedback_flow_config.h`, `attractor_types.h`

**`src/effects/`:**
- Purpose: Self-contained effect modules, each owning config, shader resources, and lifecycle
- Contains: 83 effect modules as .cpp/.h pairs, each providing config struct, Init/Setup/Uninit functions, param registration
- Categories: artistic, cellular, color, generators (geometric, filament, texture, atmosphere), graphic, motion, optical, retro, symmetry, warp

**`src/render/`:**
- Purpose: GPU rendering, shader management, post-processing pipeline
- Contains: Drawable types, shader uniform binding, render passes
- Key files: `render_pipeline.cpp`, `post_effect.cpp`, `drawable.cpp`, `blend_compositor.cpp`, `shader_setup.cpp`, `color_config.cpp`, `color_lut.cpp`

**`src/simulation/`:**
- Purpose: GPU compute shader agent simulations
- Contains: Physarum slime mold, boids flocking, curl flow, attractors, cymatics, particle life, curl advection
- Key files: `physarum.cpp`, `boids.cpp`, `curl_flow.cpp`, `particle_life.cpp`, `attractor_flow.cpp`, `cymatics.cpp`, `curl_advection.cpp`, `trail_map.cpp`, `spatial_hash.cpp`

**`src/ui/`:**
- Purpose: Dear ImGui interface panels and custom widgets
- Contains: Effect panels (category-based), modulatable sliders, gradient editor
- Key files: `imgui_panels.cpp`, `imgui_effects.cpp`, `modulatable_slider.cpp`, `modulatable_drawable_slider.cpp`
- Effect UI modules: `imgui_effects_artistic.cpp`, `imgui_effects_cellular.cpp`, `imgui_effects_color.cpp`, `imgui_effects_generators.cpp`, `imgui_effects_graphic.cpp`, `imgui_effects_motion.cpp`, `imgui_effects_optical.cpp`, `imgui_effects_retro.cpp`, `imgui_effects_symmetry.cpp`, `imgui_effects_warp.cpp`
- Generator UI sub-category modules: `imgui_effects_gen_geometric.cpp`, `imgui_effects_gen_filament.cpp`, `imgui_effects_gen_texture.cpp`, `imgui_effects_gen_atmosphere.cpp`

**`shaders/`:**
- Purpose: GLSL shader source files
- Contains: 101 fragment shaders (`.fs`) for post-effects, 10 compute shaders (`.glsl`) for simulations

**`presets/`:**
- Purpose: User-saveable visualization configurations
- Contains: JSON files with effect settings, drawables, LFO routes (24 presets)
- Key files: `SMOOTHBOB.json`, `GALACTO.json`, `GLITCHYBOB.json`, `CYMATICBOB.json`, `SPIROL.json`

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

**Core Logic:**
- `src/render/render_pipeline.cpp`: Frame rendering orchestration
- `src/render/post_effect.cpp`: Shader initialization, pass management
- `src/render/shader_setup.cpp`: Uniform binding for all effect categories
- `src/automation/modulation_engine.cpp`: LFO-to-parameter routing

## Naming Conventions

**Files:**
- `src/effects/<name>.cpp` / `src/effects/<name>.h`: Self-contained effect module (e.g., `bloom.cpp`/`bloom.h`)
- `*_config.h` (in `src/config/`): Shared configuration structs (e.g., `effect_config.h`, `lfo_config.h`)
- `imgui_*.cpp`: ImGui panel implementation (e.g., `imgui_effects.cpp`)
- `imgui_effects_*.cpp`: Category-specific effect UI (e.g., `imgui_effects_warp.cpp`)
- `imgui_effects_gen_*.cpp`: Generator sub-category UI (e.g., `imgui_effects_gen_geometric.cpp`)
- `*.fs`: Fragment shader (e.g., `kaleidoscope.fs`, `glitch.fs`)
- `*_agents.glsl`: Compute shader for agent simulation (e.g., `physarum_agents.glsl`, `particle_life_agents.glsl`)

**Directories:**
- Module directories match CMakeLists.txt source groups: `analysis`, `audio`, `automation`, `config`, `effects`, `render`, `simulation`, `ui`

## Where to Add New Code

**New Post-Processing Effect:**
- Effect module: `src/effects/<effect>.cpp` and `src/effects/<effect>.h` (config struct, Init/Setup/Uninit, param registration)
- Shader: `shaders/<effect>.fs`
- Shader loading: `src/render/post_effect.cpp` (add Shader field)
- Uniform binding: `src/render/shader_setup.cpp` (add Setup function)
- Render pass: `src/render/render_pipeline.cpp`
- Transform enum: `src/config/effect_config.h` (TransformEffectType)
- Effect descriptor: `src/config/effect_descriptor.cpp` (add row to EFFECT_DESCRIPTORS table)
- UI panel: `src/ui/imgui_effects_<category>.cpp` (add to appropriate category file)

**New Agent Simulation:**
- Config struct: Add to `src/config/effect_config.h` or dedicated `*_config.h`
- Compute shader: `shaders/<name>_agents.glsl`
- Implementation: `src/simulation/<name>.cpp` and `src/simulation/<name>.h`
- Trail map: Use `src/simulation/trail_map.cpp` for diffusion/decay
- Spatial hashing: Use `src/simulation/spatial_hash.cpp` for neighbor queries
- PostEffect integration: Add pointer to `src/render/post_effect.h`

**New Drawable Type:**
- Config: Extend union in `src/config/drawable_config.h`
- Rendering: Add case in `src/render/drawable.cpp`
- UI controls: Add to `src/ui/drawable_type_controls.cpp`
- Param registration: `src/automation/drawable_params.cpp`

**New UI Panel:**
- Implementation: `src/ui/imgui_<panel>.cpp`
- Registration: Add to `CMakeLists.txt` IMGUI_UI_SOURCES
- Entry: Call from `src/ui/imgui_panels.cpp`

**New Modulatable Parameter:**
- Effect params: Add bounds to `PARAM_TABLE` in `param_registry.cpp`, register pointer in `ParamRegistryInit()`
- Drawable params: Add bounds to `DRAWABLE_FIELD_TABLE` in `param_registry.cpp`, register pointer in `drawable_params.cpp`

## Special Directories

**`build/`:**
- Purpose: CMake build artifacts, compiled binaries
- Generated: Yes
- Committed: No

**`docs/plans/`:**
- Purpose: Active feature plans under development (1 active)
- Generated: No
- Committed: Yes

**`docs/plans/archive/`:**
- Purpose: Completed feature plans (398 archived)
- Generated: No
- Committed: Yes

**`docs/research/`:**
- Purpose: Effect research and algorithm documentation (12 active)
- Generated: No
- Committed: Yes

**`docs/research/archive/`:**
- Purpose: Completed effect research docs (128 archived)
- Generated: No
- Committed: Yes

**`.claude/`:**
- Purpose: Claude Code agent configurations and skills
- Generated: No
- Committed: Yes

---

*Run `/sync-docs` to regenerate.*
