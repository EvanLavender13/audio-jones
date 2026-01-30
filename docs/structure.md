# Codebase Structure

> Last sync: 2026-01-29 | Commit: 176b35f

## Directory Layout

```
AudioJones/
├── src/                # C++ source code
│   ├── analysis/       # FFT, beat detection, audio features
│   ├── audio/          # WASAPI loopback capture
│   ├── automation/     # LFO, modulation routing, param registry
│   ├── config/         # Effect configs, preset serialization
│   ├── render/         # Drawables, shaders, post-processing
│   ├── simulation/     # GPU agent simulations (physarum, boids, curl, particle_life)
│   ├── ui/             # ImGui panels, widgets, sliders
│   └── main.cpp        # Application entry, frame loop
├── shaders/            # GLSL fragment (.fs) and compute (.glsl)
├── presets/            # JSON preset files
├── fonts/              # UI fonts (Roboto-Medium.ttf)
├── docs/               # Documentation and plans
│   ├── plans/          # Active feature plans
│   └── plans/archive/  # Completed plans
├── build/              # CMake build output (not committed)
├── .claude/            # Claude agent configs and skills
│   ├── agents/         # Specialized agent prompts
│   └── skills/         # Skill definitions (add-effect, commit, etc.)
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
- Purpose: Configuration structs for all effects and preset I/O
- Contains: Per-effect config headers (50+ effects), JSON serialization
- Key files: `effect_config.h`, `drawable_config.h`, `preset.cpp`, `lfo_config.h`, `modulation_config.h`

**`src/render/`:**
- Purpose: GPU rendering, shader management, post-processing pipeline
- Contains: Drawable types, shader uniform binding (category-based modules), render passes
- Key files: `render_pipeline.cpp`, `post_effect.cpp`, `drawable.cpp`, `blend_compositor.cpp`
- Shader setup modules: `shader_setup.cpp` (dispatcher), `shader_setup_cellular.cpp`, `shader_setup_color.cpp`, `shader_setup_motion.cpp`, `shader_setup_style.cpp`, `shader_setup_symmetry.cpp`, `shader_setup_warp.cpp`

**`src/simulation/`:**
- Purpose: GPU compute shader agent simulations
- Contains: Physarum slime mold, boids flocking, curl flow, attractors, cymatics, particle life
- Key files: `physarum.cpp`, `boids.cpp`, `curl_flow.cpp`, `particle_life.cpp`, `trail_map.cpp`, `spatial_hash.cpp`

**`src/ui/`:**
- Purpose: Dear ImGui interface panels and custom widgets
- Contains: Effect panels (category-based), modulatable sliders, gradient editor
- Key files: `imgui_panels.cpp`, `imgui_effects.cpp`, `modulatable_slider.cpp`, `modulatable_drawable_slider.cpp`
- Effect UI modules: `imgui_effects_cellular.cpp`, `imgui_effects_color.cpp`, `imgui_effects_motion.cpp`, `imgui_effects_style.cpp`, `imgui_effects_symmetry.cpp`, `imgui_effects_warp.cpp`

**`shaders/`:**
- Purpose: GLSL shader source files
- Contains: Fragment shaders (`.fs`) for post-effects, compute shaders (`.glsl`) for simulations
- Key files: `feedback.fs`, `kaleidoscope.fs`, `disco_ball.fs`, `glitch.fs`, `surface_warp.fs`, `physarum_agents.glsl`, `boids_agents.glsl`, `particle_life_agents.glsl`

**`presets/`:**
- Purpose: User-saveable visualization configurations
- Contains: JSON files with effect settings, drawables, LFO routes
- Key files: `*.json` (e.g., `SMOOTHBOB.json`, `GALACTO.json`, `GLITCHYBOB.json`)

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Application initialization, main loop, teardown

**Configuration:**
- `src/config/effect_config.h`: Master effect config struct, transform ordering
- `src/config/drawable_config.h`: Drawable types (waveform, spectrum, shape)
- `src/config/preset.cpp`: JSON serialization/deserialization

**Core Logic:**
- `src/render/render_pipeline.cpp`: Frame rendering orchestration
- `src/render/post_effect.cpp`: Shader initialization, pass management
- `src/render/shader_setup.cpp`: Dispatcher to category-based uniform binding modules
- `src/automation/modulation_engine.cpp`: LFO-to-parameter routing

**Shader Setup Modules (category-based):**
- `src/render/shader_setup_cellular.cpp`: Voronoi, phyllotaxis, moire effects
- `src/render/shader_setup_color.cpp`: Color grading, false color, palette quantization
- `src/render/shader_setup_motion.cpp`: Feedback, radial streak, infinite zoom
- `src/render/shader_setup_style.cpp`: Artistic filters (oil paint, ink wash, toon, halftone)
- `src/render/shader_setup_symmetry.cpp`: Kaleidoscope, mobius, poincare disk, KIFS
- `src/render/shader_setup_warp.cpp`: Domain warp, sine warp, surface warp, texture warp

## Naming Conventions

**Files:**
- `*_config.h`: Configuration struct for one effect (e.g., `bloom_config.h`, `disco_ball_config.h`)
- `imgui_*.cpp`: ImGui panel implementation (e.g., `imgui_effects.cpp`)
- `imgui_effects_*.cpp`: Category-specific effect UI (e.g., `imgui_effects_warp.cpp`)
- `shader_setup_*.cpp`: Category-specific uniform binding (e.g., `shader_setup_color.cpp`)
- `*.fs`: Fragment shader (e.g., `kaleidoscope.fs`, `glitch.fs`)
- `*_agents.glsl`: Compute shader for agent simulation (e.g., `physarum_agents.glsl`, `particle_life_agents.glsl`)

**Directories:**
- Module directories match CMakeLists.txt source groups: `analysis`, `audio`, `automation`, `config`, `render`, `simulation`, `ui`

## Where to Add New Code

**New Post-Processing Effect:**
- Config struct: `src/config/<effect>_config.h`
- Shader: `shaders/<effect>.fs`
- Shader loading: `src/render/post_effect.cpp` (add Shader field)
- Uniform binding: `src/render/shader_setup_<category>.cpp` (add to appropriate category module)
- Render pass: `src/render/render_pipeline.cpp`
- Transform enum: `src/config/effect_config.h` (TransformEffectType)
- UI panel: `src/ui/imgui_effects_<category>.cpp` (add to appropriate category file)
- Param registration: `src/automation/param_registry.cpp` (PARAM_TABLE + ParamRegistryInit)

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

**New Shader Setup Category:**
- Implementation: `src/render/shader_setup_<category>.cpp` and `shader_setup_<category>.h`
- Dispatcher: Add include and dispatch case in `src/render/shader_setup.cpp`
- Header: Add function declaration to `src/render/shader_setup_<category>.h`

## Special Directories

**`build/`:**
- Purpose: CMake build artifacts, compiled binaries
- Generated: Yes
- Committed: No

**`docs/plans/`:**
- Purpose: Active feature plans under development
- Generated: No
- Committed: Yes

**`docs/plans/archive/`:**
- Purpose: Completed feature plans (historical reference)
- Generated: No
- Committed: Yes

**`.claude/`:**
- Purpose: Claude Code agent configurations and skills
- Generated: No
- Committed: Yes

---

*Run `/sync-docs` to regenerate.*
