# Architecture

> Last sync: 2026-02-06 | Commit: 957e250

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
- Contains: `EffectConfig` master struct, per-effect config aggregation, preset I/O, transform ordering
- Depends on: Effects layer (imports config structs from effect headers)
- Used by: All layers

**Effects Layer:**
- Purpose: Self-contained post-processing effect modules with shader lifecycle and uniform binding
- Location: `src/effects/`
- Contains: 60 effect modules (`.cpp` + `.h` pairs), each encapsulating config struct, effect struct, Init/Setup/Uninit functions, and param registration
- Depends on: raylib (shader API), automation layer (param registration)
- Used by: Configuration layer (config structs), Render layer (effect structs owned by `PostEffect`)

**Render Layer:**
- Purpose: Orchestrates frame rendering, feedback processing, and multi-pass post-processing
- Location: `src/render/`
- Contains: Render pipeline, `PostEffect` coordinator, shader setup dispatchers, drawable rendering, blend compositing
- Depends on: Effects layer (owns effect struct instances), Configuration layer, raylib
- Used by: Main loop

**Simulation Layer:**
- Purpose: GPU compute shader agent simulations that generate visual trails
- Location: `src/simulation/`
- Contains: Physarum slime mold, boids flocking, curl flow, particle life, attractors, cymatics
- Depends on: Render layer (accumulation texture), OpenGL 4.3+
- Used by: Render layer (trail compositing)

**UI Layer:**
- Purpose: ImGui control panels for all parameters
- Location: `src/ui/`
- Contains: Effect panels (category-based), modulatable sliders, gradient editor
- Depends on: Dear ImGui, rlImGui, Configuration layer
- Used by: Main loop

## Data Flow

**Audio-to-Visual Pipeline:**

1. miniaudio callback writes PCM frames to ring buffer (background thread)
2. `AnalysisPipelineProcess` reads ring buffer, computes FFT and beat detection
3. `ModSourcesUpdate` aggregates band energies, beat, LFOs into normalized values
4. `ModEngineUpdate` applies modulation routes to registered parameters
5. `RenderPipelineExecute` draws frame: simulations -> feedback -> drawables -> transforms -> output

**Effect Module Lifecycle:**

1. `PostEffectInit` calls each effect's `*EffectInit` to load shaders and cache uniform locations
2. `PostEffectRegisterParams` calls each effect's `*RegisterParams` to expose parameters to modulation
3. Per frame, `shader_setup_*.cpp` dispatchers call each effect's `*EffectSetup` to bind uniforms
4. `PostEffectUninit` calls each effect's `*EffectUninit` to release GPU resources

**State Management:**
- `AppContext` holds all runtime state (analysis, drawables, effects, LFOs)
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
- Pattern: Monolithic coordinator struct with Init/Uninit lifecycle. Owns 60 effect struct instances and delegates Init/Setup/Uninit calls to each module.

**TransformEffectEntry:**
- Purpose: Maps a transform enum value to its shader, setup function, and enabled flag
- Examples: `src/render/shader_setup.cpp` (`GetTransformEffect` switch)
- Pattern: Returned by `GetTransformEffect` to let the render pipeline apply effects generically without knowing each effect's internals

**ModRoute:**
- Purpose: Maps a modulation source to a parameter with amount and easing curve
- Examples: `src/automation/modulation_engine.h`, `src/config/modulation_config.h`
- Pattern: String-keyed routing table with pointer-based parameter access

**TrailMap:**
- Purpose: Shared trail texture with diffusion/decay for agent simulations
- Examples: `src/simulation/trail_map.h`, `src/simulation/trail_map.cpp`
- Pattern: GPU texture with compute shader processing

## Entry Points

**Application Entry:**
- Location: `src/main.cpp`
- Triggers: Program startup
- Responsibilities: Window init, AppContext creation, main loop, cleanup

**Frame Loop:**
- Location: `src/main.cpp:205-286`
- Triggers: Every frame at 60 FPS target
- Responsibilities: Audio analysis, modulation update, render pipeline execution, UI draw

**Preset Load:**
- Location: `src/config/preset.cpp`
- Triggers: User selects preset file
- Responsibilities: JSON parsing, config population, effect reset

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
- TraceLog reports initialization failures; per-frame errors silently ignored

---

*Run `/sync-docs` to regenerate.*
