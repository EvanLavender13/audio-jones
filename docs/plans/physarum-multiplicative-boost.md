# Physarum Multiplicative Boost

Convert physarum from color-depositing trails to an invisible intensity map with multiplicative compositing, matching voronoi's behavior where trails only brighten existing content.

## Goal

Physarum agents create organic trail networks that enhance audio-reactive visuals without competing with them. Trails are invisible on black backgrounds and only become visible when waveforms or spectrum bars pass through trail regions. This prevents the white accumulation problem caused by agents continuously sampling and redepositing scene colors.

**Prior art:** `shaders/voronoi.fs:74` uses `original * (1.0 + edgeMask * intensity)` - edges are invisible until they intersect colored content.

## Current State

**Problem:** Agents sample colors from `accumTexture`, deposit them elsewhere, causing exponential accumulation toward white.

**Agent structure** (`src/render/physarum.h:8-13`):
```cpp
typedef struct PhysarumAgent {
    float x, y, heading;
    float hue;  // Species identity - TO BE REMOVED
} PhysarumAgent;
```

**Compute shader** (`shaders/physarum_agents.glsl:180-194`):
- Deposits HSV-converted colors via lerp blending
- Reads AND writes to same `accumTexture` (RGBA32F)
- Uses hue-based species attraction scoring

**Pipeline integration** (`src/render/post_effect.cpp:269-272`):
```cpp
ApplyPhysarumPass(pe, deltaTime);  // Writes directly to accumTexture
ApplyVoronoiPass(pe);
ApplyFeedbackPass(pe, effectiveRotation);
ApplyBlurPass(pe, blurScale, deltaTime);
```

**Config** (`src/render/physarum.h:15-24`):
- Contains `ColorConfig color` for HSV computation - TO BE REMOVED

## Algorithm

Agents write scalar brightness to a dedicated R32F trail map. A post-process pass multiplies the scene by trail intensity.

**Agent behavior:**
1. Sample 3 sensors from `trailMap` (grayscale intensity)
2. Turn toward highest intensity (standard Jones algorithm)
3. Move forward by `stepSize`
4. Deposit `depositAmount` to `trailMap` at new position

**Multiplicative compositing:**
```glsl
vec3 scene = texture(sceneTexture, uv).rgb;
float trail = texture(trailMap, uv).r;
vec3 output = scene * (1.0 + trail * boostIntensity);
```

Black regions: `0 * (1 + trail) = 0` - stays black regardless of trail intensity.

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Trail format | R32F | Single channel sufficient; 25% memory of RGBA32F |
| Boost intensity range | 0.5-5.0 | 0.5 = subtle, 5.0 = dramatic highlighting |
| Default boost | 1.0 | Trail value 1.0 produces 2x brightness |
| Deposit soft cap | 10.0 | Prevents runaway accumulation in dense areas |
| Trail half-life | 0.5s | Independent decay for steering persistence |

## Architecture Decision

**Chosen:** Dedicated trail map with double-buffering and separate diffusion shaders.

**Rationale:**
- Agents never touch `accumTexture` - complete separation of concerns
- Double-buffered R32F avoids compute shader read/write hazards
- Dedicated R-only diffusion shaders (3x faster than RGB blur)
- Easy to extend later (RGB channels = 3 species)

**Rejected alternatives:**

| Approach | Rejected Reason |
|----------|-----------------|
| Reuse RGB blur shaders | Wasteful (processes 3 channels, uses 1) |
| Single trail map | Read/write hazards in compute shader |
| Alpha channel of accumTexture | Conceptually wrong; alpha may be used elsewhere |

## Component Design

### PhysarumAgent (simplified)

**File:** `src/render/physarum.h:8-13`

```cpp
typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    // REMOVED: float hue
} PhysarumAgent;
```

12 bytes per agent (down from 16).

### PhysarumConfig (revised)

**File:** `src/render/physarum.h:15-24`

```cpp
typedef struct PhysarumConfig {
    bool enabled = false;
    int agentCount = 100000;
    float sensorDistance = 20.0f;
    float sensorAngle = 0.5f;
    float turningAngle = 0.3f;
    float stepSize = 1.5f;
    float depositAmount = 1.0f;
    float boostIntensity = 1.0f;   // NEW: 0.5-5.0 range
    float trailHalfLife = 0.5f;    // NEW: independent decay
    // REMOVED: ColorConfig color
} PhysarumConfig;
```

### Physarum struct additions

**File:** `src/render/physarum.h:26-44`

Add:
- `RenderTexture2D trailMapA, trailMapB` - double-buffered R32F
- `bool useMapA` - ping-pong flag
- `Shader diffuseHShader, diffuseVShader` - trail diffusion
- `Shader boostShader` - multiplicative compositing
- Corresponding uniform locations

