# Gray-Scott Reaction-Diffusion

GPU compute shader implementation of the Gray-Scott reaction-diffusion model as a standalone visualization effect.

## Goal

Add a second organic simulation effect that coexists with physarum. Gray-Scott produces self-organizing patterns (spots, stripes, mazes, coral) through chemical reaction simulation. Users tune Feed (F) and Kill (k) parameters via sliders to explore different pattern types, with presets for common patterns from Pearson's classification.

The effect runs independently of audio, uses the same ColorConfig hue system as physarum, and blends via the existing boost shader. Full-screen resolution matches the window.

## Current State

### Physarum Architecture (Template)

- `src/render/physarum.h:8-59` - PhysarumAgent, PhysarumConfig, Physarum state struct
- `src/render/physarum.cpp:210-268` - Init: compute shader loading, SSBO creation, ping-pong RGBA32F textures
- `src/render/physarum.cpp:287-374` - Update: compute dispatch, trail processing
- `src/render/physarum.cpp:376-391` - Debug overlay rendering
- `shaders/physarum_trail.glsl:1-47` - Trail compute shader (5-tap Gaussian, decay)

### Post-Effect Integration

- `src/config/effect_config.h:27` - PhysarumConfig embedded in EffectConfig
- `src/render/post_effect.h:49` - Physarum pointer in PostEffect struct
- `src/render/post_effect.cpp:134-139` - PhysarumApplyConfig/Update/ProcessTrails per frame
- `src/render/post_effect.cpp:329-340` - Trail boost blending in output pipeline
- `shaders/physarum_boost.fs:1-35` - Headroom-limited intensity boost

### ColorConfig System

- `src/render/color_config.h:6-18` - ColorMode (SOLID/RAINBOW), ColorConfig struct with hue/saturation/value
- `shaders/physarum_agents.glsl:164-167` - HSV-to-RGB conversion for colored output

### Research Foundation

- `docs/research/reaction-diffusion-systems.md:10-14` - Gray-Scott PDEs
- `docs/research/reaction-diffusion-systems.md:108-115` - Pearson parameter presets
- `docs/research/reaction-diffusion-systems.md:132-135` - 9-point Laplacian stencil
- `docs/research/reaction-diffusion-systems.md:174-181` - Discrete update equations

## Algorithm

### Gray-Scott Equations

Two chemical species (U and V) react and diffuse on a 2D grid:

```
dU/dt = Du * laplacian(U) - U*V*V + F*(1-U)
dV/dt = Dv * laplacian(V) + U*V*V - (F+k)*V
```

| Parameter | Value | Description |
|-----------|-------|-------------|
| Du | 1.0 | U diffusion coefficient (fixed) |
| Dv | 0.5 | V diffusion coefficient (fixed, 2:1 ratio enables all patterns) |
| F | 0.01-0.11 | Feed rate (user-tunable) |
| k | 0.04-0.10 | Kill rate (user-tunable) |
| dt | 0.5 | Timestep (at stability limit for explicit Euler) |

### 9-Point Laplacian Stencil

Isotropic diffusion kernel (reduces directional bias vs 5-point):

```
        [0.05  0.2   0.05]
        [0.2   -1    0.2 ]
        [0.05  0.2   0.05]
```

Adjacent neighbors (4) weighted 0.2, diagonal neighbors (4) weighted 0.05, center -1.0.

### Discrete Update

Per-pixel per-substep:

```glsl
float lapU = laplacian9(U);
float lapV = laplacian9(V);
float UVV = U * V * V;

float newU = U + (Du * lapU - UVV + F * (1.0 - U)) * dt;
float newV = V + (Dv * lapV + UVV - (F + k) * V) * dt;

newU = clamp(newU, 0.0, 1.0);
newV = clamp(newV, 0.0, 1.0);
```

### Substeps

Run 4 substeps per frame at dt=0.5 to accelerate pattern evolution (effective 2.0 simulation time per frame at 60fps).

### Boundary Conditions

Periodic (toroidal wrap) via modulo indexing. Patterns tile seamlessly.

### Initial Conditions

U=1.0 everywhere, V=0.0 with random noise perturbation in 64x64 pixel central region. Seed density 2-5% provides good nucleation without over-saturation.

### Parameter Presets

Based on Pearson classification:

| Preset | F | k | Pattern |
|--------|---|---|---------|
| Mitosis | 0.0367 | 0.0649 | Self-replicating dividing cells |
| Coral | 0.0545 | 0.0620 | Branching coral structures |
| Fingerprint | 0.0550 | 0.0620 | Parallel stripes |
| Spots | 0.0380 | 0.0990 | Isolated hexagonal dots |
| Maze | 0.0290 | 0.0570 | Connected pathways |
| Waves | 0.0340 | 0.0950 | Traveling waves |

### Color Mapping

V concentration drives color via ColorConfig:

