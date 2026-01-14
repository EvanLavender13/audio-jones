# Curl-Advection Field

Self-advecting vector field simulation creating organic veiny and vortical patterns. Each pixel stores a velocity vector that advects itself through multi-step path tracing. Curl creates rotation, divergence creates sources/sinks, and laplacian diffuses. The balance between competing forces produces emergent structure.

## Current State

Existing simulation patterns to follow:
- `src/simulation/curl_flow.h:19-35` - CurlFlowConfig struct pattern
- `src/simulation/curl_flow.cpp:132-196` - Init/Uninit/Update flow
- `src/render/render_pipeline.cpp:47-66` - ApplyCurlFlowPass pattern
- `src/ui/imgui_effects.cpp:213-242` - CurlFlow UI section
- `docs/research/curl-advection-field.md` - Algorithm specification

New infrastructure:
- `src/simulation/curl_advection.h` - Config and simulation structs
- `src/simulation/curl_advection.cpp` - Implementation
- `shaders/curl_advection.glsl` - Compute shader

## Technical Implementation

**Source**: Research doc `docs/research/curl-advection-field.md`, user-provided ShaderToy shader

### State Storage

Each pixel stores `vec4(velocity.xy, divergence, unused)` in RGBA16F texture.

### Differential Operators (3x3 Stencil)

Sample 9 neighbors with diagonal weighting for stable operators:

```glsl
#define _D 0.6      // diagonal contribution
#define _K0 -20.0/6.0  // laplacian center
#define _K1 4.0/6.0    // laplacian edges
#define _K2 1.0/6.0    // laplacian corners
#define _G0 0.25       // gaussian center
#define _G1 0.125      // gaussian edges
#define _G2 0.0625     // gaussian corners

// Sample all neighbors
vec3 uv    = sample(0, 0);
vec3 uv_n  = sample(0, 1);   vec3 uv_s  = sample(0, -1);
vec3 uv_e  = sample(1, 0);   vec3 uv_w  = sample(-1, 0);
vec3 uv_ne = sample(1, 1);   vec3 uv_nw = sample(-1, 1);
vec3 uv_se = sample(1, -1);  vec3 uv_sw = sample(-1, -1);

// Curl (rotation): dv/dx - du/dy
float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y
    + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y
          + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);

// Divergence (expansion): du/dx + dv/dy
float div = uv_s.y - uv_n.y - uv_e.x + uv_w.x
    + _D * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y
          + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);

// Laplacian (diffusion)
vec3 lapl = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s)
                   + _K2*(uv_nw + uv_sw + uv_ne + uv_se);

// Gaussian blur
vec3 blur = _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s)
                   + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
```

### Multi-Step Self-Advection

Trace the field along its own flow, accumulating blur:

```glsl
#define STEPS 40

vec2 off = uv.xy;      // Current offset
vec2 offd = off;       // Direction (rotated by curl)
vec3 ab = vec3(0);     // Accumulated blur

for (int i = 0; i < STEPS; i++) {
    vec3 s = sampleAt(off);
    float localCurl = computeCurl(off);
    offd = rotate2d(offd, advectionCurl * localCurl);
    off += offd;
    ab += gaussianBlur(off) / float(STEPS);
}
```

### Field Update Equation

Combine all forces:

```glsl
// Pressure-like term from laplacian of divergence
float sp = pressureScale * lapl.z;

// Curl rotation amount
float sc = curlScale * curl;

// Divergence update with feedback
float sd = uv.z + divergenceUpdate * div + divergenceSmoothing * lapl.z;

// Normalize velocity for pressure term
vec2 norm = length(uv.xy) > 0.0 ? normalize(uv.xy) : vec2(0);

// Combine forces
vec2 tab = selfAmp * ab.xy           // Self-amplification of advected field
         + laplacianScale * lapl.xy  // Diffusion
         + norm * sp                  // Pressure gradient
         + uv.xy * divergenceScale * sd;  // Divergence feedback

// Apply curl rotation to combined result
vec2 rab = rotate2d(tab, sc);

// Blend with previous state for stability
vec3 result = mix(vec3(rab, sd), uv, updateSmoothing);

// Clamp to valid range
result.z = clamp(result.z, -1.0, 1.0);
result.xy = length(result.xy) > 1.0 ? normalize(result.xy) : result.xy;
```

### Energy Injection

Replace mouse interaction with modulation-driven injection:

```glsl
// Inject velocity at center when injectionIntensity > 0
vec2 center = resolution * 0.5;
vec2 d = (pos - center) / resolution.x;
float falloff = exp(-length(d) / 0.05);
result.xy += injectionIntensity * normalize(d) * falloff;
```

