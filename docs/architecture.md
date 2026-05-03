# Architecture

> Last sync: 2026-05-03 | Commit: 633041d0

## Pattern Overview

**Overall:** Layered Pipeline Architecture

**Key Characteristics:**
- Single-threaded main loop with background audio callback
- Frame-based render pipeline: capture, analyze, modulate, draw, post-process
- Module isolation via Init/Uninit lifecycle pairs and opaque pointers
- Configuration-driven effects with hot-swappable presets
- Self-contained effect modules encapsulate shader loading, uniform binding, param registration, and UI drawing
- Descriptor-driven dispatch: effect metadata, lifecycle, GPU state slots, and UI callbacks registered via macros into a central table

## Layers

**Audio Capture Layer:**
- Purpose: Captures system audio output via WASAPI loopback
- Location: `src/audio/`
- Contains: miniaudio device initialization (`audio.cpp`), ring buffer transfer, sample reading, audio device config (`audio_config.h`)
- Depends on: miniaudio library
- Used by: Analysis layer

**Analysis Layer:**
- Purpose: Transforms raw PCM samples into frequency bands, beat triggers, and spectral features
- Location: `src/analysis/`
- Contains: FFT processor (`fft.cpp`), beat detector (`beat.cpp`), band energy calculator (`bands.cpp`), audio feature extractor (`audio_features.cpp`), waveform history ring buffer, smoothing helpers (`smoothing.h`), aggregation pipeline (`analysis_pipeline.cpp`)
- Depends on: Audio capture layer
- Used by: Modulation layer, Drawable layer, Render layer (waveform/FFT GPU upload)

**Automation Layer:**
- Purpose: Routes modulation sources (LFOs, audio bands, beat, mod buses) to effect parameters
- Location: `src/automation/`
- Contains: LFO generators (`lfo.cpp`), mod bus processor (`mod_bus.cpp`), modulation engine (`modulation_engine.cpp`), parameter registry (`param_registry.cpp`), mod source aggregation (`mod_sources.cpp`), drawable param helpers (`drawable_params.cpp`), easing curves (`easing.h`)
- Depends on: Analysis layer (for audio-reactive sources)
- Used by: Render layer (parameters modulated before draw)

**Configuration Layer:**
- Purpose: Defines all effect parameters and serializes presets to JSON
- Location: `src/config/`
- Contains: `EffectConfig` master struct (`effect_config.h`), effect descriptor table (`effect_descriptor.h` + `effect_descriptor.cpp`), effect serialization (`effect_serialization.cpp`), preset I/O (`preset.cpp`), playlist sequencing (`playlist.cpp`), shared embeddable configs (`dual_lissajous_config.h`, `feedback_flow_config.h`, `procedural_warp_config.h`, `random_walk_config.h`, `band_config.h`, `lfo_config.h`, `mod_bus_config.h`, `modulation_config.h`), constants (`constants.h`), runtime state pointers aggregator (`app_configs.h`), drawable config (`drawable_config.h`)
- Depends on: Effects layer (imports config structs from effect headers)
- Used by: All layers

**Effects Layer:**
- Purpose: Self-contained effect modules with shader lifecycle, uniform binding, and colocated UI drawing
- Location: `src/effects/`
- Contains: Effect modules as paired `<name>.cpp` + `<name>.h` files, each encapsulating config struct, effect struct, Init/Setup/Uninit/Resize/RegisterParams functions, and UI draw callbacks. Effect counts and inventory derive from naming convention; see `docs/effects.md` for the catalog.
- Depends on: raylib (shader API), automation layer (param registration), Dear ImGui (colocated UI), shared config structs from `src/config/` and `src/render/`
- Used by: Configuration layer (config structs aggregated into `EffectConfig`), Render layer (effect state instances accessed via `pe->effectStates[]`), UI layer (draw callbacks invoked via descriptor dispatch)

