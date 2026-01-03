# Attractor & Curl Flow Shader Optimization

Eliminate redundant per-agent GPU computation by precomputing rotation matrices on CPU and density gradients into textures.

## Current State

Performance bottlenecks identified in compute shaders running on 100K+ agents:

- `shaders/attractor_agents.glsl:45-67` - `rotationMatrix()` computes 6 trig ops per agent per frame
- `shaders/curl_flow_agents.glsl:174-180` - Density gradient samples 5 textures per agent

Key files:
- `src/simulation/attractor_flow.cpp:227-228` - Passes rotation as vec3, shader rebuilds mat3
- `src/simulation/curl_flow.cpp:159-206` - Dispatches curl flow compute shader
- `src/simulation/curl_flow.h:37-61` - CurlFlow struct with texture handles

## Phase 1: Attractor Rotation Precomputation

**Status**: COMPLETE

**Goal**: Eliminate 6 trig ops per agent by computing rotation matrix on CPU.

**Build**:
- `src/simulation/attractor_flow.h` - Replace `rotationLoc` with `rotationMatrixLoc`
- `src/simulation/attractor_flow.cpp` - Compute mat3 from Euler angles in `AttractorFlowUpdate()`, pass via `glUniformMatrix3fv`
- `shaders/attractor_agents.glsl` - Change `uniform vec3 rotation` to `uniform mat3 rotationMatrix`, delete `rotationMatrix()` function, use uniform directly

---

## Phase 2: NoiseTexture3D Module

**Status**: FAILED AND REVERTED

**Why it failed**: A tiling 3D texture cannot replace procedural noise. With noiseFrequency=0.005, a 1920px screen samples coords 0-9.6, causing the texture to repeat ~10 times with only ~25 texels per tile. Procedural noise has infinite resolution; finite textures don't.

**Why a screen-sized 2D texture also fails**: Computing noise once per pixel per frame (1920x1080 = 2M ops) is MORE work than computing noise once per agent (100K ops). The "optimization" is 20x worse.

**Conclusion**: Procedural noise in the agent shader is the only viable approach. No texture can replace it.

---

## Phase 3: Curl Flow Gradient Pass

**Status**: COMPLETE

**Goal**: Precompute density gradient into texture, reducing 5 samples per agent to 1.

**Build**:
- `shaders/curl_gradient.glsl` - Compute shader: sample density at +/- gradientRadius in X/Y, compute central differences, write to RG16F image
- `src/simulation/curl_flow.h` - Add `gradientTexture`, `gradientProgram`, gradient uniform locations to CurlFlow struct
- `src/simulation/curl_flow.cpp` - Add `CreateGradientTexture()` and `LoadGradientProgram()` static functions, initialize in `CurlFlowInit()`, dispatch gradient pass in `CurlFlowUpdate()` when `trailInfluence >= 0.001f`, cleanup in `CurlFlowUninit()`, recreate on resize

---

## Phase 4: Curl Flow Shader Integration

**Status**: FAILED AND REVERTED

Attempted to integrate 3D noise texture. Caused visible tiling artifacts. Reverted to procedural noise. Gradient texture integration remains.

---

## Final State

**What works**:
- Phase 1: Rotation matrix precomputation (6 trig ops per frame instead of per agent)
- Phase 3: Gradient texture (5 density samples per agent reduced to 1)

**What was deleted**:
- `src/simulation/noise_texture3d.h`
- `src/simulation/noise_texture3d.cpp`
- All 3D noise texture code from curl_flow.cpp/h
- All noise texture sampling from curl_flow_agents.glsl

**Procedural noise remains in shader** - this is correct and cannot be optimized with textures.

---

## Memory Impact

| Resource | Size |
|----------|------|
| Gradient texture (1080p RGBA16F) | 16 MB |
| **Total additional VRAM** | ~16 MB |

## Actual Performance Gains

| Metric | Before | After |
|--------|--------|-------|
| Attractor trig ops/agent | 6 | 0 |
| Curl noise ALU/agent | ~100 | ~100 (unchanged) |
| Curl density samples/agent | 5 | 1 |

---

## Bug Fix: Preset Switching FPS Drop

**Problem**: Switching GRAYBOB → ICEY → GRAYBOB dropped FPS to ~55. `ApplyConfig` was gated on `enabled`, so disabled sims kept stale 1M agent buffers.

**Fix** (`render_pipeline.cpp`): Always call `ApplyConfig()` regardless of enabled state. Only reset enabled sims in `PostEffectClearFeedback()`.