### Color Output

Velocity angle maps to ColorLUT (same as CurlFlow):

```glsl
float angle = atan(result.y, result.x);
float t = (angle + PI) / (2.0 * PI);  // Normalize to 0-1
vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
float brightness = length(result.xy);  // Velocity magnitude
vec3 output = color * brightness * value;
```

### Parameters

| Name | Default | Range | Uniform | Effect |
|------|---------|-------|---------|--------|
| `steps` | 40 | 10-80 | int | Advection iterations (cost scales linearly) |
| `advectionCurl` | 0.2 | 0.0-1.0 | float | How much paths spiral during tracing |
| `curlScale` | -2.0 | -4.0-4.0 | float | Vortex rotation strength |
| `selfAmp` | 1.0 | 0.5-2.0 | float | Instability driver |
| `updateSmoothing` | 0.4 | 0.1-0.9 | float | Temporal stability |
| `injectionIntensity` | 0.0 | 0.0-1.0 | float | Energy injection (modulatable) |

---

## Phase 1: Core Algorithm

**Goal**: Compute shader runs curl-advection simulation with basic output.

**Build**:
- Create `src/simulation/curl_advection.h`:
  - `CurlAdvectionConfig` struct with core parameters and ColorConfig
  - `CurlAdvection` struct with state texture, program handles, uniform locations
  - API declarations: Init, Uninit, Update, ProcessTrails, Reset, ApplyConfig
- Create `src/simulation/curl_advection.cpp`:
  - State texture creation (RGBA16F ping-pong pair)
  - Compute shader loading with uniform binding
  - Update dispatches compute, swaps ping-pong buffers
  - ColorLUT integration for angle-to-color mapping
  - Deposits result to TrailMap for compositing
- Create `shaders/curl_advection.glsl`:
  - Stencil sampling from state texture
  - Differential operators (curl, divergence, laplacian, blur)
  - Multi-step advection loop
  - Field update equation
  - Color output to TrailMap via ColorLUT

**Done when**: Simulation runs, produces flowing patterns, TrailMap receives colored output.

---

## Phase 2: Integration

**Goal**: Simulation appears in render pipeline, UI, and presets.

**Build**:
- Modify `src/config/effect_config.h`:
  - Include curl_advection.h
  - Add `CurlAdvectionConfig curlAdvection` to EffectConfig
- Modify `src/render/post_effect.h`:
  - Forward declare `CurlAdvection`
  - Add `CurlAdvection* curlAdvection` pointer
- Modify `src/render/post_effect.cpp`:
  - Call `CurlAdvectionInit` in PostEffectInit
  - Call `CurlAdvectionUninit` in PostEffectUninit
  - Handle resize in PostEffectResize
  - Clear in PostEffectClearFeedback
- Modify `src/render/render_pipeline.cpp`:
  - Add `ApplyCurlAdvectionPass` static function (copy CurlFlow pattern)
  - Call in `ApplySimulationPasses`
  - Add trail boost pass in `RenderPipelineApplyOutput`
- Modify `src/ui/imgui_effects.cpp`:
  - Add section state `sectionCurlAdvection`
  - Add UI section after Boids with core parameter sliders
  - Use ModulatableSlider for injectionIntensity
  - ColorConfig editor via ImGuiDrawColorMode

**Done when**: Enable curl-advection from UI, see simulation running, parameters adjust behavior.

---

## Phase 3: Preset & Modulation

**Goal**: Save/load presets, modulation routes work.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for CurlAdvectionConfig
  - Add to_json entry for curlAdvection
  - Add from_json entry for curlAdvection
- Modify `src/automation/param_registry.cpp`:
  - Add PARAM_TABLE entries for modulatable params (injectionIntensity, selfAmp, curlScale)
  - Add corresponding targets array entries at matching indices

**Done when**: Preset save/load preserves curl-advection settings, modulation controls injectionIntensity.

---

## Phase 4: Polish

**Goal**: Debug overlay, edge cases, documentation.

**Build**:
- Add debug overlay rendering (velocity field visualization)
- Test boundary wrapping (toroidal topology)
- Test with various ColorConfig modes (solid, gradient, rainbow)
- Handle edge cases (zero velocity initialization, extreme parameter values)
- Update `docs/architecture.md` system diagram if needed

**Done when**: Stable simulation with all color modes working, debug overlay functional.

**Implementation Notes**:
- Fixed advection sign: `uv - off * texel` (trace backwards) instead of `uv + off * texel`
- Added Lissajous injection center animation: `injectionAmplitude` (0-0.5), `injectionFreqX/Y` (0.1-5.0 Hz)
