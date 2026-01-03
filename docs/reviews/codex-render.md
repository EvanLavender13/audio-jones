# Rendering and Effects Performance Review

Ordered by expected performance gain (highest first).

1. Downscale the feedback/effect chain and trail maps (half or quarter res, then upsample). Full-screen passes dominate GPU time in `src/render/render_pipeline.cpp`, and trail diffusion compute runs full resolution in `src/simulation/trail_map.cpp`. Smaller render targets reduce fill-rate and bandwidth every frame.
2. Skip or merge full-screen passes when parameters are neutral. `RenderPipelineApplyOutput` always runs chromatic, FXAA, and gamma passes even when `chromaticOffset == 0` or `gamma == 1.0` in `src/render/render_pipeline.cpp`. Add toggles or fuse passes into a single shader to remove 2-3 fullscreen passes.
3. Gate blur H pass when blur is effectively off. `RenderPipelineApplyFeedback` always runs blur H then blur V in `src/render/render_pipeline.cpp`; when `blurScale < 0.01`, skip blur H and keep only blur V (decay) to save one full-screen pass.
4. Throttle simulation updates or scale agent counts dynamically. Physarum, curl flow, and attractor compute shaders plus trail diffusion run every frame in `src/simulation/physarum.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/attractor_flow.cpp`, and `src/simulation/trail_map.cpp`. Updating every N frames or adapting agentCount to frame time can be a major win when simulations are enabled.
5. Avoid redundant drawable rendering into multiple trail maps. Each enabled simulation re-renders drawables via `RenderPipelineDrawablesToPhysarum/CurlFlow/Attractor` in `src/render/render_pipeline.cpp`. If multiple sims are on, render drawables once into a shared texture and feed that into each trail map (or use MRT) to cut draw calls.
6. Batch waveform rendering to reduce draw calls. `DrawWaveformLinear` and `DrawWaveformCircular` call `DrawLineEx` and `DrawCircleV` per segment in `src/render/waveform.cpp` (1024 segments per waveform, multiple passes). Build a vertex buffer or single `rlBegin(RL_TRIANGLES)` batch per waveform to reduce CPU/driver overhead.
7. Reduce trig/interpolation work per frame. `DrawWaveformCircular` does cubic interpolation and per-point sin/cos even when `INTERPOLATION_MULT == 1` in `src/render/waveform.cpp`. Use direct samples when interpolation is off and precompute base sin/cos tables, then rotate with a single sin/cos per frame.
8. Skip the output texture blit when unused. `RenderPipelineApplyOutput` always blits to `pe->outputTexture` in `src/render/render_pipeline.cpp`, but it is only needed for textured shapes in `src/render/shape.cpp`. Track whether any drawable uses `shape.textured` and skip the blit when none do.
9. Use lower-precision render targets where acceptable. The feedback chain and trail maps allocate RGBA32F textures in `src/render/render_utils.cpp` and `src/simulation/trail_map.cpp`. Switching ping-pong/output or trail maps to RGBA16F cuts bandwidth and memory pressure with minimal visual loss in many cases.
10. Skip FFT texture updates when no effect uses it. `RenderPipelineApplyFeedback` updates the FFT texture every frame in `src/render/render_pipeline.cpp`, but only physarum samples it in `src/simulation/physarum.cpp`. Avoid the CPU pass and texture upload when physarum is disabled.

## Pre/Post Feedback + Waveforms (Linear Scaling Hotspots)

Observed: `RenderPipelineDrawablesPreFeedback` and `RenderPipelineDrawablesPostFeedback` in `src/render/render_pipeline.cpp` both call `RenderPipelineDrawablesWithPhase`, which iterates all drawables in `DrawableRenderAll` (`src/render/drawable.cpp`) and then renders every waveform segment in `src/render/waveform.cpp`. Each waveform uses per-segment `DrawLineEx` + `DrawCircleV`, so cost scales with `numWaveforms * segments * passes`, plus any extra trail-map passes.

Ordered by expected performance gain (highest first):

1. Cache pre/post waveform layers at the visual update rate. Waveform data updates only in `UpdateVisuals` at 20 Hz (`src/main.cpp`), but rendering runs every frame. Render waveforms into two render textures on update or config change, then blit in pre/post passes. This removes per-frame waveform draw work while keeping the same animation cadence.
2. Build pre/post drawable lists and skip empty passes. Precompute arrays of drawables with `feedbackPhase` > 0 and < 1, update only when drawables change. Avoid scanning all drawables in both passes and skip the entire pre/post pass when list is empty.
3. Render drawables once for trail maps when multiple sims are enabled. `RenderPipelineDrawablesToPhysarum/CurlFlow/Attractor` each re-render all drawables. Cache a full-opacity drawable texture per frame and feed it to each trail map to avoid repeated waveform draws.
4. Batch waveform geometry into a single draw call per waveform. Replace per-segment `DrawLineEx` + `DrawCircleV` with a single `rlBegin(RL_TRIANGLES)` or VBO per waveform. This collapses thousands of draw calls per waveform into one.
5. Precompute per-segment color ramps. `ColorFromConfig` is called per segment in `DrawWaveformLinear`/`DrawWaveformCircular`. Build a per-waveform color LUT when the color config changes and reuse it each frame.
6. Skip interpolation and trig when not needed. With `INTERPOLATION_MULT == 1`, avoid `CubicInterp` in `DrawWaveformCircular` and use direct samples; precompute base sin/cos tables and apply a single rotation per frame.