```glsl
float hue = mix(solidHue, V * hueRange + hueOffset, float(colorMode));
vec3 rgb = hsv2rgb(vec3(hue, saturation, value));
```

Output texture stores RGB color (not raw U/V). Boost shader blends based on luminance.

## Integration

### Data Flow

```
GrayScottConfig (UI)
        |
        v
GrayScottApplyConfig() --> GPU uniforms
        |
        v
GrayScottUpdate()
   - 4x substep loop:
     - Bind read texture
     - Bind write texture
     - Dispatch compute shader (16x16 workgroups)
     - Swap textures
        |
        v
stateMap texture (RGBA32F with colored output)
        |
        v
PostEffectToScreen()
   - trailBoostShader samples stateMap
   - Headroom-limited additive blend
        |
        v
Final output to screen
```

### PostEffect Pipeline Hooks

1. **Init** (`src/render/post_effect.cpp:253`) - Create GrayScott after Physarum
2. **Uninit** (`src/render/post_effect.cpp:264`) - Destroy GrayScott
3. **Resize** (`src/render/post_effect.cpp:296`) - Resize GrayScott textures
4. **Update** (`src/render/post_effect.cpp:141`) - Call GrayScottUpdate when enabled
5. **Boost** (`src/render/post_effect.cpp:342`) - Blend GrayScott output via boost shader

### UI Panel

Add accordion section in `src/ui/ui_panel_effects.cpp` after physarum:

```
[+] Gray-Scott Reaction-Diffusion
    [x] Enabled
    Feed (F)  [========] 0.0367
    Kill (k)  [========] 0.0649
    Substeps  [====    ] 4
    Boost     [===     ] 0.5

    Presets: [Mitosis] [Coral] [Maze] [Spots]

    [Color Config controls]
    [ ] Debug Overlay
    [Reset]
```

## File Changes

| File | Change |
|------|--------|
| `src/render/grayscott.h` | Create - GrayScottConfig, GrayScott struct, API declarations |
| `src/render/grayscott.cpp` | Create - Init/Uninit/Update/Resize/Reset/ApplyConfig implementation |
| `shaders/grayscott.glsl` | Create - Compute shader with 9-point Laplacian and PDE logic |
| `src/config/effect_config.h` | Add `GrayScottConfig grayscott` field |
| `src/render/post_effect.h` | Add `GrayScott* grayscott` field, forward declaration |
| `src/render/post_effect.cpp` | Init/Uninit/Resize/Update/Boost integration (5 locations) |
| `src/ui/ui_panel_effects.cpp` | Add Gray-Scott accordion section with sliders and preset buttons |
| `src/config/preset.cpp` | Add GrayScottConfig JSON serialization |
| `CMakeLists.txt` | Add grayscott.cpp and shader copy command |

**Total: 3 new files, 6 modified files, ~600 lines of code**

## Build Sequence

### Phase 1: Core Simulation Module

1. Create `src/render/grayscott.h`:
   - `GrayScottConfig` struct with enabled, feedRate, killRate, substeps, boostIntensity, debugOverlay, ColorConfig
   - `GrayScott` struct with computeProgram, stateMap/stateMapTemp, uniform locations, dimensions
   - API: `GrayScottSupported/Init/Uninit/Update/Reset/Resize/ApplyConfig/DrawDebug`

2. Create `shaders/grayscott.glsl`:
   - OpenGL 4.3 compute shader with 16x16 local workgroup
   - Read from `inputState` image (binding 1), write to `outputState` image (binding 2)
   - 9-point Laplacian function with periodic boundary modulo
   - Gray-Scott PDE update with clamping
   - HSV-to-RGB color output based on V concentration and ColorConfig uniforms

3. Create `src/render/grayscott.cpp`:
   - `GrayScottSupported()` - Check RLGL_COMPUTE_SHADER support
   - `GrayScottInit()` - Load shader, create RGBA32F textures, cache uniform locations, seed initial state
   - `GrayScottUninit()` - Unload shader and textures
   - `GrayScottUpdate()` - 4-substep loop: set uniforms, bind images, dispatch, swap textures
   - `GrayScottReset()` - Re-seed initial conditions (U=1, V=noise)
   - `GrayScottResize()` - Recreate textures at new size, re-seed
   - `GrayScottApplyConfig()` - Update cached config (no agent buffer like physarum)
   - `GrayScottDrawDebug()` - Draw stateMap as full-screen quad

4. Build and test standalone: verify patterns emerge from noise with default F/k values

### Phase 2: Config and PostEffect Integration

1. Modify `src/config/effect_config.h`:
   - Add forward declaration or include for grayscott.h
   - Add `GrayScottConfig grayscott;` field after PhysarumConfig

2. Modify `src/render/post_effect.h`:
   - Add forward declaration: `typedef struct GrayScott GrayScott;`
   - Add `GrayScott* grayscott;` field to PostEffect struct