**Render Layer:**
- Purpose: Orchestrates frame rendering, feedback processing, and multi-pass post-processing
- Location: `src/render/`
- Contains: Render pipeline (`render_pipeline.cpp`), `PostEffect` coordinator (`post_effect.cpp`), shader setup dispatchers (`shader_setup.cpp`), drawable rendering (`drawable.cpp`, `waveform.cpp`, `spectrum_bars.cpp`, `shape.cpp`, `thick_line.cpp`), blend compositing (`blend_compositor.cpp`), color LUT and gradient helpers (`color_lut.cpp`, `color_config.cpp`, `gradient.cpp`), noise texture (`noise_texture.cpp`), profiler (`profiler.cpp`), render utilities (`render_utils.cpp`, `draw_utils.cpp`), render context struct (`render_context.h`), blend modes (`blend_mode.h`)
- Depends on: Effects layer (owns effect struct instances via descriptor `state` pointers), Configuration layer, Simulation layer, raylib
- Used by: Main loop

**Simulation Layer:**
- Purpose: GPU compute shader agent simulations that generate visual trails
- Location: `src/simulation/`
- Contains: Physarum slime mold (`physarum.cpp`), boids flocking (`boids.cpp`), curl flow (`curl_flow.cpp`), particle life (`particle_life.cpp`), attractor flow (`attractor_flow.cpp`), maze worms (`maze_worms.cpp`), shared trail map (`trail_map.cpp`), spatial hash (`spatial_hash.cpp`), shader utilities (`shader_utils.cpp`), bounds modes (`bounds_mode.h`)
- Depends on: Render layer (accumulation texture), OpenGL 4.3+
- Used by: Render layer (trail compositing)

**UI Layer:**
- Purpose: ImGui control panels, descriptor-driven effect dispatch, and shared widgets
- Location: `src/ui/`
- Contains: Effects panel (`imgui_effects.cpp`), descriptor-driven category dispatch (`imgui_effects_dispatch.cpp` + `.h`), playlist panel (`imgui_playlist.cpp`), preset panel (`imgui_presets.cpp`), drawables panel (`imgui_drawables.cpp`), audio panel (`imgui_audio.cpp`), analysis panel (`imgui_analysis.cpp`), LFO panel (`imgui_lfo.cpp`), mod bus panel (`imgui_bus.cpp`), shared panel utilities (`imgui_panels.cpp`), modulatable sliders (`modulatable_slider.cpp`, `modulatable_drawable_slider.cpp`), gradient editor (`gradient_editor.cpp`), drawable type controls (`drawable_type_controls.cpp`), widget primitives (`imgui_widgets.cpp`), unit-aware helpers (`ui_units.h`), theme (`theme.h`), loading screen (`loading_screen.cpp`)
- Depends on: Dear ImGui, rlImGui, Configuration layer, Effects layer (via descriptor `drawParams`/`drawOutput` callbacks)
- Used by: Main loop

## Data Flow

**Audio-to-Visual Pipeline:**

1. miniaudio callback writes PCM frames to ring buffer (background thread)
2. `AnalysisPipelineProcess` reads ring buffer, computes FFT and beat detection (every frame)
3. `AnalysisPipelineUpdateWaveformHistory` updates waveform ring buffer for cymatics (every frame)
4. `LFOProcess` advances each LFO state and writes outputs
5. `ModSourcesUpdate` aggregates band energies, beat, audio features, and LFO outputs into normalized values
6. `ModBusEvaluate` processes 8 mod buses (combiners, envelope followers, slew limiters) and writes outputs back to mod sources
7. `ModEngineUpdate` applies modulation routes to registered parameters
8. `DrawableTickRotations` accumulates per-drawable rotation phases
9. `RenderPipelineExecute` draws frame: waveform/FFT upload -> simulations -> feedback -> blit -> drawables -> output chain

**Effect Module Lifecycle:**

1. `PostEffectInit` populates `pe->effectStates[]` from `EFFECT_DESCRIPTORS[i].state` (file-local static effect instances declared by `REGISTER_*` macros), then iterates the table calling each descriptor's `init` function pointer to load shaders and cache uniform locations
2. `PostEffectRegisterParams` calls each descriptor's `registerParams` to expose parameters to modulation
3. Per frame, each descriptor's `setup` function pointer binds current config values to shader uniforms; generators additionally use `scratchSetup` to populate the scratch render texture
4. `PostEffectResize` calls each descriptor's `resize` for effects with `EFFECT_FLAG_NEEDS_RESIZE`
5. `PostEffectUninit` calls each descriptor's `uninit` to release GPU resources

