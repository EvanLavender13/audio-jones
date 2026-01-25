# Lissajous Attractor Motion

Add Lissajous motion to physarum multi-home bounds attractors. Attractors follow parametric curves instead of remaining static, creating dynamic colony movement patterns.

## Current State

- `src/simulation/physarum.h:42-43` — `attractorCount` field (2-8)
- `shaders/physarum_agents.glsl:354-369` — Multi-home mode computes static attractor positions from `hash(attractorIdx)`
- `src/simulation/cymatics.cpp:127-140` — Reference pattern for Lissajous (CPU phase accumulation, uniform array)

## Technical Implementation

**Lissajous curve equations**:
```
x(t) = baseX + amplitude * sin(freqX * phase + offset)
y(t) = baseY + amplitude * cos(freqY * phase + offset)
```

Where:
- `baseX, baseY` = radial distribution around center (like cymatics)
- `amplitude` = fraction of screen (0.0-0.5)
- `freqX, freqY` = oscillation frequencies (Hz)
- `phase` = CPU-accumulated time (radians)
- `offset` = per-attractor phase offset (`TWO_PI * i / count`)

**Parameters**:
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `lissajousAmplitude` | float | 0.0-0.5 | 0.1 |
| `lissajousFreqX` | float | 0.01-1.0 | 0.05 |
| `lissajousFreqY` | float | 0.01-1.0 | 0.08 |
| `lissajousBaseRadius` | float | 0.0-0.5 | 0.3 |

**CPU computation** (in `PhysarumUpdate`):
```cpp
const float TWO_PI = 6.28318530718f;
p->lissajousPhase += deltaTime * TWO_PI;

float attractors[16];  // 8 attractors * 2 components
const float amp = cfg->lissajousAmplitude;
const float phaseX = p->lissajousPhase * cfg->lissajousFreqX;
const float phaseY = p->lissajousPhase * cfg->lissajousFreqY;
const int count = cfg->attractorCount;

for (int i = 0; i < count; i++) {
    const float angle = TWO_PI * (float)i / (float)count;
    const float baseX = 0.5f + cfg->lissajousBaseRadius * cosf(angle);
    const float baseY = 0.5f + cfg->lissajousBaseRadius * sinf(angle);
    const float offset = (float)i / (float)count * TWO_PI;
    attractors[i * 2 + 0] = baseX + amp * sinf(phaseX + offset);
    attractors[i * 2 + 1] = baseY + amp * cosf(phaseY + offset);
}
```

**Shader change**: Replace hash-based computation with uniform lookup:
```glsl
uniform vec2 attractors[8];  // Normalized 0-1 positions

// In boundsMode == 8 block:
uint attractorIdx = hash(id) % uint(attractorCount);
vec2 target = attractors[attractorIdx] * resolution;
```

---

## Phase 1: Config + CPU State

**Goal**: Add Lissajous parameters to config and phase accumulator to runtime state.
**Depends on**: —
**Files**: `src/simulation/physarum.h`

**Build**:
- Add four float fields to `PhysarumConfig`: `lissajousAmplitude`, `lissajousFreqX`, `lissajousFreqY`, `lissajousBaseRadius` with defaults from table above
- Add `float lissajousPhase` field to `Physarum` struct (runtime accumulator)
- Add `int attractorsLoc` uniform location to `Physarum` struct

**Verify**: `cmake.exe --build build` compiles without errors.

**Done when**: Config struct contains all four Lissajous parameters and Physarum struct has phase accumulator.

---

## Phase 2: Shader Uniform

**Goal**: Modify shader to receive attractor positions from CPU.
**Depends on**: —
**Files**: `shaders/physarum_agents.glsl`

**Build**:
- Add `uniform vec2 attractors[8]` declaration near other uniforms
- Replace boundsMode == 8 block: remove hash-based position computation, use `attractors[attractorIdx] * resolution` instead

**Verify**: `cmake.exe --build build` compiles shader without errors.

**Done when**: Shader reads attractor positions from uniform array.

---

## Phase 3: CPU Computation + Upload

**Goal**: Compute Lissajous positions on CPU and upload to shader.
**Depends on**: Phase 1, Phase 2
**Files**: `src/simulation/physarum.cpp`

**Build**:
- In `LoadComputeProgram`: get `attractorsLoc` uniform location
- In `PhysarumUpdate`: accumulate `lissajousPhase`, compute attractor positions array using formulas from Technical Implementation, upload via `glUniform2fv`
- Initialize `lissajousPhase = 0.0f` in `PhysarumInit`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → enable physarum, set bounds mode to Multi-Home → attractors move in Lissajous patterns.

**Done when**: Attractors visibly follow Lissajous curves.

---

## Phase 4: UI + Serialization

**Goal**: Expose Lissajous parameters in UI and persist in presets.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects.cpp`, `src/config/preset.cpp`

**Build**:
- In `imgui_effects.cpp` inside multi-home bounds section (after attractor count slider): add sliders for amplitude, freqX, freqY, baseRadius
- In `preset.cpp`: add four fields to `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysarumConfig, ...)`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → UI sliders appear when Multi-Home selected → save/load preset preserves values.

**Done when**: UI controls work and presets serialize correctly.

---

## Phase 5: Parameter Registration

**Goal**: Enable modulation routing to Lissajous parameters.
**Depends on**: Phase 1
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add four entries to `PARAM_TABLE`:
  - `{"physarum.lissajousAmplitude", {0.0f, 0.5f}}`
  - `{"physarum.lissajousFreqX", {0.01f, 1.0f}}`
  - `{"physarum.lissajousFreqY", {0.01f, 1.0f}}`
  - `{"physarum.lissajousBaseRadius", {0.0f, 0.5f}}`
- Add corresponding entries to `targets` array at matching indices

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → assign LFO to `physarum.lissajousAmplitude` → amplitude modulates.

**Done when**: All four parameters respond to modulation.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1, Phase 2 | — |
| 2 | Phase 3, Phase 4, Phase 5 | Wave 1 |

Phases 1 and 2 are independent (config vs shader). Phases 3, 4, 5 all depend on Phase 1 (config) but don't share files, so they can run in parallel.
