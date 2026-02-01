# Architecture

> Last sync: 2026-02-01 | Commit: 996fbfd

## Pattern Overview

**Overall:** Layered pipeline architecture with immediate-mode UI

**Key Characteristics:**
- Single-threaded main loop with audio callback on separate thread
- Ping-pong render texture chain for multi-pass post-processing
- Static module initialization via Init/Uninit function pairs
- Configuration structs drive all effect behavior (no inheritance)

## Layers

**Audio Capture (`src/audio/`):**
- Purpose: Captures system audio via WASAPI loopback
- Location: `src/audio/audio.cpp`
- Contains: Ring buffer, miniaudio device management
- Depends on: miniaudio library
- Used by: Analysis layer

**Analysis (`src/analysis/`):**
- Purpose: Transforms raw PCM into usable audio features
- Location: `src/analysis/analysis_pipeline.cpp`
- Contains: FFT processor, beat detector, band energies, spectral features
- Depends on: Audio capture layer
- Used by: Modulation engine, drawables, simulations

**Configuration (`src/config/`):**
- Purpose: Defines all effect parameters as plain C structs
- Location: `src/config/effect_config.h`, per-effect `*_config.h` headers
- Contains: 50+ effect config structs, preset serialization
- Depends on: None (data-only)
- Used by: All other layers

**Automation (`src/automation/`):**
- Purpose: Routes modulation sources (LFOs, audio bands) to effect parameters
- Location: `src/automation/modulation_engine.cpp`
- Contains: LFO generators, param registry, mod source aggregation
- Depends on: Analysis layer, configuration layer
- Used by: Main loop (updates each frame)

**Simulation (`src/simulation/`):**
- Purpose: GPU compute shader agent simulations
- Location: `src/simulation/physarum.cpp`, `boids.cpp`, `curl_flow.cpp`, `particle_life.cpp`
- Contains: Agent update via compute shaders, trail maps, spatial hashing
- Depends on: Configuration layer, render textures
- Used by: Render pipeline

**Render (`src/render/`):**
- Purpose: Orchestrates GPU rendering and post-processing
- Location: `src/render/render_pipeline.cpp`, `post_effect.cpp`
- Contains: Drawable rendering, shader uniform binding, ping-pong passes
- Depends on: All simulation and configuration layers
- Used by: Main loop

**UI (`src/ui/`):**
- Purpose: Immediate-mode interface panels via Dear ImGui
- Location: `src/ui/imgui_panels.cpp`, `imgui_effects.cpp`
- Contains: Effect panels, modulatable sliders, gradient editor
- Depends on: Configuration layer, modulation engine
- Used by: Main loop (when UI visible)

## Data Flow

**Audio to Visuals:**

1. Audio callback writes PCM to ring buffer (`audio.cpp:audio_data_callback`)
2. Main loop reads buffer into analysis pipeline (`analysis_pipeline.cpp:AnalysisPipelineProcess`)
3. FFT processor extracts frequency bins; beat detector identifies transients
4. Band energies and spectral features update modulation sources
5. Modulation engine applies routes to effect parameters
6. Drawables sample waveform/spectrum data for visualization

**Render Pipeline (`render_pipeline.cpp:RenderPipelineExecute`):**

1. Upload waveform texture for simulations
2. Run GPU simulations (physarum, boids, curl flow, cymatics, particle life)
3. Apply feedback effects (warp, blur, decay) via ping-pong buffers
4. Copy feedback result to output texture for shape sampling
5. Draw all drawables at configured opacity
6. Apply output transform chain (chromatic, kaleidoscope, bloom, etc.)
7. Final FXAA and gamma correction

**State Management:**
- `AppContext` struct (`main.cpp:24-41`) owns all runtime state
- `EffectConfig` struct (`effect_config.h:259-445`) stores all effect parameters
- `PostEffect` struct (`post_effect.h:18-617`) holds GPU resources and shader locations
- Presets serialize to JSON via nlohmann/json (`preset.cpp`)

## Key Abstractions

**Drawable:**
- Purpose: Represents a renderable audio visualization element
- Examples: `src/render/drawable.cpp`, `src/config/drawable_config.h`
- Pattern: Tagged union with type enum + data union (waveform, spectrum, shape, parametric trail)

**PostEffect:**
- Purpose: Manages all GPU render textures, shaders, and simulation pointers
- Examples: `src/render/post_effect.cpp`, `src/render/post_effect.h`
- Pattern: Monolithic struct with Init/Uninit lifecycle

**EffectConfig:**
- Purpose: Aggregates all 50+ effect configuration sub-structs
- Examples: `src/config/effect_config.h`
- Pattern: Flat composition of per-effect config structs

**ModRoute:**
- Purpose: Maps a modulation source to a target parameter with curve/amount
- Examples: `src/automation/modulation_engine.cpp`
- Pattern: String-keyed registry with base/offset separation

## Entry Points

**Application Entry (`main`):**
- Location: `src/main.cpp:146`
- Triggers: OS process start
- Responsibilities: Window creation, AppContext init, main loop, cleanup

**Frame Loop (`main.cpp:171-252`):**
- Location: `src/main.cpp:171`
- Triggers: Every frame (60 FPS target)
- Responsibilities: Audio analysis, modulation update, render pipeline, UI draw

**Preset Load (`PresetToAppConfigs`):**
- Location: `src/config/preset.cpp:866`
- Triggers: User loads preset from UI
- Responsibilities: Apply effect/drawable/modulation state, sync param registry

## Thread Model

**Main Thread:**
- Responsibilities: Render loop, UI, analysis pipeline, modulation engine
- Synchronization: Single-threaded; no locks required

**Audio Callback Thread:**
- Responsibilities: Writes PCM samples to ring buffer
- Synchronization: Lock-free ring buffer (`ma_pcm_rb` from miniaudio)

**GPU Compute (Async):**
- Responsibilities: Agent simulation updates (physarum, boids, etc.)
- Synchronization: Dispatch before render pass; implicit GPU barrier

## Error Handling

**Strategy:** Early return with cleanup macros

**Patterns:**
- `INIT_OR_FAIL(ptr, expr)` macro assigns and checks allocation; calls cleanup on NULL
- `CHECK_OR_FAIL(expr)` macro evaluates bool; calls cleanup on false
- Init functions return `bool` or pointer (NULL on failure)
- TraceLog for initialization failures; no per-frame logging in hot paths

---

*Run `/sync-docs` to regenerate.*
