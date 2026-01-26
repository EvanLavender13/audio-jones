# Architecture

> Last sync: 2026-01-25 | Commit: 990f2e7

## Pattern Overview

**Overall:** Pipeline Architecture with Component Composition

**Key Characteristics:**
- Single-threaded main loop with audio callback on separate thread
- Multi-pass GPU render pipeline with ping-pong buffers
- Modular config structs with in-class defaults
- Init/Uninit lifecycle pairs for resource management

## Layers

**Audio Layer:**
- Purpose: Captures system audio via WASAPI loopback
- Location: `src/audio/`
- Contains: miniaudio device init, ring buffer for thread-safe transfer
- Depends on: miniaudio library
- Used by: Analysis layer

**Analysis Layer:**
- Purpose: Extracts audio features (FFT, beat detection, frequency bands)
- Location: `src/analysis/`
- Contains: FFT processor (kiss_fft), beat detector, band energies, audio features
- Depends on: Audio layer
- Used by: Main loop, Modulation engine

**Automation Layer:**
- Purpose: Routes modulation sources to effect/drawable parameters
- Location: `src/automation/`
- Contains: LFO state, modulation engine, parameter registry, drawable params
- Depends on: Analysis layer (band energies, beat, features)
- Used by: Main loop (parameter animation)

**Config Layer:**
- Purpose: Defines all effect and drawable configuration structs
- Location: `src/config/`
- Contains: Per-effect config structs with defaults, preset serialization
- Depends on: Nothing (pure data)
- Used by: Render layer, UI layer, Preset system

**Render Layer:**
- Purpose: GPU rendering pipeline with multi-pass effects
- Location: `src/render/`
- Contains: PostEffect, RenderPipeline, Drawable, shaders, blend compositor
- Depends on: Config layer, raylib
- Used by: Main loop

**Simulation Layer:**
- Purpose: GPU compute simulations (physarum, boids, curl flow)
- Location: `src/simulation/`
- Contains: Compute shader wrappers, trail maps, spatial hash
- Depends on: OpenGL 4.3+ compute shaders
- Used by: Render layer (trail boost compositing)

**UI Layer:**
- Purpose: Dear ImGui panels for parameter editing
- Location: `src/ui/`
- Contains: Panel draw functions, theme, modulatable sliders
- Depends on: Config layer, rlImGui
- Used by: Main loop

## Data Flow

**Audio to Visualization:**

1. Audio callback writes samples to ring buffer (audio thread)
2. AnalysisPipelineProcess reads ring buffer, feeds FFT, updates beat/bands
3. ModSourcesUpdate extracts normalized values from analysis results
4. ModEngineUpdate applies modulation routes to registered parameters
5. DrawableProcessWaveforms/Spectrum converts audio to vertex data
6. RenderPipelineExecute draws to accumTexture with effects

**Render Pipeline:**

1. Simulations update (physarum, curl flow, boids, etc.)
2. Feedback pass: zoom/rotate/translate accumTexture with decay
3. Drawables render to accumTexture (waveforms, shapes, spectrum)
4. Transform effects apply in user-defined order
5. Output pass: trail boost, chromatic aberration, FXAA, gamma

**State Management:**
- `AppContext` owns all runtime state (analysis, drawables, effects, LFOs)
- `EffectConfig` holds all effect parameters with in-class defaults
- `Preset` serializes configs to JSON via nlohmann/json
- Modulation engine stores base values; runtime values = base + offset

## Key Abstractions

**Drawable:**
- Purpose: Audio-reactive visual element (waveform, spectrum, shape)
- Examples: `src/render/drawable.h`, `src/config/drawable_config.h`
- Pattern: Tagged union (type + path + base + type-specific data)

**PostEffect:**
- Purpose: Central render state (shaders, textures, simulation pointers)
- Examples: `src/render/post_effect.h`
- Pattern: Resource aggregate with Init/Uninit lifecycle

**EffectConfig:**
- Purpose: Aggregate of all transform/simulation configs
- Examples: `src/config/effect_config.h`
- Pattern: Composition of config structs with TransformOrderConfig

**ModRoute:**
- Purpose: Maps modulation source to parameter with amount/curve
- Examples: `src/automation/modulation_engine.h`
- Pattern: Registry pattern (paramId string to float pointer)

## Entry Points

**main():**
- Location: `src/main.cpp`
- Triggers: Application launch
- Responsibilities: Window init, AppContext creation, 60fps loop, cleanup

**RenderPipelineExecute:**
- Location: `src/render/render_pipeline.cpp`
- Triggers: Every frame from main loop
- Responsibilities: Orchestrates simulation, feedback, drawable, and output stages

**AnalysisPipelineProcess:**
- Location: `src/analysis/analysis_pipeline.cpp`
- Triggers: Every frame from main loop
- Responsibilities: Reads audio, updates FFT/beat/bands/features

## Thread Model

**Main Thread:**
- Responsibilities: Render loop, ImGui, analysis processing, modulation updates
- Synchronization: Single-threaded; no locks required

**Audio Callback Thread:**
- Responsibilities: Receives audio from WASAPI, writes to ring buffer
- Synchronization: miniaudio lock-free ring buffer (`ma_pcm_rb`)

## Error Handling

**Strategy:** Fail-fast with NULL checks and graceful degradation

**Patterns:**
- Init functions return NULL on failure; callers check and cleanup
- INIT_OR_FAIL/CHECK_OR_FAIL macros for cascading init in AppContextInit
- Compute shader features degrade gracefully when OpenGL 4.3 unavailable
- Preset load failures preserve current state (no partial loads)

---

*Run `/sync-docs` to regenerate.*
