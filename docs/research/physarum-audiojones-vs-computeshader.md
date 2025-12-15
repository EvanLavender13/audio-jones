# Physarum Implementation Comparison: AudioJones vs ComputeShader

## Scope

This document compares the physarum/slime simulation implementations in:

- AudioJones (this repository)
- ComputeShader (`/mnt/c/Users/EvanUhhh/source/repos/ComputeShader`)

This document focuses on agent state, sensing/steering, trail deposition, diffusion/decay, and render integration.
See `docs/research/physarum-simulation.md:1` for background on the Jeff Jones-style algorithm.

## Primary Sources

AudioJones sources:

- `src/render/physarum.h:8`
- `src/render/physarum.cpp:150`
- `shaders/physarum_agents.glsl:131`
- `shaders/blur_h.fs:15`
- `shaders/blur_v.fs:17`
- `src/render/post_effect.cpp:254`

ComputeShader sources:

- `ComputeShader/app/src/main/resources/compute.glsl:86`
- `ComputeShader/app/src/main/java/computeshader/App.java:258`
- `ComputeShader/app/src/main/java/computeshader/slime/AgentUtil.java:50`
- `ComputeShader/images/README.md:1`

## Executive Summary (Observed and Inferred)

- AudioJones updates agents in a compute shader and applies diffusion/decay in fullscreen fragment passes (`shaders/physarum_agents.glsl:131`, `shaders/blur_v.fs:38`).
- ComputeShader performs both agent updates and diffusion/decay inside a two-stage compute shader (`ComputeShader/app/src/main/resources/compute.glsl:93`, `ComputeShader/app/src/main/resources/compute.glsl:201`).
- AudioJones uses hue-based attraction and repulsion with circular hue distance and intensity gating (`shaders/physarum_agents.glsl:109`, `shaders/physarum_agents.glsl:118`).
- ComputeShader steers using absolute hue distance without circular wrap handling or intensity gating (`ComputeShader/app/src/main/resources/compute.glsl:127`, `ComputeShader/app/src/main/resources/compute.glsl:132`).
- AudioJones deposits trails using a saturating blend toward the agent’s hue (`shaders/physarum_agents.glsl:188`).
- ComputeShader deposits trails via additive accumulation into an HDR trail map (`ComputeShader/app/src/main/resources/compute.glsl:193`).
- ComputeShader ships screenshots that show dense worm textures and high-contrast networks (`ComputeShader/images/README.md:1`).
- This repository does not include screenshots of the AudioJones physarum output (TBD).

## Implementation Overview

### AudioJones (Render-Integrated Physarum Pass)

AudioJones stores agents in an SSBO with four floats per agent (x, y, heading, hue) (`src/render/physarum.h:8`).
AudioJones updates agents and deposits directly into the main accumulation render texture (`src/render/physarum.cpp:150`).
AudioJones then diffuses and decays the accumulation texture using separable blur and exponential decay (`shaders/blur_h.fs:25`, `shaders/blur_v.fs:38`).

AudioJones runs the pass early in the post-effect pipeline, before voronoi/feedback/blur (`src/render/post_effect.cpp:269`).
This ordering makes the physarum output part of the feedback and blur history that the next frame reuses.

### ComputeShader (Standalone Slime Demo)

ComputeShader stores agents in an SSBO with position, angle, and per-agent RGB color (`ComputeShader/app/src/main/resources/compute.glsl:23`).
ComputeShader uses a two-stage compute shader:

- Stage 0 moves agents and writes deposits (`ComputeShader/app/src/main/resources/compute.glsl:93`).
- Stage 1 blurs and decays the trail map (`ComputeShader/app/src/main/resources/compute.glsl:201`).

ComputeShader drives the stages from the CPU and copies output textures back into inputs each stage (`ComputeShader/app/src/main/java/computeshader/App.java:258`).
ComputeShader draws the resulting trail map directly to the screen without additional post effects (`ComputeShader/app/src/main/java/computeshader/App.java:299`).