Remove:
- `saturationLoc`, `valueLoc` - no longer needed

### Agent compute shader

**File:** `shaders/physarum_agents.glsl`

Key changes:
- Remove `hue` from Agent struct
- Change bindings: `layout(r32f, binding = 1) uniform image2D trailMapRead`
- Add: `layout(r32f, binding = 2) uniform image2D trailMapWrite`
- Remove all HSV/RGB conversion functions (lines 44-100)
- Simplify `sampleTrailScore` to return raw brightness
- Deposit scalar: `imageStore(trailMapWrite, coord, vec4(newValue, 0, 0, 1))`

### Trail diffusion shaders (NEW)

**Files:** `shaders/physarum_diffuse_h.fs`, `shaders/physarum_diffuse_v.fs`

Single-channel Gaussian blur with exponential decay. Clone structure from `blur_h.fs`/`blur_v.fs` but operate on `.r` channel only.

### Boost compositing shader (NEW)

**File:** `shaders/physarum_boost.fs`

```glsl
uniform sampler2D sceneTexture;  // texture0 (accumTexture)
uniform sampler2D trailMap;      // texture1
uniform float boostIntensity;

void main() {
    vec3 scene = texture(sceneTexture, fragTexCoord).rgb;
    float trail = texture(trailMap, fragTexCoord).r;
    vec3 output = scene * (1.0 + trail * boostIntensity);
    finalColor = vec4(output, 1.0);
}
```

## Integration

### Pipeline order

**File:** `src/render/post_effect.cpp:269-277`

```cpp
// 1. Update physarum trail map (internal, never touches accumTexture)
PhysarumUpdate(pe->physarum, deltaTime);

// 2. Apply scene effects
ApplyVoronoiPass(pe);
ApplyFeedbackPass(pe, effectiveRotation);
ApplyBlurPass(pe, blurScale, deltaTime);

// 3. Apply physarum boost (accumTexture * trailMap)
PhysarumApplyBoost(pe->physarum, &pe->accumTexture, &pe->tempTexture);

// 4. Copy result back
BeginTextureMode(pe->accumTexture);
    DrawFullscreenQuad(pe, &pe->tempTexture);
// Leave open for waveform drawing
```

### API changes

**File:** `src/render/physarum.h`

```cpp
// OLD:
void PhysarumUpdate(Physarum* p, float deltaTime, RenderTexture2D* target);

// NEW:
void PhysarumUpdate(Physarum* p, float deltaTime);
void PhysarumApplyBoost(Physarum* p, RenderTexture2D* scene, RenderTexture2D* output);
```

### UI changes

**File:** `src/ui/ui_panel_effects.cpp`

Remove:
- Color mode dropdown
- Hue/saturation/value sliders

Add:
- `DrawLabeledSlider(l, "P.Boost", &effects->physarum.boostIntensity, 0.5f, 5.0f)`
- `DrawLabeledSlider(l, "P.Trail", &effects->physarum.trailHalfLife, 0.1f, 2.0f)`

### Preset serialization

**File:** `src/config/preset.cpp`

Remove keys: `colorMode`, `colorHue`, `colorSat`, `colorVal`
Add keys: `boostIntensity`, `trailHalfLife`

## File Changes

| File | Change |
|------|--------|
| `src/render/physarum.h` | Modify - remove hue from agent, remove ColorConfig, add boost/trail params, add trail map state |
| `src/render/physarum.cpp` | Modify - remove color logic, add R32F texture creation, implement ping-pong, add boost function |
| `shaders/physarum_agents.glsl` | Modify - remove HSV logic, dual R32F bindings, brightness-only sensing/deposit |
| `shaders/physarum_diffuse_h.fs` | Create - horizontal blur for R32F trail map |
| `shaders/physarum_diffuse_v.fs` | Create - vertical blur + decay for R32F trail map |
| `shaders/physarum_boost.fs` | Create - multiplicative compositing shader |
| `src/render/post_effect.cpp` | Modify - update ApplyPhysarumPass, add boost integration |
| `src/ui/ui_panel_effects.cpp` | Modify - remove color controls, add boost slider |
| `src/config/preset.cpp` | Modify - update JSON keys |

## Build Sequence

### Phase 1: Data Structure Cleanup

Remove species/color system from data structures. Physarum will be disabled during this phase.

- [ ] Remove `hue` field from `PhysarumAgent` in `src/render/physarum.h:8-13`
- [ ] Remove `ColorConfig color` from `PhysarumConfig` in `src/render/physarum.h:15-24`
- [ ] Add `float boostIntensity = 1.0f` and `float trailHalfLife = 0.5f` to `PhysarumConfig`
- [ ] Remove `#include "color_config.h"` if present
- [ ] Update `InitializeAgents` in `src/render/physarum.cpp:18-53` - remove hue assignment
- [ ] Remove `ColorConfigChanged` function from `src/render/physarum.cpp:223-234`
- [ ] Update `PhysarumApplyConfig` to remove color change detection in `src/render/physarum.cpp:236-284`
- [ ] Build and verify compilation succeeds

