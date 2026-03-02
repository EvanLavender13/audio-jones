# Loading Screen

A progress bar with category-level status text rendered during application startup, replacing the blank white window that currently shows while ~128 shaders compile and GPU resources allocate (~1-3 seconds).

## Classification

- **Category**: General (software/architecture)

## References

- [OpenGL Loading Screen - GameDev.net](https://gamedev.net/forums/topic/352902-opengl-loading-screen/3311289/) - Draw-load-draw interleaving pattern
- [How to display a loading bar - Khronos Forums](https://community.khronos.org/t/how-to-display-a-loading-bar-2021/107587) - Chunked loading with per-frame progress rendering
- [Unvanquished shader progress bar](https://github.com/Unvanquished/Unvanquished/issues/217) - Real-world example of shader compilation progress tracking

## Algorithm

### Core Pattern: Draw-Load-Draw Interleaving

The approach is single-threaded and synchronous. Between each init phase, render one frame showing the updated progress bar. raylib's `BeginDrawing`/`EndDrawing` flush the GPU and swap buffers, so the user sees each progress update immediately.

```
InitWindow(...)
SetClearColor(BLACK)

for each phase in init_phases:
    run phase.init()
    render_loading_frame(phase.progress, phase.label)

enter main loop
```

### Restructuring Init into Phases

Current init is two monolithic calls: `AppContextInit()` calls `PostEffectInit()` which does everything. This must be split into discrete phases that yield to the loading screen.

**Proposed phases** (with approximate weight for progress bar):

| Phase | Weight | Status Text | What Happens |
|-------|--------|-------------|--------------|
| 1. Window & ImGui | 5% | "Initializing window..." | Already done before loading starts (sets black background) |
| 2. Core pipeline shaders | 10% | "Loading core shaders..." | 7 fragment shaders (feedback, blur, fxaa, clarity, gamma, shape_texture) + uniform caching |
| 3. Render textures | 5% | "Allocating render targets..." | 4 full-res HDR RTs, 2 half-res, generator scratch, FFT/waveform textures |
| 4. Simulations | 25% | "Initializing simulations..." | 7 simulations (physarum, curl_flow, curl_advection, attractor_flow, particle_life, boids, cymatics) — each loads compute shaders + trail maps + SSBOs |
| 5. Blend compositor | 2% | "Loading compositor..." | 1 shader |
| 6. Transform effects | 40% | "Compiling effects..." | ~91 effect shaders via EFFECT_DESCRIPTORS[] loop |
| 7. Audio capture | 8% | "Starting audio..." | WASAPI loopback device init |
| 8. Analysis & modulation | 5% | "Initializing analysis..." | FFT processor, modulation engine, param registration, LFOs |

Phase 6 (transform effects) is the largest. For smoother progress, this phase should update the bar after every N effects rather than all at once. Updating after each individual shader would flicker too fast; updating after every ~10 effects gives ~9 visible steps within this phase.

### Loading Screen Renderer

A standalone function using only raylib primitives (no shader/ImGui dependencies):

```
void DrawLoadingFrame(float progress, const char* statusText)
    BeginDrawing()
    ClearBackground(BLACK)

    // Progress bar: centered, bottom third of screen
    // Bar background (dark gray outline)
    // Bar fill (accent color, width = totalWidth * progress)

    // Status text: centered below bar, white, default raylib font
    // "AudioJones" title text: centered above bar, larger size

    EndDrawing()
```

Uses `DrawRectangle`, `DrawRectangleLines`, `DrawText`, `MeasureText` — all available immediately after `InitWindow`.

### Architectural Changes

**New files:**
- `src/ui/loading_screen.h` — `DrawLoadingFrame(float progress, const char* statusText)` declaration
- `src/ui/loading_screen.cpp` — Implementation using raylib primitives

**Modified files:**
- `src/main.cpp` — Replace single `AppContextInit()` call with phased init loop that calls `DrawLoadingFrame` between phases
- `src/render/post_effect.cpp` — Split `PostEffectInit()` into sub-functions for each phase, or add a callback mechanism that the caller can use to render progress between stages
- `src/render/post_effect.h` — Expose phased init functions or progress callback type

**Two approaches for splitting PostEffectInit:**

1. **Explicit sub-functions**: Break `PostEffectInit` into `PostEffectInitCoreShaders()`, `PostEffectInitRenderTextures()`, `PostEffectInitSimulations()`, `PostEffectInitEffects()`, etc. Main calls each one and renders a loading frame between them. Simple but adds many public functions.

2. **Progress callback**: `PostEffectInit` takes a callback `void (*onProgress)(float progress, const char* status, void* userData)`. It calls this callback between phases internally. Main passes a callback that calls `DrawLoadingFrame`. Keeps init logic encapsulated but adds indirection.

**Recommended: Explicit sub-functions.** Simpler, no function pointer indirection, and the phases map cleanly to logical groups that are already somewhat separated in the current code. The main.cpp init sequence becomes self-documenting.

### Progress Bar Visuals

- **Background**: Solid black (`ClearBackground(BLACK)`)
- **Title**: "AudioJones" in white, centered horizontally, positioned above the bar
- **Bar track**: Dark gray rectangle (`{40, 40, 40}`) — full width of bar area
- **Bar fill**: Accent color (neon cyan/teal matching the ImGui theme) — width scales with progress
- **Bar dimensions**: ~60% of window width, ~8px tall, vertically centered or slightly below center
- **Status text**: White, smaller font size, centered below the bar
- **No percentage number** — the bar itself communicates progress

## Notes

- The white flash comes from raylib's default clear color being white. Setting `ClearBackground(BLACK)` in the first frame (before any init) eliminates it regardless of loading screen implementation.
- Shader compilation time varies by GPU driver. NVIDIA tends to be faster than AMD/Intel. First run is slower due to empty shader cache.
- The loading screen itself uses zero shaders — pure CPU-side raylib primitives — so it's available immediately.
- Phase weights are approximate. The progress bar doesn't need to be perfectly linear — it just needs to move visibly and not stall for long periods.
- If any init phase fails, the existing error handling (TraceLog + early return) still applies. The loading screen doesn't change failure behavior.
