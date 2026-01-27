# Particle Life

Emergent multi-species particle simulation: NxN attraction matrix defines species interactions (attraction/repulsion), producing clusters, chasers, orbiters, and territorial competition. 3D positions with orthographic projection following Attractor Flow's pattern.

## Current State

Relevant existing code to hook into:
- `src/simulation/attractor_flow.h:19-58` - 3D agent struct, rotation config pattern
- `src/simulation/attractor_flow.cpp:234-251` - CPU rotation matrix computation
- `shaders/attractor_agents.glsl:145-179` - 3D projection to screen
- `src/simulation/trail_map.h` - Shared trail infrastructure
- `src/render/render_pipeline.cpp:130-149` - Simulation pass pattern
- `src/render/shader_setup.cpp:292-298` - Trail boost setup pattern
- `src/automation/param_registry.cpp:81-86` - Rotation param registration

Research document: `docs/research/particle-life.md`

## Design Decisions

- **Neighbor queries**: Brute-force O(N²) for initial 10K particles
- **Attraction matrix**: GPU uniform array (max 64 floats for N=8 species)
- **Matrix UI**: Seed-only randomization (no grid editor)
- **LFO modulation**: Minimal (rotation speeds only: rotationSpeedX/Y/Z)

## Technical Implementation

**Source**: `docs/research/particle-life.md`, [lisyarus WebGPU implementation](https://lisyarus.github.io/blog/posts/particle-life-simulation-in-browser-using-webgpu.html)

### Force Model

Two force zones within interaction radius R_MAX:

**Inner repulsion (r < beta × R_MAX):**
```glsl
force = r / beta - 1.0;  // -1 at r=0, approaches 0 at r=beta
```

**Interaction zone (beta × R_MAX < r < R_MAX):**
```glsl
force = a * (1.0 - abs(2.0 * r - 1.0 - beta) / (1.0 - beta));
```

Where `a ∈ [-1, 1]` is the attraction matrix value for the species pair.

**Complete force function:**
```glsl
float force(float r, float a, float beta) {
    if (r < beta) {
        return r / beta - 1.0;
    } else if (r < 1.0) {
        return a * (1.0 - abs(2.0 * r - 1.0 - beta) / (1.0 - beta));
    }
    return 0.0;
}
```

### Attraction Matrix Generation

Seeded random matrix (asymmetric):
```glsl
float attraction(int from, int to, int seed) {
    float v = hash(vec2(from, to) + float(seed * numSpecies));
    return v * 2.0 - 1.0;  // Map [0,1] to [-1,1]
}
```

Upload as uniform array: `uniform float attractionMatrix[64];` (max 8×8)

### Main Update Loop

```glsl
void main() {
    uint id = gl_GlobalInvocationID.x;
    Particle p = particles[id];
    vec3 pos = vec3(p.x, p.y, p.z);
    vec3 vel = vec3(p.vx, p.vy, p.vz);

    vec3 totalForce = vec3(0.0);

    // Brute-force neighbor iteration
    for (uint j = 0; j < numParticles; j++) {
        if (id == j) continue;

        Particle other = particles[j];
        vec3 delta = vec3(other.x, other.y, other.z) - pos;
        float r = length(delta);

        if (r > 0.0 && r < rMax) {
            float a = attractionMatrix[p.species * numSpecies + other.species];
            float f = force(r / rMax, a, beta);
            totalForce += (delta / r) * f;
        }
    }

    // Centering force (keeps particles from drifting)
    totalForce -= pos * centeringStrength * exp(length(pos) * centeringFalloff);

    totalForce *= rMax * forceFactor;

    // Semi-implicit Euler integration
    vel *= friction;
    vel += totalForce * timeStep;
    pos += vel * timeStep;

    // Project 3D -> 2D and deposit trail (same as Attractor Flow)
    vec3 rotated = rotationMatrix * pos;
    vec2 screenPos = vec2(rotated.x, rotated.z) * projectionScale * min(resolution.x, resolution.y);
    screenPos += center * resolution;

    // Deposit colored trail
    if (onScreen(screenPos)) {
        vec3 color = hsv2rgb(vec3(p.hue, saturation, value));
        vec4 current = imageLoad(trailMap, ivec2(screenPos));
        imageStore(trailMap, ivec2(screenPos), vec4(current.rgb + color * depositAmount, 0.0));
    }

    // Store updated state
    particles[id] = Particle(pos.x, pos.y, pos.z, vel.x, vel.y, vel.z, p.hue, p.species);
}
```

### 3D Projection

Same as Attractor Flow:
```glsl
vec2 projectToScreen(vec3 pos) {
    vec3 rotated = rotationMatrix * pos;  // CPU-computed 3x3 rotation matrix
    vec2 projected = vec2(rotated.x, rotated.z);  // Drop Y (depth)
    return projected * projectionScale * min(resolution.x, resolution.y) + center * resolution;
}
```

### Agent Struct (32 bytes, GPU-aligned)

```glsl
struct Particle {
    float x, y, z;       // Position
    float vx, vy, vz;    // Velocity
    float hue;           // Derived from species for colored deposits
    int species;         // 0 to numSpecies-1
};
```

---

## Phase 1: Config and Header

**Goal**: Define configuration struct and runtime state.
**Depends on**: —
**Files**: `src/simulation/particle_life.h`

**Build**:
- Create `ParticleLifeConfig` struct with fields from research doc:
  - `enabled`, `agentCount`, `speciesCount`, `rMax`, `forceFactor`, `friction`, `beta`
  - `attractionSeed`, `centeringStrength`, `centeringFalloff`
  - 3D view: `x`, `y`, `rotationAngleX/Y/Z`, `rotationSpeedX/Y/Z`, `projectionScale`
  - Trail: `depositAmount`, `decayHalfLife`, `diffusionScale`, `boostIntensity`, `blendMode`
  - `ColorConfig color`, `debugOverlay`
- Create `ParticleLifeAgent` struct (32 bytes): x, y, z, vx, vy, vz, hue, species
- Create `ParticleLife` runtime struct with: agentBuffer, computeProgram, trailMap, uniform locations, rotation accumulators, config, supported flag
- Declare API: `ParticleLifeInit`, `ParticleLifeUninit`, `ParticleLifeUpdate`, `ParticleLifeProcessTrails`, `ParticleLifeApplyConfig`, `ParticleLifeReset`, `ParticleLifeResize`, `ParticleLifeDrawDebug`

**Verify**: File compiles with `#include "particle_life.h"` from another source.

**Done when**: Header defines complete config struct matching research doc parameters.

---

## Phase 2: Compute Shader

**Goal**: Implement the particle simulation GPU logic.
**Depends on**: —
**Files**: `shaders/particle_life_agents.glsl`

**Build**:
- Create compute shader with `layout(local_size_x = 1024) in`
- Define `Particle` struct matching C++ layout (32 bytes)
- Add SSBO binding 0 for particle buffer
- Add image binding 1 for trail map (rgba32f)
- Add uniforms: resolution, time, numParticles, numSpecies, rMax, forceFactor, friction, beta, centeringStrength, centeringFalloff, timeStep, center, rotationMatrix, projectionScale, depositAmount, saturation, value, attractionMatrix[64]
- Implement `force()` function from research doc
- Implement `hash()` and `hsv2rgb()` (copy from attractor_agents.glsl)
- Implement brute-force N² neighbor loop with force accumulation
- Apply centering force, velocity integration, position update
- Project to screen using rotation matrix
- Deposit colored trail with proportional scaling

**Verify**: Shader compiles with `glslangValidator shaders/particle_life_agents.glsl`.

**Done when**: Compute shader compiles and contains complete force model logic.

---

## Phase 3: Simulation Implementation

**Goal**: CPU-side simulation orchestration.
**Depends on**: Phase 1, Phase 2
**Files**: `src/simulation/particle_life.cpp`

**Build**:
- Implement `ParticleLifeSupported()` - check OpenGL 4.3
- Implement `ParticleLifeInit()`:
  - Allocate ParticleLife struct
  - Load compute shader, query uniform locations
  - Create trail map via `TrailMapInit()`
  - Create agent buffer with random 3D positions and species assignment
  - Assign hues via `ColorConfigAgentHue()` based on species
- Implement `ParticleLifeUpdate()`:
  - Accumulate rotation angles: `accum += speed * deltaTime`
  - Compute rotation matrix on CPU (copy from attractor_flow.cpp:234-251)
  - Generate attraction matrix on CPU from seed, upload as uniform array
  - Bind uniforms, dispatch compute, memory barrier
- Implement `ParticleLifeProcessTrails()` - delegate to `TrailMapProcess()`
- Implement `ParticleLifeApplyConfig()` - handle agent count changes
- Implement `ParticleLifeUninit()`, `ParticleLifeReset()`, `ParticleLifeResize()`
- Implement `ParticleLifeDrawDebug()` - draw trail map to screen

**Verify**: `cmake.exe --build build` succeeds with no errors.

**Done when**: All API functions implemented and compile.

---

## Phase 4: PostEffect Integration

**Goal**: Wire simulation into render pipeline.
**Depends on**: Phase 3
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/render/render_pipeline.cpp`, `src/render/shader_setup.h`, `src/render/shader_setup.cpp`, `src/config/effect_config.h`

**Build**:
- Add `ParticleLife* particleLife` to PostEffect struct (`post_effect.h`)
- Add `bool particleLifeBoostActive` flag
- Add `ParticleLifeConfig particleLife` to EffectConfig struct (`effect_config.h`)
- Add `TRANSFORM_PARTICLE_LIFE_BOOST` enum entry in TransformEffectType
- Add name case in `TransformEffectName()`
- Add to `TransformOrderConfig::order` array
- Add `IsTransformEnabled()` case checking boostIntensity > 0
- Initialize simulation in `PostEffectInit()` (`post_effect.cpp`)
- Free simulation in `PostEffectUninit()`
- Add `ApplyParticleLifePass()` function in `render_pipeline.cpp`
- Call it from appropriate location in render loop
- Add `SetupParticleLifeTrailBoost()` in `shader_setup.cpp`
- Add dispatch case in `GetTransformEffect()`
- Declare setup function in `shader_setup.h`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → no crash on startup, simulation can be enabled via code.

**Done when**: Simulation runs and deposits trails when enabled via config.

---

## Phase 5: UI Panel

**Goal**: Add user interface controls.
**Depends on**: Phase 4
**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_simulations.cpp`

**Build**:
- Add `GetTransformCategory()` case for `TRANSFORM_PARTICLE_LIFE_BOOST` returning category badge
- Add static section state: `static bool sectionParticleLife = false`
- Create `DrawSimulationsParticleLife()` helper function with:
  - Enabled checkbox with auto-move-to-end behavior
  - Agent Count slider (1000-20000)
  - Species Count slider (2-8)
  - Seed slider + Randomize button
  - Physics section: rMax, forceFactor, friction, beta sliders
  - Centering section: centeringStrength, centeringFalloff sliders
  - 3D View section: position, rotation angles (degrees), rotation speeds (ModulatableSlider)
  - Trail section: deposit, decay, diffusion sliders
  - Output section: boost intensity, blend mode, color config, debug checkbox
- Call helper from `DrawSimulationsCategory()`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Particle Life panel appears in Simulations category with all controls functional.

**Done when**: All UI controls modify simulation in real-time.

---

## Phase 6: Preset Serialization

**Goal**: Save/load Particle Life settings in presets.
**Depends on**: Phase 4
**Files**: `src/config/preset.cpp`

**Build**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParticleLifeConfig, ...)` macro listing all config fields
- Add `if (e.particleLife.enabled) { j["particleLife"] = e.particleLife; }` in `to_json()`
- Add `e.particleLife = j.value("particleLife", e.particleLife);` in `from_json()`

**Verify**: Save preset with Particle Life enabled, reload → settings preserved.

**Done when**: Presets correctly serialize and deserialize Particle Life config.

---

## Phase 7: Parameter Registration

**Goal**: Enable LFO modulation of physics and rotation parameters.
**Depends on**: Phase 4
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add to PARAM_TABLE:
  ```cpp
  {"particleLife.rotationSpeedX", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
  {"particleLife.rotationSpeedY", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
  {"particleLife.rotationSpeedZ", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
  ```
- Add to targets array at matching indices

**Verify**: Route LFO to `particleLife.rotationSpeedX` → rotation responds to modulation.

**Done when**: All rotation speed parameters respond to LFO modulation.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1, Phase 2 | — |
| 2 | Phase 3 | Wave 1 |
| 3 | Phase 4 | Wave 2 |
| 4 | Phase 5, Phase 6, Phase 7 | Wave 3 |

Phases in the same wave execute as parallel subagents. Each wave completes before the next begins.

---

## Post-Implementation Notes

Changes made after testing that extend beyond the original plan:

### Replaced: Centering force with reflective bounds (2026-01-26)

**Reason**: Exponential centering force caused particle collapse. The Shadertoy reference worked in normalized space (~0.1 units), but implementation used pixel space (~100+ units). Force exploded to billions.

**Changes**:
- `particle_life.h`: Removed `centeringStrength`, `centeringFalloff`. Added `boundsRadius`.
- `particle_life.cpp`: Removed centering uniform setup. Added bounds uniform.
- `particle_life_agents.glsl`: Replaced centering force with spherical reflective boundary.
- `imgui_effects.cpp`: Replaced centering sliders with bounds slider.
- `preset.cpp`: Updated serialization fields.

### Changed: Normalized simulation space (2026-01-26)

**Reason**: Pixel-space coordinates caused parameter scaling issues.

**Changes**:
- `particle_life.h`: `rMax` default 0.3 (was 50.0), `projectionScale` default 0.4 (was 0.01)
- `particle_life.cpp`: Spawn radius 0.5 (was 100.0)
- `imgui_effects.cpp`: Updated slider ranges for normalized space.

### Renamed: `friction` → `momentum` (2026-01-26)

**Reason**: Counter-intuitive naming. Higher friction meant less slowdown (velocity retention).

**Changes**:
- `particle_life.h`: Field renamed.
- `particle_life.cpp`: Uniform renamed.
- `particle_life_agents.glsl`: Uniform renamed.
- `imgui_effects.cpp`: Slider renamed.
- `preset.cpp`: Serialization field renamed.
- `param_registry.cpp`: Registry entry renamed.

### Added: Physics parameter modulation (2026-01-26)

**Reason**: User requested LFO control of physics beyond rotation speeds.

**Changes**:
- `param_registry.cpp`: Added `rMax`, `forceFactor`, `momentum`, `beta` entries.
- `imgui_effects.cpp`: Changed physics sliders to `ModulatableSlider`.

### Changed: Max species 8 → 16 (2026-01-26)

**Reason**: User requested more species variety.

**Changes**:
- `particle_life.cpp`: `MAX_SPECIES = 16`
- `particle_life_agents.glsl`: `attractionMatrix[256]`
- `imgui_effects.cpp`: Slider range 2-16.

### Rejected: Hue-binned species (2026-01-26)

**Reason**: Full rainbow with species derived from hue bins was too visually busy.

**Reverted**: Species assigned evenly by index, hue derived from species.
