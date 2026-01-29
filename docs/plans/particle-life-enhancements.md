# Particle Life Enhancements

Three improvements to the particle life simulation: auto-evolving attraction matrix for ever-changing emergent behavior, soft boundary repulsion instead of hard reflection, and symmetric force toggle for different dynamics.

## Current State

- `src/simulation/particle_life.h:22-50` - Config struct with physics parameters
- `src/simulation/particle_life.cpp:31-40` - Matrix generation from seed (regenerated every frame)
- `src/simulation/particle_life.cpp:262-265` - Matrix upload to shader
- `shaders/particle_life_agents.glsl:143-151` - Spherical reflective boundary
- `src/ui/imgui_effects.cpp:559-610` - UI panel

## Technical Implementation

### Auto-Evolving Matrix

Store persistent matrix in `ParticleLife` struct. Each frame, if evolution enabled:

```cpp
// Random walk on each matrix value
for (int i = 0; i < speciesCount * speciesCount; i++) {
    float noise = (HashFloat(frameCounter + i) - 0.5f) * 2.0f;  // [-1, 1]
    matrix[i] += noise * evolutionSpeed * deltaTime;
    matrix[i] = fmaxf(-1.0f, fminf(1.0f, matrix[i]));  // Clamp to valid range
}
```

When `symmetricForces` enabled, after mutating `matrix[A*stride + B]`, copy to `matrix[B*stride + A]`.

When seed changes, regenerate matrix from seed into stored array.

### Soft Boundary Repulsion

Replace hard reflection with exponential force toward origin:

```glsl
// Soft boundary: exponential repulsion near edge
float dist = length(pos);
if (dist > boundsRadius * 0.8) {  // Start repulsion at 80% of boundary
    float overshoot = (dist - boundsRadius * 0.8) / (boundsRadius * 0.2);
    float repulsionMag = boundaryStiffness * exp(overshoot * 3.0) - boundaryStiffness;
    vec3 normal = pos / dist;
    totalForce -= normal * repulsionMag;
}

// Safety clamp at 110% boundary
if (dist > boundsRadius * 1.1) {
    pos = normalize(pos) * boundsRadius * 1.1;
}
```

Parameters:
- `boundaryStiffness`: 0.1-5.0, default 1.0 - strength of repulsion force

### Symmetric Forces

When enabled, matrix generation enforces `matrix[A][B] == matrix[B][A]`:

```cpp
// After generating matrix[from][to], copy to matrix[to][from]
for (int from = 0; from < speciesCount; from++) {
    for (int to = from + 1; to < speciesCount; to++) {
        matrix[to * MAX_SPECIES + from] = matrix[from * MAX_SPECIES + to];
    }
}
```

---

## Phase 1: Config and Persistent Matrix Storage

**Goal**: Store attraction matrix persistently instead of regenerating each frame.
**Files**: `src/simulation/particle_life.h`, `src/simulation/particle_life.cpp`

**Build**:
- Add to `ParticleLifeConfig`:
  - `float evolutionSpeed = 0.0f` (0-2.0, units: magnitude per second)
  - `bool symmetricForces = false`
- Add to `ParticleLife` struct:
  - `float attractionMatrix[256]` (persistent storage, MAX_SPECIES=16)
  - `int lastSeed` (track seed changes)
  - `unsigned int evolutionFrameCounter` (for hash variation)
- Add function: `static void RegenerateMatrix(ParticleLife* pl)` - generates from seed into stored array, applies symmetric constraint if enabled
- In `ParticleLifeInit()`: call `RegenerateMatrix()` after struct initialization
- In `ParticleLifeUpdate()`:
  - If `config.attractionSeed != lastSeed`, call `RegenerateMatrix()` and update `lastSeed`
  - Upload `pl->attractionMatrix` instead of local array
- In `ParticleLifeApplyConfig()`: if seed or symmetricForces changed, call `RegenerateMatrix()`

**Verify**: `cmake.exe --build build` compiles. Run app, particle life behaves identically (matrix still deterministic from seed).

**Done when**: Matrix stored in struct, uploaded from stored values, seed changes regenerate matrix.

---

## Phase 2: Matrix Evolution

**Goal**: Add per-frame random walk on matrix values when evolution enabled.
**Files**: `src/simulation/particle_life.cpp`

**Build**:
- In `ParticleLifeUpdate()`, after seed-change check, before matrix upload:
  ```cpp
  if (pl->config.evolutionSpeed > 0.0f) {
      for (int from = 0; from < pl->config.speciesCount; from++) {
          for (int to = 0; to < pl->config.speciesCount; to++) {
              int idx = from * MAX_SPECIES + to;
              unsigned int h = HashSeed(pl->evolutionFrameCounter * 256 + idx);
              float noise = (HashFloat(h) - 0.5f) * 2.0f;
              pl->attractionMatrix[idx] += noise * pl->config.evolutionSpeed * deltaTime;
              pl->attractionMatrix[idx] = fmaxf(-1.0f, fminf(1.0f, pl->attractionMatrix[idx]));
          }
      }
      // Enforce symmetry after evolution if enabled
      if (pl->config.symmetricForces) {
          for (int from = 0; from < pl->config.speciesCount; from++) {
              for (int to = from + 1; to < pl->config.speciesCount; to++) {
                  pl->attractionMatrix[to * MAX_SPECIES + from] = pl->attractionMatrix[from * MAX_SPECIES + to];
              }
          }
      }
      pl->evolutionFrameCounter++;
  }
  ```