**UI Dispatch Flow:**

1. `ImGuiDrawEffectsPanel` (`src/ui/imgui_effects.cpp`) draws feedback/output sections directly, then calls `DrawEffectCategory()` for each category section index (0-16)
2. `DrawEffectCategory` (`src/ui/imgui_effects_dispatch.cpp`) iterates `EFFECT_DESCRIPTORS[]` filtered by `categorySectionIndex`
3. For each matching effect, it draws the section header/toggle and calls the descriptor's `drawParams` and `drawOutput` function pointers
4. Callbacks are defined as `static` functions inside each effect's `.cpp` file, registered via `REGISTER_EFFECT`, `REGISTER_EFFECT_CFG`, `REGISTER_GENERATOR`, `REGISTER_GENERATOR_FULL`, or `REGISTER_SIM_BOOST` macros

**Render Pipeline Stages (`RenderPipelineExecute`):**

1. Upload FFT magnitude texture and waveform history texture for shader consumption
2. Run GPU simulations (physarum, curl flow, attractor flow, particle life, boids, maze worms)
3. Apply feedback effects (flow field warp, blur, decay) to accumulation texture
4. Blit feedback result to output texture for textured shape sampling
5. Draw all drawables (waveforms, spectra, shapes) to accumulation texture
6. Output chain: transforms (user-ordered, includes generators and sim boosts) -> clarity -> FXAA -> gamma -> screen

**State Management:**
- `AppContext` (`src/main.cpp`) holds all runtime state (analysis, drawables, effects, LFOs, mod buses, profiler)
- `EffectConfig` struct aggregates all per-effect config structs from `src/effects/` headers
- `PostEffect` struct owns the descriptor-driven effect state slots `void *effectStates[TRANSFORM_EFFECT_COUNT]`, simulation pointers, shared render textures, framework shaders (feedback/blur/FXAA/clarity/gamma/shape-texture), FFT/waveform GPU textures, half-res buffers, and the blend compositor. Per-effect uniform locations live inside each effect's own `<Name>Effect` struct, not on `PostEffect`
- `Preset` (`src/config/preset.h`) serializes/deserializes full application state to JSON
- `AppConfigs` (`src/config/app_configs.h`) aggregates pointers to all config slices (drawables, effects, audio, LFOs, mod buses) for preset I/O and UI panels
- `Playlist` (`src/config/playlist.h`) holds an ordered sequence of preset paths with an active index; keyboard shortcuts (Left/Right arrows) advance through the sequence, loading each preset into `AppConfigs`
- Ring buffer synchronizes audio callback with main thread

## Key Abstractions

**Drawable:**
- Purpose: Audio-reactive visual element (waveform, spectrum, shape, parametric trail)
- Examples: `src/config/drawable_config.h`, `src/render/drawable.cpp`
- Pattern: Tagged union with type-specific data structs

**Effect Module:**
- Purpose: Encapsulates one post-processing effect's shader, uniforms, config, and UI
- Examples: `src/effects/bloom.h` + `src/effects/bloom.cpp`, `src/effects/kaleidoscope.h` + `src/effects/kaleidoscope.cpp`
- Pattern: Paired `*Config` struct (parameters) and `*Effect` struct (GPU state) with Init/Setup/Uninit/RegisterParams functions plus colocated `DrawParams`/`DrawOutput` UI callbacks. Config structs define user-facing parameters. Effect structs store loaded shaders, cached uniform locations, and animation accumulators. Setup functions bind current config values to shader uniforms. UI callbacks draw ImGui sliders and controls. The registration macro at the bottom of each `.cpp` file declares a file-local static `<Name>Effect` instance and pushes a row into `EFFECT_DESCRIPTORS[]`.

**PostEffect:**
- Purpose: Coordinates all effect modules, manages shared render textures and framework shaders, and owns simulation pointers
- Examples: `src/render/post_effect.h`, `src/render/post_effect.cpp`
- Pattern: Coordinator struct holding (1) framework shaders and their uniform locations (feedback, blur, FXAA, clarity, gamma, shape-texture sampling) inline as named fields; (2) `void *effectStates[TRANSFORM_EFFECT_COUNT]` filled at init time from each descriptor's `state` pointer (file-local static `<Name>Effect` instance declared by the registration macro); (3) simulation pointers, shared ping-pong/half-res render textures, generator scratch texture, FFT/waveform GPU textures, and the blend compositor. Lifecycle calls (init/uninit/resize/registerParams) iterate `EFFECT_DESCRIPTORS[]` and dispatch through function pointers.

