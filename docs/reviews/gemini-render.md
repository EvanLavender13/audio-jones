# Rendering and Effect Subsystem Review

## Overview

This document outlines the performance review of the rendering and effect subsystem. The current architecture relies heavily on a multi-pass post-processing pipeline using ping-pong buffering. While flexible, this approach incurs significant overhead due to repeated draw calls, texture binds, and memory bandwidth usage.

## Identified Bottlenecks

### Post-Processing & Simulation
1.  **Excessive Draw Calls in Output Stage:** The `RenderPipelineApplyOutput` function performs a linear sequence of draw calls for effects that could be mathematically combined. Currently, it executes:
    -   Trail Boost (up to 3 passes)
    -   Kaleidoscope
    -   Infinite Zoom
    -   Blit (Copy)
    -   Chromatic Aberration
    -   Clarity
    -   FXAA
    -   Gamma Correction
    
    Each of the last 3-4 stages reads the full screen texture and writes it back, which is bandwidth-heavy.

2.  **Unnecessary Render Passes:** The feedback loop (`RenderPipelineApplyFeedback`) executes BlurH and BlurV passes even if `blurScale` is effectively zero, wasting GPU time on identity transformations.

3.  **Redundant Blit Operation:** An explicit `BlitTexture` call exists in `RenderPipelineApplyOutput` to "save" the texture state before chromatic aberration. This copy operation consumes bandwidth and might be avoidable with better pipeline management.

4.  **Separate Trail Boost Passes:** The trails for Physarum, Curl Flow, and Attractor Flow are composited onto the main image in three separate draw calls using the `BlendCompositor`.

5.  **Compute Shader Synchronization:** The `TrailMapProcess` uses two compute shader dispatches (Horizontal and Vertical) with a `glMemoryBarrier` in between. While correct for separable Gaussian blur, this synchronization point can stall the pipeline.

### Waveforms & Drawables (Pre/Post Feedback)
6.  **Inefficient Waveform Rendering (Linear Scaling):**
    -   **Problem:** `DrawWaveformLinear` and `DrawWaveformCircular` use immediate mode rendering (Raylib's `DrawLineEx` and `DrawCircleV`) inside loops.
    -   **Impact:** For a standard 1024-sample waveform, this generates **>2000 individual draw calls per waveform** (one for the line segment, one for the joint circle).
    -   **Scaling:** With multiple waveforms enabled, or when drawn multiple times (Pre-Feedback, Post-Feedback, Reflection), the number of draw calls explodes (e.g., 5 active waveforms = >10,000 draw calls per frame). This linear scaling is the primary cause of CPU-side render bottlenecks.
    -   **Interpolation Overhead:** `DrawWaveformCircular` performs cubic interpolation on the CPU for every point, which is computationally expensive and redundant when `INTERPOLATION_MULT` is 1.

## Prioritized Optimizations

### 1. Batch Waveform Geometry (Critical Impact)
**Goal:** Reduce waveform draw calls from >2000 to 1 per waveform.
**Description:** Rewrite `DrawWaveformLinear` and `DrawWaveformCircular` to use `rlgl` (OpenGL) directly.
-   Use `RL_TRIANGLE_STRIP` to construct the thick line geometry in a single batch.
-   Calculate vertex normals on the fly to handle line thickness and smooth joins.
-   Eliminate CPU-side cubic interpolation in favor of raw sample data (or move smoothing to the pre-process stage).
**Benefit:** Massive reduction in CPU overhead and driver validation time. This is the single most effective optimization for the "linear scaling" issue.

### 2. Implement "Uber-Shader" for Final Post-Processing (High Impact)
**Goal:** Reduce 3-4 draw calls into 1.
**Description:** Combine `Chromatic`, `Clarity`, `Gamma`, and potentially `Vignette` (if added) into a single `FinalPass` shader. These operations are per-pixel color transformations that do not require neighboring pixel access (except Chromatic, which is a small offset).
**Benefit:** drastic reduction in memory bandwidth and draw call overhead.

### 3. Pass Culling (Medium Impact)
**Goal:** Eliminate wasted work.
**Description:** Modify `RenderPipelineApplyFeedback` to check if `pe->effects.blurScale` is near zero. If so, skip the `BlurH` and `BlurV` passes. Apply similar logic to `Voronoi` and other optional effects.
**Benefit:** Immediate performance gain when effects are disabled or subtle.

### 4. Consolidate Trail Composition (Medium Impact)
**Goal:** Reduce 2 draw calls.
**Description:** Create a specific shader or update `BlendCompositor` to accept up to 3 input textures (Trail Maps) and their intensities/blend modes simultaneously. This allows compositing all active simulation trails in a single pass.
**Benefit:** Reduced draw calls and texture state changes.

### 5. Remove Explicit Blit (Low/Medium Impact)
**Goal:** Save one full-screen copy.
**Description:** The `BlitTexture` call helps separate the "pre-distortion" state. By rearranging the pipeline or using the `pingPong` buffers more effectively (e.g., reading from the last valid output directly), this copy can likely be removed.
**Benefit:** Reduced VRAM bandwidth.

## Recommended Action Plan

1.  **Phase 1 (Critical):** Implement **Waveform Batching**. This addresses the immediate regression in performance when adding multiple waveforms.
2.  **Phase 2:** Implement **Pass Culling** immediately as it requires no shader changes, only logic updates in `render_pipeline.cpp`.
3.  **Phase 3:** Create the **Uber-Shader** (`final_composite.fs`) and update `RenderPipelineApplyOutput` to use it.
4.  **Phase 4:** Refactor `RenderPipelineApplyOutput` to remove the explicit `BlitTexture`.
5.  **Phase 5:** Optimize `TrailBoost` blending if profiling shows it remains a bottleneck after the above changes.