# Architecture

> Last sync: 2026-02-20 | Commit: 6b8481f

## Pattern Overview

**Overall:** Layered Pipeline Architecture

**Key Characteristics:**
- Single-threaded main loop with background audio callback
- Frame-based render pipeline: capture, analyze, modulate, draw, post-process
- Module isolation via Init/Uninit lifecycle pairs and opaque pointers
- Configuration-driven effects with hot-swappable presets
- Self-contained effect modules encapsulate shader loading, uniform binding, and param registration

## Layers

**Audio Capture Layer:**
- Purpose: Captures system audio output via WASAPI loopback
- Location: `src/audio/`
- Contains: miniaudio device initialization, ring buffer transfer, sample reading
- Depends on: miniaudio library
- Used by: Analysis layer

**Analysis Layer:**
- Purpose: Transforms raw PCM samples into frequency bands, beat triggers, and spectral features
- Location: `src/analysis/`
- Contains: FFT processor, beat detector, band energy calculator, audio feature extractor
- Depends on: Audio capture layer
- Used by: Modulation layer, Drawable layer

**Automation Layer:**
- Purpose: Routes modulation sources (LFOs, audio bands, beat) to effect parameters
- Location: `src/automation/`
- Contains: LFO generators, modulation engine, parameter registry, mod source aggregation
- Depends on: Analysis layer (for audio-reactive sources)
- Used by: Render layer (parameters modulated before draw)

**Configuration Layer:**
- Purpose: Defines all effect parameters and serializes presets to JSON
- Location: `src/config/`
- Contains: `EffectConfig` master struct, per-effect config aggregation, preset I/O, transform ordering, effect descriptor table
- Depends on: Effects layer (imports config structs from effect headers)
- Used by: All layers

**Effects Layer:**
- Purpose: Self-contained post-processing effect modules with shader lifecycle and uniform binding
- Location: `src/effects/`
- Contains: 83 effect modules (`.cpp` + `.h` pairs), each encapsulating config struct, effect struct, Init/Setup/Uninit functions, and param registration
- Depends on: raylib (shader API), automation layer (param registration)
- Used by: Configuration layer (config structs), Render layer (effect structs owned by `PostEffect`)

**Render Layer:**
- Purpose: Orchestrates frame rendering, feedback processing, and multi-pass post-processing
- Location: `src/render/`
- Contains: Render pipeline, `PostEffect` coordinator, shader setup dispatchers (category-based), drawable rendering, blend compositing
- Depends on: Effects layer (owns effect struct instances), Configuration layer, raylib
- Used by: Main loop

**Simulation Layer:**
- Purpose: GPU compute shader agent simulations that generate visual trails
- Location: `src/simulation/`
- Contains: Physarum slime mold, boids flocking, curl flow, curl advection, particle life, attractor flow, cymatics
- Depends on: Render layer (accumulation texture), OpenGL 4.3+
- Used by: Render layer (trail compositing)

**UI Layer:**
- Purpose: ImGui control panels for all parameters
- Location: `src/ui/`
- Contains: Effect panels (category-based), modulatable sliders, gradient editor, dockable panels
- Depends on: Dear ImGui, rlImGui, Configuration layer
- Used by: Main loop

## Data Flow

**Audio-to-Visual Pipeline:**

1. miniaudio callback writes PCM frames to ring buffer (background thread)
2. `AnalysisPipelineProcess` reads ring buffer, computes FFT and beat detection (every frame)
3. `AnalysisPipelineUpdateWaveformHistory` updates waveform ring buffer for cymatics (every frame)
4. `ModSourcesUpdate` aggregates band energies, beat, LFOs into normalized values
5. `ModEngineUpdate` applies modulation routes to registered parameters
6. `RenderPipelineExecute` draws frame: simulations -> feedback -> drawables -> transforms -> output

**Effect Module Lifecycle:**

1. `PostEffectInit` calls each effect's `*EffectInit` to load shaders and cache uniform locations
2. `PostEffectRegisterParams` calls each effect's `*RegisterParams` to expose parameters to modulation
3. Per frame, `shader_setup.cpp` calls each effect's `*EffectSetup` to bind uniforms
4. `PostEffectUninit` calls each effect's `*EffectUninit` to release GPU resources

**Render Pipeline Stages (`RenderPipelineExecute`):**

1. Upload waveform texture for simulation consumption
2. Run GPU simulations (physarum, curl flow, curl advection, attractor flow, particle life, boids, cymatics)
3. Apply feedback effects (flow field warp, blur, decay) to accumulation texture
4. Blit feedback result to output texture for textured shape sampling
5. Draw all drawables (waveforms, spectra, shapes) to accumulation texture
6. Output chain: chromatic aberration -> transforms (user-ordered) -> clarity -> FXAA -> gamma -> screen