**EffectDescriptor:**
- Purpose: Central table mapping transform enum values to metadata, lifecycle function pointers, GPU state, and UI callbacks
- Examples: `src/config/effect_descriptor.h` (`EFFECT_DESCRIPTORS[]`), `src/config/effect_descriptor.cpp`
- Pattern: Each descriptor row contains: `type` enum, `name` (display), `categoryBadge` (UI grouping), `categorySectionIndex` (ordering), `enabledOffset` (field pointer in `EffectConfig`), `paramPrefix` (dot-terminated, e.g. `"bloom."`, used for route cleanup; `nullptr` when no params), `flags` bitmask (`EFFECT_FLAG_BLEND`, `EFFECT_FLAG_HALF_RES`, `EFFECT_FLAG_SIM_BOOST`, `EFFECT_FLAG_NEEDS_RESIZE`), function pointers for `init`/`uninit`/`resize`/`registerParams`/`getShader`/`setup`, optional `getScratchShader`/`scratchSetup` for generators, optional `render` callback for custom render paths, UI callbacks (`drawParams`, `drawOutput`), and a `state` pointer to the file-local `<Name>Effect` instance. Self-registration macros (`REGISTER_EFFECT`, `REGISTER_EFFECT_CFG`, `REGISTER_GENERATOR`, `REGISTER_GENERATOR_FULL`, `REGISTER_SIM_BOOST`) at the bottom of each effect `.cpp` file populate the table at static-init time. The dispatch system (`src/ui/imgui_effects_dispatch.cpp`) iterates the table to render UI without per-category source files.

**ModRoute:**
- Purpose: Maps a modulation source to a parameter with amount and easing curve
- Examples: `src/automation/modulation_engine.h`, `src/config/modulation_config.h`
- Pattern: String-keyed routing table with pointer-based parameter access. Easing curves (linear, ease-in/out, spring, elastic, bounce) shape modulation response.

**ModBus:**
- Purpose: Combines, shapes, or envelopes one or two mod sources into a derived signal
- Examples: `src/automation/mod_bus.h`, `src/config/mod_bus_config.h`
- Pattern: 8 buses, each with configurable operator (add, multiply, min, max, gate, crossfade, difference, envelope follow, envelope trigger, exponential slew, linear slew). Single-input operators (envelope followers, slew limiters) ignore inputB. Bus outputs are written back to `ModSources` so they can drive modulation routes or feed other buses.

**TrailMap:**
- Purpose: Shared trail texture with diffusion/decay for agent simulations
- Examples: `src/simulation/trail_map.h`, `src/simulation/trail_map.cpp`
- Pattern: GPU texture with compute shader processing

**BlendCompositor:**
- Purpose: Renders generator effects into a scratch texture and composites onto the main chain
- Examples: `src/render/blend_compositor.h`, `src/render/blend_compositor.cpp`
- Pattern: Shared shader that blends generator output with configurable opacity and blend mode

## Entry Points

**Application Entry:**
- Location: `src/main.cpp`
- Triggers: Program startup
- Responsibilities: Window init, ImGui/font setup, AppContext creation, main loop, cleanup

**Frame Loop:**
- Location: `src/main.cpp` (`while (!WindowShouldClose())`)
- Triggers: Every frame at 60 FPS target
- Responsibilities: Window resize handling, audio analysis (every frame), waveform history update, LFO processing, mod source aggregation, mod bus evaluation, modulation update, drawable rotation tick, visual update (20 Hz), render pipeline execution, playlist keyboard navigation, UI draw

**Preset Load:**
- Location: `src/config/preset.cpp`
- Triggers: User selects preset file or playlist advances
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
- `PostEffectInit` `goto cleanup` pattern delegates teardown to `PostEffectUninit`
- TraceLog reports initialization failures; per-frame errors silently ignored

---

*Run `/sync-docs` to regenerate.*
