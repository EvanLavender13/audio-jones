# Physarum Trail Diffusion and Decay

Add diffusion (blur) and decay (fade) to the physarum trail map, creating organic flowing patterns characteristic of real physarum simulations.

## Goal

Trail maps in physarum simulations require two key behaviors:
1. **Diffusion**: Trails spread to neighboring cells over time (blur effect)
2. **Decay**: Trails fade over time (prevents saturation)

Without these, trails accumulate indefinitely and saturate at 1.0, losing the dynamic feedback loop that drives interesting agent behavior. This implements the Jones 2010 algorithm's trail processing step.

## Current State

The trail map exists but lacks processing:

- `src/render/physarum.h:27` - Trail map is `RenderTexture2D` with R32F format
- `src/render/physarum.cpp:36` - Created with `RL_PIXELFORMAT_UNCOMPRESSED_R32`
- `shaders/physarum_agents.glsl:93-95` - Agents deposit trails additively, saturating at 1.0
- `src/render/physarum.h:21` - Config has `depositAmount` but no decay/diffusion params

Current behavior: trails accumulate forever until reset.

## Algorithm

### Diffusion (Separable 5-tap Gaussian)

Separable blur executes in two passes for efficiency:

| Pass | Direction | Kernel |
|------|-----------|--------|
| H | Horizontal | `[0.0625, 0.25, 0.375, 0.25, 0.0625]` |
| V | Vertical | Same weights |

The kernel sums to 1.0 (energy-preserving). Separable reduces samples from 25 (5x5) to 10 (5+5).

Edge behavior: **wrap** via `mod(coord, resolution)` to match agent wrapping.

### Decay (Framerate-Independent Half-Life)

Exponential decay ensures consistent behavior across framerates:

```glsl
float decayFactor = exp(-0.693147 * deltaTime / halfLife);
intensity *= decayFactor;
```

| Parameter | Formula | Example |
|-----------|---------|---------|
| Half-life = 1.0s | After 1s, intensity = 50% | Slow fade |
| Half-life = 0.1s | After 0.1s, intensity = 50% | Fast fade |

The constant 0.693147 is `ln(2)`, derived from the half-life equation.

### Processing Order

```
Per frame:
1. PhysarumUpdate() - agents sense and deposit
2. Trail H-pass - horizontal diffusion (trailMap → trailMapTemp)
3. Trail V-pass - vertical diffusion + decay (trailMapTemp → trailMap)
```

Decay applies in V-pass only (once per frame).

## Architecture

### Binding Swap Pattern (No Shader Branching)

Single compute shader with two image bindings:
- `binding = 1`: input (read)
- `binding = 2`: output (write)

C++ swaps which texture is bound to which binding between passes:

```
H-pass: bind trailMap→1, trailMapTemp→2, dispatch
barrier
V-pass: bind trailMapTemp→1, trailMap→2, dispatch
```

This avoids shader conditionals while using one shader program.

## Integration

### Physarum Struct Changes (`src/render/physarum.h`)

Add to `Physarum` struct after line 27:
- `RenderTexture2D trailMapTemp;` - Ping-pong texture
- `unsigned int trailProgram;` - Trail processing compute shader
- `int trailResolutionLoc;`
- `int trailDiffusionScaleLoc;`
- `int trailDecayFactorLoc;`

### Config Changes (`src/render/physarum.h`)

Add to `PhysarumConfig` after line 21:
- `float decayHalfLife = 0.5f;` - Seconds for 50% decay (0.1-5.0 range)
- `int diffusionScale = 1;` - Diffusion kernel scale in pixels (0-4 range)

### New Shader (`shaders/physarum_trail.glsl`)

```glsl
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(r32f, binding = 1) readonly uniform image2D inputMap;
layout(r32f, binding = 2) writeonly uniform image2D outputMap;

uniform vec2 resolution;
uniform int diffusionScale;
uniform float decayFactor;  // Pre-computed: exp(-0.693147 * dt / halfLife)
uniform int applyDecay;     // 1 for V-pass, 0 for H-pass
uniform int direction;      // 0 = horizontal, 1 = vertical

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    if (coord.x >= int(resolution.x) || coord.y >= int(resolution.y)) {
        return;
    }

    float result;
    if (diffusionScale == 0) {
        result = imageLoad(inputMap, coord).r;
    } else {
        // 5-tap Gaussian with wrapping
        ivec2 offset = (direction == 0) ? ivec2(1, 0) : ivec2(0, 1);
        ivec2 res = ivec2(resolution);

        result = imageLoad(inputMap, (coord - 2 * offset * diffusionScale + res) % res).r * 0.0625
               + imageLoad(inputMap, (coord - 1 * offset * diffusionScale + res) % res).r * 0.25
               + imageLoad(inputMap, coord).r * 0.375
               + imageLoad(inputMap, (coord + 1 * offset * diffusionScale) % res).r * 0.25
               + imageLoad(inputMap, (coord + 2 * offset * diffusionScale) % res).r * 0.0625;
    }

    if (applyDecay == 1) {
        result *= decayFactor;
    }

    imageStore(outputMap, coord, vec4(result, 0.0, 0.0, 0.0));
}
```