**State Management:**
- `AppContext` holds all runtime state (analysis, drawables, effects, LFOs, profiler)
- `EffectConfig` struct aggregates all per-effect config structs from `src/effects/` headers
- `PostEffect` struct owns all effect struct instances (shader handles, uniform locations, animation accumulators)
- `Preset` serializes/deserializes full application state to JSON
- Ring buffer synchronizes audio callback with main thread

## Key Abstractions

**Drawable:**
- Purpose: Audio-reactive visual element (waveform, spectrum, shape, parametric trail)
- Examples: `src/config/drawable_config.h`, `src/render/drawable.cpp`
- Pattern: Tagged union with type-specific data structs

**Effect Module:**
- Purpose: Encapsulates one post-processing effect's shader, uniforms, and config
- Examples: `src/effects/bloom.h` + `src/effects/bloom.cpp`, `src/effects/kaleidoscope.h` + `src/effects/kaleidoscope.cpp`
- Pattern: Paired `*Config` struct (parameters) and `*Effect` struct (GPU state) with Init/Setup/Uninit/RegisterParams functions. Config structs define user-facing parameters. Effect structs store loaded shaders, cached uniform locations, and animation accumulators. Setup functions bind current config values to shader uniforms.

**PostEffect:**
- Purpose: Coordinates all effect modules, manages shared render textures, and owns simulation pointers
- Examples: `src/render/post_effect.h`, `src/render/post_effect.cpp`
- Pattern: Monolithic coordinator struct with Init/Uninit lifecycle. Owns 83 effect struct instances and delegates Init/Setup/Uninit calls to each module. Holds ping-pong render textures, half-res buffers, generator scratch texture, FFT/waveform GPU textures, and blend compositor.

**EffectDescriptor:**
- Purpose: Constexpr table mapping transform enum values to metadata and lifecycle function pointers
- Examples: `src/config/effect_descriptor.h` (`EFFECT_DESCRIPTORS[]`), `src/config/effect_descriptor.cpp`
- Pattern: Each descriptor row contains: name (display), categoryBadge (UI grouping), categorySectionIndex (ordering), enabledOffset (field pointer in EffectConfig), flags bitmask (EFFECT_FLAG_BLEND, EFFECT_FLAG_HALF_RES, EFFECT_FLAG_SIM_BOOST, EFFECT_FLAG_NEEDS_RESIZE), and function pointers for init/uninit/resize/registerParams/getShader/setup. Self-registration macros (`REGISTER_EFFECT`, `REGISTER_GENERATOR`, `REGISTER_SIM_BOOST`, etc.) at the bottom of each effect `.cpp` file populate the table at static-init time, replacing dispersed switch statements and separate lookup tables.

**ModRoute:**
- Purpose: Maps a modulation source to a parameter with amount and easing curve
- Examples: `src/automation/modulation_engine.h`, `src/config/modulation_config.h`
- Pattern: String-keyed routing table with pointer-based parameter access. Easing curves (linear, ease-in/out, spring, elastic, bounce) shape modulation response.

**TrailMap:**
- Purpose: Shared trail texture with diffusion/decay for agent simulations
- Examples: `src/simulation/trail_map.h`, `src/simulation/trail_map.cpp`
- Pattern: GPU texture with compute shader processing

**BlendCompositor:**
- Purpose: Renders generator effects into a scratch texture and composites onto the main chain
- Examples: `src/render/blend_compositor.h`
- Pattern: Shared shader that blends generator output with configurable opacity

## Entry Points

**Application Entry:**
- Location: `src/main.cpp`
- Triggers: Program startup
- Responsibilities: Window init, ImGui/font setup, AppContext creation, main loop, cleanup

**Frame Loop:**
- Location: `src/main.cpp` (lines 206-287)
- Triggers: Every frame at 60 FPS target
- Responsibilities: Window resize handling, audio analysis (every frame), waveform history update, LFO processing, modulation update, visual update (20 Hz), render pipeline execution, UI draw

**Preset Load:**
- Location: `src/config/preset.cpp`
- Triggers: User selects preset file
- Responsibilities: JSON parsing, config population, effect reset, feedback clear

## Thread Model

**Main Thread:**
- Responsibilities: Window events, rendering, UI, analysis processing, modulation updates
- Synchronization: Single-threaded; no explicit locks for main logic

**Audio Callback Thread:**
- Responsibilities: Copies PCM samples from WASAPI to ring buffer
- Synchronization: Lock-free ring buffer (`ma_pcm_rb`) isolates audio from main thread

## Error Handling

**Strategy:** Early return with cleanup on failure

**Patterns:**
- `INIT_OR_FAIL(ptr, expr)`: Assigns result, cleans up and returns NULL on failure
- `CHECK_OR_FAIL(expr)`: Evaluates bool, cleans up and returns NULL on false
- Init functions return NULL or false on failure; callers propagate errors upward
- Effect Init functions cascade cleanup of previously loaded shaders when a later shader fails
- TraceLog reports initialization failures; per-frame errors silently ignored

---

*Run `/sync-docs` to regenerate.*
