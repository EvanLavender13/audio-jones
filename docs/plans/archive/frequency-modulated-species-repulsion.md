# Frequency-Modulated Species Repulsion

Extend physarum simulation so agents repel different-hue trails with strength modulated by FFT energy. Each agent's position in the color spectrum (0-1) maps to a frequency band (log scale, 20Hz-20kHz). When that frequency is loud, agents become more aggressive/repulsive toward different species. The spectrum maps directly onto the color distribution - first color = bass, last color = treble - regardless of actual hue values.

## Current State

- `src/render/physarum.h:8` - `PhysarumAgent` struct (16 bytes: x, y, heading, hue)
- `src/render/physarum.h:15` - `PhysarumConfig` struct with existing parameters
- `src/render/physarum.h:31` - `Physarum` struct with uniform locations
- `src/render/physarum.cpp:52` - `InitializeAgents()` assigns hue based on ColorConfig
- `src/render/physarum.cpp:294` - `PhysarumUpdate()` binds textures at units 1-2
- `src/render/post_effect.cpp:150` - calls `PhysarumUpdate()` with `accumTexture`
- `src/analysis/fft.h:17` - `FFTProcessor.magnitude[1025]` contains FFT data
- `src/main.cpp:186` - `PostEffectBeginAccum()` call site
- `shaders/physarum_agents.glsl:7` - Agent struct in shader (matches CPU struct)
- `shaders/physarum_agents.glsl:76` - `computeAffinity()` computes hue-based attraction only

---

## Phase 1: FFT Texture Infrastructure

**Goal**: Create 1D texture for FFT data in PostEffect and wire up upload pipeline.

**Build**:

1. **`src/render/post_effect.h`** - Add FFT texture fields to PostEffect struct:
   - `Texture2D fftTexture` - 1D texture (1025x1) for normalized FFT magnitudes
   - `float fftMaxMagnitude` - Running max for auto-normalization

2. **`src/render/post_effect.h`** - Update `PostEffectBeginAccum()` signature:
   - Add parameter: `const float* fftMagnitude`

3. **`src/render/post_effect.cpp`** - Create FFT texture in `PostEffectInit()`:
   - Allocate 1025x1 R32F texture with bilinear filter, clamp wrap
   - Initialize `fftMaxMagnitude = 1.0f`

4. **`src/render/post_effect.cpp`** - Add `UpdateFFTTexture()` static helper:
   - Find max magnitude in array
   - Track running max with 0.99 decay factor (smooth normalization)
   - Normalize values to 0-1 range
   - Upload via `UpdateTexture()`

5. **`src/render/post_effect.cpp`** - Call `UpdateFFTTexture()` in `PostEffectBeginAccum()`

6. **`src/render/post_effect.cpp`** - Clean up texture in `PostEffectUninit()`

7. **`src/main.cpp:186`** - Update call site to pass `ctx->analysis.fft.magnitude`

**Done when**: App builds and runs without crashes. FFT texture created (verify via TraceLog or debugger).

---

## Phase 2: Agent Struct and Physarum FFT Binding

**Goal**: Extend agent struct with spectrum position, pass FFT texture to shader, add config parameters.

**Build**:

1. **`src/render/physarum.h`** - Extend `PhysarumAgent` struct (16 → 32 bytes):
   ```c
   typedef struct PhysarumAgent {
       float x;
       float y;
       float heading;
       float hue;         // For deposit color and affinity
       float spectrumPos; // Position in color distribution (0-1) for FFT lookup
       float _pad[3];     // Pad to 32 bytes for GPU alignment
   } PhysarumAgent;
   ```

2. **`src/render/physarum.cpp`** - Update `InitializeAgents()`:
   - For all color modes, set `spectrumPos = (float)i / (float)agentCount`
   - This is the normalized position used to sample the color distribution

3. **`src/render/physarum.h`** - Add to `PhysarumConfig`:
   - `float frequencyModulation = 0.0f` - FFT repulsion strength (0-1)
   - `bool spectrumLogScale = true` - Log vs linear frequency mapping

4. **`src/render/physarum.h`** - Add to `Physarum` struct:
   - `int frequencyModulationLoc` - Uniform location

5. **`src/render/physarum.h`** - Update `PhysarumUpdate()` signature:
   - Add parameter: `Texture2D fftTexture`

6. **`src/render/physarum.cpp`** - In `LoadComputeProgram()`:
   - Get uniform location for `frequencyModulation`

7. **`src/render/physarum.cpp`** - In `PhysarumUpdate()`:
   - Accept new `fftTexture` parameter
   - Set `frequencyModulation` uniform
   - Bind FFT texture to GL_TEXTURE3

8. **`src/render/post_effect.cpp:150`** - Update `PhysarumUpdate()` call:
   - Pass `pe->fftTexture` as new parameter

**Done when**: Build succeeds. Physarum runs unchanged with `frequencyModulation = 0` (default). Agents now carry `spectrumPos` field.

---

## Phase 3: Shader FFT Modulation

**Goal**: Implement frequency-modulated affinity calculation in compute shader.

**Build**:

1. **`shaders/physarum_agents.glsl`** - Update Agent struct to match CPU:
   ```glsl
   struct Agent {
       float x;
       float y;
       float heading;
       float hue;
       float spectrumPos;  // Position in color distribution for FFT lookup
       float _pad1;
       float _pad2;
       float _pad3;
   };
   ```