**Verify**: Run app. Set evolution speed > 0. Observe particle behaviors gradually changing over time. Set speed = 0, behavior stabilizes.

**Done when**: Matrix values drift when evolution enabled, stay static when disabled.

---

## Phase 3: Soft Boundary in Shader

**Goal**: Replace hard reflection with exponential repulsion force.
**Files**: `shaders/particle_life_agents.glsl`, `src/simulation/particle_life.cpp`, `src/simulation/particle_life.h`

**Build**:
- Add uniform to shader: `uniform float boundaryStiffness;`
- Add to `ParticleLifeConfig`: `float boundaryStiffness = 1.0f` (range 0.1-5.0)
- Add uniform location to `ParticleLife` struct and `LoadComputeProgram()`
- In `ParticleLifeUpdate()`: upload `boundaryStiffness` uniform
- In shader, replace the spherical boundary section (lines ~143-151):
  ```glsl
  // Soft boundary: exponential repulsion starting at 80% radius
  float dist = length(pos);
  float softStart = boundsRadius * 0.8;
  if (dist > softStart) {
      float overshoot = (dist - softStart) / (boundsRadius * 0.2);
      float repulsionMag = boundaryStiffness * (exp(overshoot * 3.0) - 1.0);
      vec3 normal = pos / dist;
      totalForce -= normal * repulsionMag;
  }

  // After position update, safety clamp at 110%
  dist = length(pos);
  if (dist > boundsRadius * 1.1) {
      pos = normalize(pos) * boundsRadius * 1.1;
      // Also dampen outward velocity
      vec3 normal = pos / length(pos);
      float outwardVel = dot(vel, normal);
      if (outwardVel > 0.0) {
          vel -= normal * outwardVel;
      }
  }
  ```
- Remove the old reflective boundary code

**Verify**: Run app. Particles should smoothly slow down and push back near boundary instead of bouncing. Increase boundaryStiffness to see stronger repulsion.

**Done when**: Particles exhibit soft boundary behavior, no hard reflections.

---

<!-- CHECKPOINT: Core features working, no UI yet -->

## Phase 4: UI Controls

**Goal**: Add sliders and toggles for new parameters.
**Files**: `src/ui/imgui_effects.cpp`

**Build**:
- In the Particle Life section (around line 574), add after the existing "Physics" controls:
  ```cpp
  ImGui::Checkbox("Symmetric##plife", &e->particleLife.symmetricForces);
  ModulatableSlider("Evolve##plife", &e->particleLife.evolutionSpeed,
                    "particleLife.evolutionSpeed", "%.2f", modSources);
  ImGui::SliderFloat("Boundary Stiffness##plife", &e->particleLife.boundaryStiffness, 0.1f, 5.0f, "%.2f");
  ```
- Place Symmetric checkbox near Species section (since it affects species interactions)
- Place Evolve slider near Seed (since it relates to matrix behavior)
- Place Boundary Stiffness near Bounds slider

**Verify**: Run app. All three new controls appear and function. Symmetric toggle regenerates matrix visibly. Evolution slider causes gradual change. Stiffness affects how particles bounce at edge.

**Done when**: All three controls visible and functional in UI.

---

## Phase 5: Param Registration

**Goal**: Register evolutionSpeed for audio modulation.
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add to `PARAM_TABLE`:
  ```cpp
  {"particleLife.evolutionSpeed", 0.0f, 2.0f, 0.0f},
  ```
- Add to `ParamRegistryInit()`:
  ```cpp
  RegisterParam("particleLife.evolutionSpeed", &e->particleLife.evolutionSpeed);
  ```

**Verify**: Run app. Open LFO panel, evolution speed appears in parameter dropdown. Route an LFO to it, observe matrix evolution speed pulsing.

**Done when**: evolutionSpeed modulatable via LFO.

---

## Phase 6: Serialization

**Goal**: Save/load new config fields in presets.
**Files**: `src/config/preset.cpp`

**Build**:
- In `SerializeParticleLife()` (or equivalent), add:
  ```cpp
  j["evolutionSpeed"] = config.evolutionSpeed;
  j["symmetricForces"] = config.symmetricForces;
  j["boundaryStiffness"] = config.boundaryStiffness;
  ```
- In `DeserializeParticleLife()`, add with defaults:
  ```cpp
  config.evolutionSpeed = j.value("evolutionSpeed", 0.0f);
  config.symmetricForces = j.value("symmetricForces", false);
  config.boundaryStiffness = j.value("boundaryStiffness", 1.0f);
  ```

**Verify**: Save a preset with non-default values. Reload it. Values persist correctly.

**Done when**: All three new parameters serialize/deserialize.

---

<!-- CHECKPOINT: Feature complete, ready for testing -->