## Key Differences and Visual Impact

### 1) Agent Count and Initialization Distribution

- AudioJones defaults to 100,000 agents (`src/render/physarum.h:17`) and initializes positions uniformly across the screen (`src/render/physarum.cpp:37`).
- ComputeShader defaults to 2^23 agents (8,388,608) (`ComputeShader/app/src/main/java/computeshader/App.java:155`) and initializes positions within a center disk (`ComputeShader/app/src/main/java/computeshader/App.java:421`).

Impact on visuals:

- Higher agent density saturates the trail map faster and produces more continuous filaments per frame.
- Center-disk initialization produces a visible “growth from center” phase before the simulation fills the frame.

### 2) Agent Color Representation and Steering Metric

- AudioJones stores a hue scalar per agent and converts that hue to RGB only during deposition (`shaders/physarum_agents.glsl:183`).
- ComputeShader stores per-agent RGB and converts to HSV hue for steering (`ComputeShader/app/src/main/resources/compute.glsl:127`).

AudioJones steering:

- AudioJones computes a signed attraction score using circular hue distance and trail brightness (`shaders/physarum_agents.glsl:109`, `shaders/physarum_agents.glsl:119`).
- AudioJones repels agents from opposite hues by allowing negative scores (`shaders/physarum_agents.glsl:125`).

ComputeShader steering:

- ComputeShader computes `abs(agentHue - trailHue)` and compares distances to steer (`ComputeShader/app/src/main/resources/compute.glsl:132`).
- This computation does not implement circular hue wrap, so hue values near 0 and 1 appear far apart.

Impact on visuals:

- Circular hue distance removes a “red seam” artifact when hues wrap at 0/1.
- Signed repulsion strengthens species separation and increases empty boundaries between color domains.

### 3) Trail Deposition Model (Blend vs Additive)

- AudioJones blends the current pixel toward the agent’s deposit color and clamps brightness (`shaders/physarum_agents.glsl:188`, `shaders/physarum_agents.glsl:192`).
- ComputeShader adds scaled RGB into the trail map (`ComputeShader/app/src/main/resources/compute.glsl:193`).
- ComputeShader computes the deposited value using `TrailMap` at the old pixel, then writes it to the new pixel (`ComputeShader/app/src/main/resources/compute.glsl:187`).

Impact on visuals:

- Additive deposition increases brightness over time and can push mixed-color regions toward white saturation.
- Blended deposition preserves chroma and caps intensity earlier, which reduces overbright “blowout” regions.
- Old-pixel sampling transports existing trail energy along the agent path, which strengthens continuous streaks.

### 4) Trail Buffering Strategy (In-Place vs Double-Buffered)

- AudioJones reads and writes the same image2D trail map inside one dispatch (`shaders/physarum_agents.glsl:107`, `shaders/physarum_agents.glsl:194`).
- ComputeShader reads from `TrailMap` and writes to `TrailMapOut`, then copies outputs back to inputs (`ComputeShader/app/src/main/resources/compute.glsl:9`, `ComputeShader/app/src/main/java/computeshader/App.java:273`).

Impact on visuals:

- Double-buffering reduces within-frame read-after-write feedback, so sensing reads a stable previous state.
- In-place updates introduce execution-order dependence when multiple agents write the same pixel (`shaders/physarum_agents.glsl:194`).

### 5) Diffusion Kernel (Gaussian vs Box) and Edge Handling

- AudioJones uses a separable 5-tap Gaussian blur and clamps UVs at edges (`shaders/blur_h.fs:25`, `shaders/blur_h.fs:30`).
- ComputeShader uses a 5×5 box blur and wraps sample coordinates with `mod` (`ComputeShader/app/src/main/resources/compute.glsl:213`).

Impact on visuals:

