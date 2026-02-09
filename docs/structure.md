# Codebase Structure

> Last sync: 2026-02-08 | Commit: 88d66c3

## Codebase Size

| Language | Files | Code |
|----------|-------|------|
| C++ (.cpp) | 141 | 19,579 |
| C++ Headers (.h) | 146 | 5,788 |
| GLSL | 97 | 6,634 |
| **Total** | 384 | 32,001 |

## Directory Layout

```
AudioJones/
├── src/                # C++ source code
│   ├── analysis/       # FFT, beat detection, audio features
│   ├── audio/          # WASAPI loopback capture
│   ├── automation/     # LFO, modulation routing, param registry
│   ├── config/         # Shared configs (11 headers), preset serialization
│   ├── effects/        # Effect modules (70 .cpp/.h pairs)
│   ├── render/         # Drawables, shaders, post-processing
│   ├── simulation/     # GPU agent simulations (physarum, boids, curl, particle_life)
│   ├── ui/             # ImGui panels, widgets, sliders
│   └── main.cpp        # Application entry, frame loop
├── shaders/            # GLSL fragment (.fs) and compute (.glsl)
├── presets/            # JSON preset files (21 presets)
├── fonts/              # UI fonts (Roboto-Medium.ttf, font_atlas.png)
├── scripts/            # Utility scripts (gen_font_atlas.py)
├── docs/               # Documentation and plans
│   ├── plans/          # Active feature plans (1 file)
│   ├── plans/archive/  # Completed plans (351 files)
│   └── research/       # Effect research docs (103 files)
├── build/              # CMake build output (not committed)
├── .claude/            # Claude agent configs and skills
│   ├── agents/         # Specialized agent prompts (2 agents)
│   └── skills/         # Skill definitions (9 skills)
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
- Contains: Master effect config, drawable config, feedback/LFO/modulation configs, JSON serialization
- Key files: `effect_config.h`, `drawable_config.h`, `preset.cpp`, `lfo_config.h`, `modulation_config.h`, `dual_lissajous_config.h`

**`src/effects/`:**
- Purpose: Self-contained effect modules, each owning config, shader resources, and lifecycle
- Contains: 70 effect modules as .cpp/.h pairs (e.g., `bloom.cpp`/`bloom.h`, `kaleidoscope.cpp`/`kaleidoscope.h`)
- Each module provides: config struct, Init/Setup/Uninit functions, param registration
- Categories: artistic (oil paint, watercolor, impressionist, ink wash, pencil sketch, cross hatching), cellular (voronoi, lattice fold, phyllotaxis, disco ball), color (color grade, false color, palette quantization), generators (plasma, interference, constellation, circuit board, moire generator, moire interference, filaments, muons, slashes, spectral arcs, signal frames, arc strobe), graphic (toon, neon glow, kuwahara, halftone, synthwave), motion (infinite zoom, radial streak, droste zoom, density wave spiral, anamorphic streak), optical (bloom, bokeh, heightfield relief), retro (pixelation, glitch, ASCII art, matrix rain, lego bricks, crt, dot matrix, glyph field), symmetry (kaleidoscope, KIFS, poincare disk, radial pulse, mandelbox, triangle fold, moire, radial IFS), warp (sine warp, texture warp, gradient flow, wave ripple, mobius, chladni, domain warp, surface warp, interference warp, corridor warp)

**`src/render/`:**
- Purpose: GPU rendering, shader management, post-processing pipeline
- Contains: Drawable types, shader uniform binding (category-based modules), render passes
- Key files: `render_pipeline.cpp`, `post_effect.cpp`, `drawable.cpp`, `blend_compositor.cpp`
- Shader setup modules: `shader_setup.cpp` (dispatcher), `shader_setup_artistic.cpp`, `shader_setup_cellular.cpp`, `shader_setup_color.cpp`, `shader_setup_generators.cpp`, `shader_setup_graphic.cpp`, `shader_setup_motion.cpp`, `shader_setup_optical.cpp`, `shader_setup_retro.cpp`, `shader_setup_symmetry.cpp`, `shader_setup_warp.cpp`

**`src/simulation/`:**
- Purpose: GPU compute shader agent simulations
- Contains: Physarum slime mold, boids flocking, curl flow, attractors, cymatics, particle life
- Key files: `physarum.cpp`, `boids.cpp`, `curl_flow.cpp`, `particle_life.cpp`, `attractor_flow.cpp`, `cymatics.cpp`, `curl_advection.cpp`, `trail_map.cpp`, `spatial_hash.cpp`

**`src/ui/`:**
- Purpose: Dear ImGui interface panels and custom widgets
- Contains: Effect panels (category-based), modulatable sliders, gradient editor
- Key files: `imgui_panels.cpp`, `imgui_effects.cpp`, `modulatable_slider.cpp`, `modulatable_drawable_slider.cpp`
- Effect UI modules: `imgui_effects_artistic.cpp`, `imgui_effects_cellular.cpp`, `imgui_effects_color.cpp`, `imgui_effects_generators.cpp`, `imgui_effects_graphic.cpp`, `imgui_effects_motion.cpp`, `imgui_effects_optical.cpp`, `imgui_effects_retro.cpp`, `imgui_effects_symmetry.cpp`, `imgui_effects_warp.cpp`

**`shaders/`:**
- Purpose: GLSL shader source files
- Contains: Fragment shaders (87 `.fs` files) for post-effects, compute shaders (10 `.glsl` files) for simulations
- Key files: `feedback.fs`, `kaleidoscope.fs`, `disco_ball.fs`, `glitch.fs`, `surface_warp.fs`, `physarum_agents.glsl`, `boids_agents.glsl`, `particle_life_agents.glsl`

**`presets/`:**
- Purpose: User-saveable visualization configurations
- Contains: JSON files with effect settings, drawables, LFO routes (21 presets)
- Key files: `SMOOTHBOB.json`, `GALACTO.json`, `GLITCHYBOB.json`, `BOIDIUS.json`, `CYMATICBOB.json`

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

**Core Logic:**
- `src/render/render_pipeline.cpp`: Frame rendering orchestration
- `src/render/post_effect.cpp`: Shader initialization, pass management
- `src/render/shader_setup.cpp`: Dispatcher to category-based uniform binding modules
- `src/automation/modulation_engine.cpp`: LFO-to-parameter routing

**Effect Modules (each provides config, Init/Setup/Uninit, param registration):**
- `src/effects/bloom.cpp`: Dual Kawase blur with soft threshold extraction
- `src/effects/kaleidoscope.cpp`: Angular mirroring with configurable segments
- `src/effects/glitch.cpp`: Digital artifact simulation
- `src/effects/voronoi.cpp`: Voronoi cell tessellation
- `src/effects/plasma.cpp`: Procedural plasma generator
- Full list: 70 modules in `src/effects/`

**Shader Setup Modules (category-based):**
- `src/render/shader_setup_artistic.cpp`: Oil paint, watercolor, impressionist, ink wash, pencil sketch, cross hatching
- `src/render/shader_setup_cellular.cpp`: Voronoi, lattice fold, phyllotaxis, disco ball
- `src/render/shader_setup_color.cpp`: Color grading, false color, palette quantization
- `src/render/shader_setup_generators.cpp`: Procedural content generators (plasma, interference, constellation, circuit board, moire generator, moire interference, filaments, muons, slashes, spectral arcs, signal frames, arc strobe)
- `src/render/shader_setup_graphic.cpp`: Toon, neon glow, kuwahara, halftone, synthwave
- `src/render/shader_setup_motion.cpp`: Infinite zoom, radial streak, droste zoom, density wave spiral, anamorphic streak
- `src/render/shader_setup_optical.cpp`: Bloom, bokeh, heightfield relief
- `src/render/shader_setup_retro.cpp`: Pixelation, glitch, ASCII art, matrix rain, lego bricks, crt, dot matrix, glyph field
- `src/render/shader_setup_symmetry.cpp`: Kaleidoscope, KIFS, poincare disk, radial pulse, mandelbox, triangle fold, moire, radial IFS
- `src/render/shader_setup_warp.cpp`: Sine warp, texture warp, gradient flow, wave ripple, mobius, chladni, domain warp, surface warp, interference warp, corridor warp

## Naming Conventions

**Files:**
- `src/effects/<name>.cpp` / `src/effects/<name>.h`: Self-contained effect module (e.g., `bloom.cpp`/`bloom.h`)
- `*_config.h` (in `src/config/`): Shared configuration structs (e.g., `effect_config.h`, `lfo_config.h`)
- `imgui_*.cpp`: ImGui panel implementation (e.g., `imgui_effects.cpp`)
- `imgui_effects_*.cpp`: Category-specific effect UI (e.g., `imgui_effects_warp.cpp`)
- `shader_setup_*.cpp`: Category-specific uniform binding (e.g., `shader_setup_color.cpp`)
- `*.fs`: Fragment shader (e.g., `kaleidoscope.fs`, `glitch.fs`)
- `*_agents.glsl`: Compute shader for agent simulation (e.g., `physarum_agents.glsl`, `particle_life_agents.glsl`)

**Directories:**
- Module directories match CMakeLists.txt source groups: `analysis`, `audio`, `automation`, `config`, `effects`, `render`, `simulation`, `ui`

## Where to Add New Code

**New Post-Processing Effect:**
- Effect module: `src/effects/<effect>.cpp` and `src/effects/<effect>.h` (config struct, Init/Setup/Uninit, param registration)
- Shader: `shaders/<effect>.fs`
- Shader loading: `src/render/post_effect.cpp` (add Shader field)
- Uniform binding: `src/render/shader_setup_<category>.cpp` (add to appropriate category module)
- Render pass: `src/render/render_pipeline.cpp`
- Transform enum: `src/config/effect_config.h` (TransformEffectType)
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
- Purpose: Completed feature plans (351 archived)
- Generated: No
- Committed: Yes

**`docs/research/`:**
- Purpose: Effect research and algorithm documentation (103 files)
- Generated: No
- Committed: Yes

**`.claude/`:**
- Purpose: Claude Code agent configurations and skills
- Generated: No
- Committed: Yes

---

*Run `/sync-docs` to regenerate.*