2. **`shaders/physarum_agents.glsl`** - Add uniforms after existing uniforms:
   ```glsl
   layout(binding = 3) uniform sampler2D fftMap;
   uniform float frequencyModulation;
   ```

3. **`shaders/physarum_agents.glsl`** - Add `sampleFFT()` helper before `computeAffinity()`:
   - Map spectrum position (0-1) to frequency using log scale (20Hz-20kHz)
   - Convert frequency to FFT bin index
   - Sample 1D texture at normalized bin position
   - Return magnitude (0-1)

4. **`shaders/physarum_agents.glsl`** - Modify `computeAffinity()` signature and body:
   - Add `float agentSpectrumPos` parameter
   - Sample FFT energy at agent's spectrum position (not hue!)
   - Compute modulation: `1.0 + fftEnergy * frequencyModulation * 2.0`
   - Apply: `modulatedHueDiff = hueDiff * modulation`
   - Return: `modulatedHueDiff + (1.0 - intensity) * 0.3`

5. **`shaders/physarum_agents.glsl`** - Update call sites of `computeAffinity()`:
   - Pass `agent.spectrumPos` as additional argument

**Done when**: Enable physarum, set `frequencyModulation = 0.5` (hardcoded for test), play music. Agents at the start of the color gradient should repel more strongly when bass is loud; agents at the end should repel when treble is loud.

---

## Phase 4: Config, UI, and Presets

**Goal**: Expose parameters to users and persist in presets.

**Build**:

1. **`src/config/preset.cpp:75`** - Update `PhysarumConfig` serialization macro:
   - Add `frequencyModulation, spectrumLogScale` to field list

2. **`src/ui/imgui_effects.cpp`** - Add UI controls after line 74 (before Debug checkbox):
   - `ImGui::SliderFloat("Freq Mod", &e->physarum.frequencyModulation, 0.0f, 1.0f)`
   - Conditional checkbox for log scale when frequencyModulation > 0

**Done when**:
- Slider controls repulsion strength in real-time
- Save preset with `frequencyModulation = 0.7`, reload, value persists
- Toggle works and affects behavior

---

## Data Flow Summary

```
Audio (48kHz) → FFTProcessor.magnitude[1025] (CPU)
       ↓
PostEffectBeginAccum() receives magnitude pointer
       ↓
UpdateFFTTexture() normalizes (running max decay) → uploads to GPU
       ↓
PhysarumUpdate() binds fftTexture at GL_TEXTURE3
       ↓
physarum_agents.glsl:
  - Agent spectrumPos (0-1) → log frequency → FFT bin → texture sample
  - fftEnergy modulates hueDiff in computeAffinity()
  - Louder frequency → stronger repulsion from different hues
```

## Key Formula

```glsl
float fftEnergy = sampleFFT(agent.spectrumPos);  // 0-1 normalized, based on color distribution position
float modulation = 1.0 + fftEnergy * frequencyModulation * 2.0;  // 1x to 3x
float modulatedHueDiff = hueDiff * modulation;
return modulatedHueDiff + (1.0 - intensity) * 0.3;
```

- `spectrumPos` is the agent's position in the color distribution (0 = first color, 1 = last color)
- Same hue (hueDiff=0): No change regardless of FFT
- Different hue + quiet: Normal attraction/avoidance
- Different hue + loud at agent's spectrum position: Strong repulsion (agents expand territory)

## Expected Visual Result

- **Quiet**: Species coexist, mixed organic networks
- **Bass hit**: Agents from start of color gradient surge outward, carve territory
- **Highs active**: Agents from end of color gradient expand, create fine edges
- **Full spectrum**: Chaotic territorial competition pulsing with music

Works with any color scheme:
- Rainbow red→violet: red agents pulse to bass, violet to treble
- Gradient blue→green: blue agents pulse to bass, green to treble
- Solid color: All agents respond to same frequency (no territorial competition, but still audio-reactive overall intensity)

---

## Implementation Notes

Additional features added during implementation:

### Beat Intensity Modulation
Beyond FFT-based frequency modulation, two beat-reactive parameters were added:

1. **Step Beat** (0-10): Modulates agent step size by beat intensity
   - Formula: `stepSize * (1.0 + beatIntensity * stepBeatModulation)`
   - Effect: Agents move faster on beats, creating pulsing motion

2. **Sensor Beat** (0-10): Modulates sensor distance by beat intensity
   - Formula: `sensorDistance * (1.0 + beatIntensity * sensorBeatModulation)`
   - Effect: Agents "see" further on beats, affecting pathfinding decisions

### Slider Ranges
All audio-reactive sliders extended to 0-10 for chaotic experimentation:
- Freq Mod: 0-10 (was 0-1)
- Step Beat: 0-10
- Sensor Beat: 0-10

### FFT Response Curve
Applied `sqrt()` to FFT energy values to boost mid-range response, making quieter sounds more visible in the modulation.

### Modulation Strength
Changed multiplier from `*2.0` (plan) to `*3.0` (implementation) for more dramatic effect at higher slider values. Combined with sqrt() curve and extended slider range, this allows for extreme chaotic territory wars.