3. Modify `src/render/post_effect.cpp`:
   - Add `#include "grayscott.h"`
   - In `PostEffectInit()`: `pe->grayscott = GrayScottInit(width, height, &effects->grayscott);`
   - In `PostEffectUninit()`: `GrayScottUninit(pe->grayscott);`
   - In `PostEffectResize()`: `GrayScottResize(pe->grayscott, width, height);`
   - In `ApplyAccumulationPipeline()` after physarum block:
     ```cpp
     if (pe->effects.grayscott.enabled) {
         GrayScottApplyConfig(pe->grayscott, &pe->effects.grayscott);
         GrayScottUpdate(pe->grayscott);
     }
     ```
   - In `ApplyOutputPipeline()` after physarum boost:
     ```cpp
     if (pe->grayscott && pe->effects.grayscott.enabled &&
         pe->effects.grayscott.boostIntensity > 0.0f) {
         BeginTextureMode(pe->tempTexture);
         BeginShaderMode(pe->trailBoostShader);
             SetShaderValueTexture(pe->trailBoostShader, pe->trailMapLoc,
                                   pe->grayscott->stateMap.texture);
             SetShaderValue(pe->trailBoostShader, pe->trailBoostIntensityLoc,
                            &pe->effects.grayscott.boostIntensity, SHADER_UNIFORM_FLOAT);
             DrawFullscreenQuad(pe, currentSource);
         EndShaderMode();
         EndTextureMode();
         currentSource = &pe->tempTexture;
     }
     ```

4. Build and verify: enable Gray-Scott via debugger/hardcode, confirm compute shader executes

### Phase 3: UI and Serialization

1. Modify `src/ui/ui_panel_effects.cpp`:
   - Add `DrawGrayScottPanel(GrayScottConfig* config)` function after physarum panel
   - Include: checkbox (enabled), sliders (F, k, substeps, boost), preset buttons, ColorConfig, debug toggle, reset button
   - Call from main panel draw function

2. Modify `src/config/preset.cpp`:
   - Add `GrayScottConfig` fields to JSON serialization (enabled, feedRate, killRate, substeps, boostIntensity, debugOverlay, color)
   - Use `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` or manual to/from_json

3. Modify `CMakeLists.txt`:
   - Add `src/render/grayscott.cpp` to source list
   - Add shader copy for `shaders/grayscott.glsl`

4. Build and test: adjust F/k sliders, verify patterns change; save/load preset, verify persistence

### Phase 4: Validation and Polish

1. Test all presets:
   - Mitosis (F=0.0367, k=0.0649) - dividing cells within 5 seconds
   - Coral (F=0.0545, k=0.0620) - branching structures
   - Fingerprint (F=0.0550, k=0.0620) - parallel stripes
   - Spots (F=0.0380, k=0.0990) - stable hexagonal dots
   - Maze (F=0.0290, k=0.0570) - connected pathways

2. Test coexistence: enable both physarum and Gray-Scott, verify additive blending works

3. Test ColorConfig: verify SOLID and RAINBOW modes produce expected coloring

4. Test window resize: verify simulation re-initializes at new resolution

5. Performance check: verify 60fps at 1920x1080 with 4 substeps

## Validation

- [ ] Gray-Scott checkbox enables/disables simulation
- [ ] Debug overlay shows colored pattern output
- [ ] Mitosis preset produces dividing cells within 5 seconds
- [ ] Coral preset produces branching coral structures
- [ ] Fingerprint preset produces parallel stripe patterns
- [ ] Spots preset produces stable hexagonal dot arrays
- [ ] Maze preset produces connected labyrinth patterns
- [ ] Feed (F) slider range 0.01-0.11 with visible pattern changes
- [ ] Kill (k) slider range 0.04-0.10 with visible pattern changes
- [ ] Substeps slider 1-8 affects evolution speed
- [ ] Boost intensity 0.0-2.0 affects output brightness
- [ ] ColorConfig SOLID mode produces single-hue output with V-driven brightness
- [ ] ColorConfig RAINBOW mode produces hue-mapped output
- [ ] Reset button re-seeds simulation
- [ ] Presets save/load correctly via preset system
- [ ] Window resize re-initializes simulation at new resolution
- [ ] Both physarum and Gray-Scott enabled simultaneously blend additively
- [ ] No OpenGL errors during update/render
- [ ] 60fps maintained at 1920x1080 with Gray-Scott enabled

## References

- `docs/research/reaction-diffusion-systems.md` - Mathematical foundations, parameter space, GPU implementation
- `src/render/physarum.h:8-59` - API pattern for Init/Uninit/Update/ApplyConfig
- `src/render/physarum.cpp:210-486` - Implementation reference for compute shader management
- `shaders/physarum_trail.glsl:1-47` - Compute shader structure (local_size, image bindings)
- `shaders/physarum_boost.fs:1-35` - Boost blending shader (reused for Gray-Scott)
- Karl Sims - Reaction-Diffusion Tutorial (karlsims.com/rd.html)
