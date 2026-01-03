# Attractor & Curl Flow Shader Optimization

Eliminate redundant per-agent GPU computation by precomputing rotation matrices on CPU and replacing procedural noise with texture lookups.

## Current State

Performance bottlenecks identified in compute shaders running on 100K+ agents:

- `shaders/attractor_agents.glsl:45-67` - `rotationMatrix()` computes 6 trig ops per agent per frame
- `shaders/curl_flow_agents.glsl:49-118` - `snoise3_grad()` runs ~100 ALU ops per agent
- `shaders/curl_flow_agents.glsl:174-180` - Density gradient samples 5 textures per agent

Key files:
- `src/simulation/attractor_flow.cpp:227-228` - Passes rotation as vec3, shader rebuilds mat3
- `src/simulation/curl_flow.cpp:159-206` - Dispatches curl flow compute shader
- `src/simulation/curl_flow.h:37-61` - CurlFlow struct with texture handles
- `src/render/color_lut.cpp:5-21` - Pattern for CPU texture generation

## Phase 1: Attractor Rotation Precomputation

**Goal**: Eliminate 6 trig ops per agent by computing rotation matrix on CPU.

**Build**:
- `src/simulation/attractor_flow.h` - Replace `rotationLoc` with `rotationMatrixLoc`
- `src/simulation/attractor_flow.cpp` - Compute mat3 from Euler angles in `AttractorFlowUpdate()`, pass via `glUniformMatrix3fv`
- `shaders/attractor_agents.glsl` - Change `uniform vec3 rotation` to `uniform mat3 rotationMatrix`, delete `rotationMatrix()` function, use uniform directly at line 183

**Done when**: Attractor renders identically with rotation controls working.

---

## Phase 2: NoiseTexture3D Module

**Goal**: Create reusable 3D tileable noise texture infrastructure for curl flow and future effects.

**Build**:
- `src/simulation/noise_texture3d.h` - Struct with texture handle, size, frequency; declare Init/Uninit/GetTexture/Regenerate
- `src/simulation/noise_texture3d.cpp` - Generate 128^3 tileable simplex noise on CPU, upload as GL_TEXTURE_3D with RG16F format (stores curl XY), trilinear filtering, GL_REPEAT wrap
- `CMakeLists.txt` - Add new source file

NoiseTexture3D struct:
```
unsigned int textureId;  // GL_TEXTURE_3D handle
int size;                // Cube dimension (128)
float frequency;         // Noise frequency used during generation
```

**Done when**: `NoiseTexture3DInit(128, 1.0f)` returns valid texture, debug visualization shows smooth tileable noise.

---

## Phase 3: Curl Flow Gradient Pass

**Goal**: Precompute density gradient into texture, reducing 5 samples per agent to 1.

**Build**:
- `shaders/curl_gradient.glsl` - Compute shader: sample density at +/- gradientRadius in X/Y, compute central differences, write to RG16F image
- `src/simulation/curl_flow.h` - Add `gradientTexture`, `gradientProgram`, gradient uniform locations to CurlFlow struct
- `src/simulation/curl_flow.cpp` - Add `CreateGradientTexture()` and `LoadGradientProgram()` static functions, initialize in `CurlFlowInit()`, dispatch gradient pass in `CurlFlowUpdate()` when `trailInfluence >= 0.001f`, cleanup in `CurlFlowUninit()`, recreate on resize

**Done when**: Gradient texture updates each frame when trail influence > 0, debug visualization shows gradient vectors.

---

## Phase 4: Curl Flow Shader Integration

**Goal**: Replace procedural noise and multi-sample gradient with texture lookups.

**Build**:
- `src/simulation/curl_flow.h` - Add `NoiseTexture3D*` pointer, `noiseTextureLoc`, `gradientTextureLoc`
- `src/simulation/curl_flow.cpp` - Initialize NoiseTexture3D in `CurlFlowInit()`, bind noise (slot 4) and gradient (slot 5) textures in `CurlFlowUpdate()`, cleanup in `CurlFlowUninit()`
- `shaders/curl_flow_agents.glsl` - Add `sampler3D noiseTexture` and `sampler2D gradientMap` uniforms, replace `snoise3_grad()` call with texture lookup, replace 5-sample gradient with single texture sample, delete unused simplex noise functions (lines 44-128)

**Done when**: Curl flow renders with same visual quality, procedural noise code removed from shader.

---

## Phase 5: Verification

**Goal**: Confirm optimization correctness and measure performance gains.

**Build**:
- Test attractor rotation at various angles, verify no visual difference
- Test curl flow with `trailInfluence` at 0, 0.5, 1.0 - verify flow bending works
- Test curl flow with varying `noiseFrequency` - verify noise scales correctly
- Profile GPU frame time with 100K+ agents before/after

**Done when**: Visual output matches pre-optimization, GPU time reduced.

---

## Memory Impact

| Resource | Size |
|----------|------|
| 3D noise texture (128^3 RG16F) | 8 MB |
| Gradient texture (1080p RG16F) | 8 MB |
| **Total additional VRAM** | ~16 MB |

## Expected Performance Gains

| Metric | Before | After |
|--------|--------|-------|
| Attractor trig ops/agent | 6 | 0 |
| Curl noise ALU/agent | ~100 | ~10 |
| Curl density samples/agent | 5 | 1 |