### Trail Processing Function (`src/render/physarum.cpp`)

Add after `PhysarumUpdate()`:

```cpp
void PhysarumProcessTrails(Physarum* p, float deltaTime)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }

    // Pre-compute decay factor
    float safeHalfLife = fmaxf(p->config.decayHalfLife, 0.001f);
    float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);

    rlEnableShader(p->trailProgram);

    float resolution[2] = { (float)p->width, (float)p->height };
    rlSetUniform(p->trailResolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(p->trailDiffusionScaleLoc, &p->config.diffusionScale, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(p->trailDecayFactorLoc, &decayFactor, RL_SHADER_UNIFORM_FLOAT, 1);

    int workGroupsX = (p->width + 15) / 16;
    int workGroupsY = (p->height + 15) / 16;

    // H-pass: trailMap → trailMapTemp
    int direction = 0;
    int applyDecay = 0;
    rlSetUniform(rlGetLocationUniform(p->trailProgram, "direction"), &direction, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(rlGetLocationUniform(p->trailProgram, "applyDecay"), &applyDecay, RL_SHADER_UNIFORM_INT, 1);
    rlBindImageTexture(p->trailMap.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, true);  // read
    rlBindImageTexture(p->trailMapTemp.texture.id, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32, false); // write
    rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // V-pass: trailMapTemp → trailMap (with decay)
    direction = 1;
    applyDecay = 1;
    rlSetUniform(rlGetLocationUniform(p->trailProgram, "direction"), &direction, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(rlGetLocationUniform(p->trailProgram, "applyDecay"), &applyDecay, RL_SHADER_UNIFORM_INT, 1);
    rlBindImageTexture(p->trailMapTemp.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, true);  // read
    rlBindImageTexture(p->trailMap.texture.id, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32, false); // write
    rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}
```

### Call Site (`src/render/post_effect.cpp`)

Modify `ApplyPhysarumPass()` at line 127:

```cpp
static void ApplyPhysarumPass(PostEffect* pe, float deltaTime)
{
    if (pe->physarum == NULL || !pe->effects.physarum.enabled) {
        return;
    }

    PhysarumApplyConfig(pe->physarum, &pe->effects.physarum);
    PhysarumUpdate(pe->physarum, deltaTime);
    PhysarumProcessTrails(pe->physarum, deltaTime);  // NEW

    BeginTextureMode(pe->accumTexture);
    PhysarumDrawDebug(pe->physarum);
    EndTextureMode();
}
```

### UI Controls (`src/ui/ui_panel_effects.cpp`)

Add after existing physarum sliders (line 48):

```cpp
UISliderFloat(&rect, "Decay Half-life", &effects.physarum.decayHalfLife, 0.1f, 5.0f, rect.y += spacing);
UISliderInt(&rect, "Diffusion Scale", &effects.physarum.diffusionScale, 0, 4, rect.y += spacing);
```

## File Changes

| File | Change |
|------|--------|
| `src/render/physarum.h` | Add `trailMapTemp`, trail shader program, uniform locs, config params |
| `src/render/physarum.cpp` | Create temp texture, load trail shader, implement `PhysarumProcessTrails()` |
| `shaders/physarum_trail.glsl` | Create - diffusion + decay compute shader |
| `src/render/post_effect.cpp` | Call `PhysarumProcessTrails()` in `ApplyPhysarumPass()` |
| `src/ui/ui_panel_effects.cpp` | Add decay and diffusion sliders |

## Build Sequence

### Phase 1: Core Infrastructure
1. Add `trailMapTemp` texture to `Physarum` struct
2. Initialize/cleanup temp texture in `PhysarumInit()`/`PhysarumUninit()`
3. Handle resize in `PhysarumResize()`
4. Verify temp texture creation with `TraceLog()`

### Phase 2: Trail Processing Shader
1. Create `shaders/physarum_trail.glsl`
2. Load shader in `PhysarumInit()` (after agent shader)
3. Cache uniform locations
4. Implement `PhysarumProcessTrails()` with both passes

### Phase 3: Integration and Config
1. Add `decayHalfLife` and `diffusionScale` to `PhysarumConfig`
2. Call `PhysarumProcessTrails()` from `ApplyPhysarumPass()`
3. Add UI sliders

### Phase 4: Tuning and Validation
1. Test decay behavior (trails should fade smoothly)
2. Test diffusion behavior (trails should spread)
3. Verify edge wrapping works correctly
4. Tune default values for good visual results

## Validation

- [ ] Trails fade over time when decay half-life < 5.0s
- [ ] Trails spread outward when diffusion scale > 0
- [ ] Trails wrap at screen edges (no hard cutoff)
- [ ] Framerate changes don't affect decay speed (test at 30fps and 60fps)
- [ ] Diffusion scale 0 produces no blur (sharp trails)
- [ ] UI sliders control both parameters in real-time
- [ ] No visual artifacts at texture boundaries
- [ ] Performance: <1ms overhead at 1080p (measure with RenderDoc or similar)

## References

- Jones, J. (2010). "Characteristics of pattern formation and evolution in approximations of Physarum transport networks"
- Existing blur implementation: `shaders/blur_v.fs:38-40` (half-life decay formula)