### Phase 2: Trail Map Infrastructure

Add R32F textures and new shaders to Physarum struct.

- [ ] Add `RenderTexture2D trailMapA, trailMapB` and `bool useMapA` to Physarum struct
- [ ] Add diffusion and boost shader state (shaders + uniform locations)
- [ ] Implement `InitRenderTextureR32F()` helper function
- [ ] Update `PhysarumInit` to create trail maps and load new shaders
- [ ] Update `PhysarumUninit` to unload trail maps and shaders
- [ ] Update `PhysarumResize` to recreate trail maps at new resolution
- [ ] Build and verify initialization succeeds

### Phase 3: New Shaders

Create the three new fragment shaders.

- [ ] Create `shaders/physarum_diffuse_h.fs` - horizontal R32F blur
- [ ] Create `shaders/physarum_diffuse_v.fs` - vertical R32F blur with decay
- [ ] Create `shaders/physarum_boost.fs` - multiplicative compositing
- [ ] Verify all shaders compile during `PhysarumInit`
- [ ] Build and verify shader loading succeeds

### Phase 4: Compute Shader Rewrite

Update agent shader for brightness-only operation.

- [ ] Remove `hue` from Agent struct in `shaders/physarum_agents.glsl`
- [ ] Change image bindings to dual R32F (read binding 1, write binding 2)
- [ ] Remove all HSV/RGB conversion functions
- [ ] Simplify `sampleTrailScore` to return raw `.r` brightness
- [ ] Update deposit to write scalar brightness with soft cap
- [ ] Remove saturation/value uniforms
- [ ] Update uniform locations in `PhysarumInit`
- [ ] Build and verify shader compiles

### Phase 5: PhysarumUpdate Refactor

Implement trail map ping-pong and internal diffusion.

- [ ] Update `PhysarumUpdate` signature to remove `RenderTexture2D* target`
- [ ] Implement dual image binding (read map, write map)
- [ ] Add trail map swap after compute dispatch
- [ ] Implement diffusion pass: horizontal blur → vertical blur + decay
- [ ] Remove color uniform setting (saturation/value)
- [ ] Build and verify physarum updates trail map internally

### Phase 6: PhysarumApplyBoost Implementation

Add the multiplicative compositing function.

- [ ] Add `PhysarumApplyBoost` declaration to `src/render/physarum.h`
- [ ] Implement dual texture binding (scene as texture0, trail as texture1)
- [ ] Set boost intensity uniform
- [ ] Render fullscreen quad to output texture
- [ ] Handle disabled/unsupported case (pass through)
- [ ] Build and verify function compiles

### Phase 7: Pipeline Integration

Wire everything into the post-effect pipeline.

- [ ] Update `ApplyPhysarumPass` to call `PhysarumUpdate` (no target param)
- [ ] Add `PhysarumApplyBoost` call after blur pass in `PostEffectBeginAccum`
- [ ] Verify pipeline order: physarum update → scene effects → boost → waveforms
- [ ] Build and test full pipeline

### Phase 8: UI and Config

Update controls and serialization.

- [ ] Remove color mode controls from physarum UI panel
- [ ] Add boost intensity slider (0.5-5.0, label "P.Boost")
- [ ] Add trail half-life slider (0.1-2.0, label "P.Trail")
- [ ] Update preset.cpp JSON save/load for new keys
- [ ] Handle legacy presets gracefully (default values for missing keys)
- [ ] Build and verify UI controls work

## Validation

- [ ] Physarum agents create visible trails on trail map (debug view if needed)
- [ ] Trails are invisible on black background (no white accumulation)
- [ ] Waveforms passing through trail regions appear brighter
- [ ] Boost intensity slider produces visible range from subtle (0.5) to dramatic (5.0)
- [ ] Trail half-life slider affects how long trails persist
- [ ] All other effects (voronoi, feedback, kaleidoscope) work correctly with physarum
- [ ] Preset save/load preserves new parameters
- [ ] Frame time at 1080p with 100K agents remains under 16ms
- [ ] Disabling physarum passes scene through unchanged

## References

- `docs/plans/physarum-scene-sampling.md` - Failed approach (color sampling caused white accumulation)
- `docs/research/physarum-simulation.md` - Jeff Jones algorithm background
- `shaders/voronoi.fs:74` - Multiplicative boost pattern to replicate
- `shaders/blur_v.fs:38-44` - Exponential decay formula
