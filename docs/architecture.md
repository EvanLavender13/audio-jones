# Architecture

> Last sync: 2026-01-29 | Commit: 176b35f

## Pattern Overview
**Overall:** Pipeline architecture with layered separation

**Key Characteristics:**
- Single-threaded main loop with fixed 60 FPS render and 20 Hz visual updates
- GPU compute shaders for agent simulations (physarum, boids, curl flow)
- Modulation engine routes audio-reactive signals to effect parameters
- Ping-pong buffer pattern for multi-pass shader effects

## Layers
**Audio Capture (`src/audio/`):**
- Purpose: Captures system audio via WASAPI loopback
- Location: `src/audio/`
- Contains: Ring buffer, device enumeration, capture lifecycle
- Depends on: miniaudio
- Used by: AnalysisPipeline

**Analysis (`src/analysis/`):**
- Purpose: Transforms raw PCM into usable features (FFT, beat, bands)
- Location: `src/analysis/`
- Contains: FFT processor, beat detector, band energies, spectral features
- Depends on: Audio layer
- Used by: ModSources, Drawables

**Automation (`src/automation/`):**
- Purpose: Parameter modulation from audio features and LFOs
- Location: `src/automation/`
- Contains: LFO generators, modulation engine, parameter registry
- Depends on: Analysis layer (band energies, beat, features)
- Used by: EffectConfig parameters, Drawable parameters

**Configuration (`src/config/`):**
- Purpose: Defines all effect parameters and preset serialization
- Location: `src/config/`
- Contains: Per-effect config structs, JSON preset I/O
- Depends on: None
- Used by: PostEffect, UI panels, Modulation engine

**Render (`src/render/`):**
- Purpose: GPU rendering, shader management, post-processing pipeline
- Location: `src/render/`
- Contains: Drawable rendering, shader uniform binding, render passes
- Depends on: raylib, Configuration layer
- Used by: Main loop

**Simulation (`src/simulation/`):**
- Purpose: GPU compute shader agent simulations
- Location: `src/simulation/`
- Contains: Physarum, boids, curl flow, attractors, cymatics, particle life
- Depends on: raylib compute shaders, trail map
- Used by: RenderPipeline (feedback stage)

**UI (`src/ui/`):**
- Purpose: Dear ImGui interface panels and custom widgets
- Location: `src/ui/`
- Contains: Effect panels, modulatable sliders, preset browser
- Depends on: Dear ImGui, rlImGui, Configuration layer
- Used by: Main loop

## Data Flow
**Audio to Visualization:**
1. `AudioCapture` reads system audio into ring buffer (WASAPI loopback at 48kHz)
2. `AnalysisPipeline` consumes audio: FFT (2048-point), beat detection, band energies
3. `ModSources` aggregates analysis outputs (bass, mid, treb, beat, spectral features)
4. `ModEngine` routes modulation sources to registered effect/drawable parameters
5. `DrawableState` processes audio into waveform/spectrum buffers at 20 Hz
6. `RenderPipeline` executes frame: simulations, feedback, drawables, output chain

**Render Frame Pipeline:**
1. GPU simulations update (physarum, boids, curl flow, attractors, cymatics)
2. Feedback pass applies: decay, blur, flow field displacement
3. Drawables render to accumulation buffer (waveforms, shapes, spectrum bars)
4. Output chain applies: chromatic aberration, transform effects, FXAA, gamma

**State Management:**
- `AppContext` owns all application state (analysis, drawables, effects, modulation)
- `PostEffect` holds shader handles, uniform locations, render textures
- `EffectConfig` stores all effect parameters as plain struct fields
- Modulation engine stores base values separately; applies offsets each frame

## Key Abstractions
**PostEffect:**
- Purpose: Manages all shaders, render textures, and simulation pointers
- Examples: `src/render/post_effect.h`, `src/render/post_effect.cpp`
- Pattern: Init/Uninit lifecycle, ping-pong buffers for multi-pass effects

**AnalysisPipeline:**
- Purpose: Encapsulates audio-to-features processing
- Examples: `src/analysis/analysis_pipeline.h`, `src/analysis/analysis_pipeline.cpp`
- Pattern: Feed audio, update FFT, derive beat/band/feature signals

**ModEngine:**
- Purpose: Routes modulation sources to effect parameters
- Examples: `src/automation/modulation_engine.h`, `src/automation/modulation_engine.cpp`
- Pattern: Register param (id, ptr, min, max), set route (source, amount, curve), update each frame

**Drawable:**
- Purpose: Renderable visualization element (waveform, spectrum, shape)
- Examples: `src/render/drawable.h`, `src/config/drawable_config.h`
- Pattern: Type union with per-type config, processed at 20 Hz, rendered at 60 FPS

**EffectConfig:**
- Purpose: Central struct holding all post-processing effect parameters
- Examples: `src/config/effect_config.h`
- Pattern: Nested config structs per effect, transform order array for execution sequence

## Entry Points
**Application Entry:**
- Location: `src/main.cpp`
- Triggers: Program startup
- Responsibilities: Window init, AppContext creation, main loop, cleanup

**Main Loop (`src/main.cpp:171-252`):**
- Location: `src/main.cpp`
- Triggers: Every frame (60 FPS target)
- Responsibilities: Audio analysis (every frame), visual updates (20 Hz), modulation routing, render pipeline execution, UI drawing

**RenderPipelineExecute:**
- Location: `src/render/render_pipeline.cpp`
- Triggers: Once per frame from main loop
- Responsibilities: Simulation passes, feedback stage, drawable rendering, output chain

## Thread Model
**Main Thread:**
- Responsibilities: All application logic, rendering, UI, audio processing
- Synchronization: None required (single-threaded)

**Audio Callback (miniaudio):**
- Responsibilities: Writes captured audio to ring buffer
- Synchronization: Lock-free ring buffer between capture callback and main thread read

## Error Handling
**Strategy:** Early return with cleanup via RAII-style macros

**Patterns:**
- `INIT_OR_FAIL(ptr, expr)`: Assign result, cleanup and return NULL on failure
- `CHECK_OR_FAIL(expr)`: Evaluate bool, cleanup and return NULL on false
- Init functions return `bool` or pointer (NULL on failure)
- Uninit functions accept NULL pointers safely

---

*Run `/sync-docs` to regenerate.*