- Gaussian blur produces smoother diffusion with fewer flat “plateau” regions at a fixed iteration count.
- Wrap-around diffusion produces seamless continuity across edges, which removes visible border absorption.
- Both implementations wrap agent positions at screen boundaries (`shaders/physarum_agents.glsl:175`, `ComputeShader/app/src/main/resources/compute.glsl:152`).

### 6) Decay Model (Time-Based Half-Life vs Per-Frame Multiplier)

- AudioJones computes a framerate-independent decay multiplier using `halfLife` and `deltaTime` (`shaders/blur_v.fs:38`).
- ComputeShader multiplies by `(1.0 - decayAmount)` each Stage 1 pass (`ComputeShader/app/src/main/resources/compute.glsl:219`).

Impact on visuals:

- Time-based decay keeps trail persistence stable across machines and frame rates.
- Per-frame decay changes trail persistence when frame rate changes, even with identical parameters.

Mapping note:

- ComputeShader’s effective half-life equals `dt * ln(0.5) / ln(1 - decayAmount)` (TBD: confirm dt usage).

### 7) Integration Into a Larger Render Pipeline

- AudioJones warps and composites the accumulation buffer after physarum via feedback and voronoi passes (`src/render/post_effect.cpp:270`).
- ComputeShader displays the trail map directly without feedback warping (`ComputeShader/app/src/main/java/computeshader/App.java:299`).

Impact on visuals:

- Feedback warping turns stable networks into swirls and recursive structures over multiple frames.
- Direct display produces patterns that reflect only the physarum dynamics and diffusion/decay settings.

## Observed Visual Output (ComputeShader)

ComputeShader includes a sequence of screenshots that show multiple stable regimes (`ComputeShader/images/README.md:1`).
These images include:

- Dense “worm” textures with many short segments (`ComputeShader/images/README.md:1`).
- High-contrast network structures with large empty cells (`ComputeShader/images/README.md:5`).
- Thick, high-brightness filaments with sparse background trails (`ComputeShader/images/README.md:16`).

The screenshots labeled “After converting color to HSV for sensing” align with the hue-based steering code path (`ComputeShader/app/src/main/resources/compute.glsl:127`).

## Expected Visual Differences in AudioJones (Needs Validation)

AudioJones’s default agent count and blended deposition reduce trail saturation compared to ComputeShader (`src/render/physarum.h:17`, `shaders/physarum_agents.glsl:188`).
AudioJones’s exponential half-life stabilizes persistence under variable frame times (`shaders/blur_v.fs:38`).
AudioJones’s circular hue distance and repulsion reduce cross-hue mixing and increase color-domain separation (`shaders/physarum_agents.glsl:119`, `shaders/physarum_agents.glsl:125`).

This repository does not include reference screenshots for these claims (TBD).

## Experiment Plan (Reproducible Comparison)

1. Match resolution and agent count between projects (`src/render/physarum.h:17`, `ComputeShader/app/src/main/java/computeshader/App.java:155`).
2. Disable AudioJones feedback/voronoi/chromatic to isolate physarum (`src/render/post_effect.cpp:269`).
3. Replace AudioJones Gaussian blur with a 5×5 box blur to match ComputeShader diffusion (`shaders/blur_h.fs:25`).
4. Convert ComputeShader per-frame decay to a half-life parameter to match AudioJones (`ComputeShader/app/src/main/resources/compute.glsl:219`).
5. Capture still frames at fixed time stamps and compare edge continuity and color-domain boundaries (TBD).

## Open Questions

- AudioJones render textures use an unspecified sampling filter for `texture()` fetches in the blur shaders (TBD).
- AudioJones in-place `imageLoad`/`imageStore` behavior depends on driver scheduling and can vary by GPU (TBD).
- ComputeShader Stage 0 “old pixel read, new pixel write” behavior may be intentional advection or an indexing bug (TBD) (`ComputeShader/app/src/main/resources/compute.glsl:187`).
