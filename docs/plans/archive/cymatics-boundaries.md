# Cymatics Boundary Reflections

Add wave reflections at screen edges to the Cymatics simulation using the mirror source method. Virtual sources positioned outside the visible area create interference patterns that simulate waves bouncing off boundaries, producing standing wave effects similar to bounded cymatics plates.

## Specification

### Config Changes

Add two fields to `CymaticsConfig` in `src/simulation/cymatics.h`:

```cpp
bool boundaries = false;       // Enable edge reflections
float reflectionGain = 1.0f;   // Mirror source attenuation (0.0-1.0)
```

Place after `patternAngle` and before `blendMode`.

### Shader Algorithm

In `shaders/cymatics.glsl`, after sampling each real source, sample 4 mirror positions if `boundaries` is enabled:

```glsl
uniform bool boundaries;
uniform float reflectionGain;

// Inside main(), replace the source sampling loop:
for (int i = 0; i < sourceCount; i++) {
    vec2 sourcePos = sources[i];
    sourcePos.x *= aspect;

    // Real source
    float dist = length(uv - sourcePos);
    float delay = dist * waveScale;
    float attenuation = exp(-dist * dist * falloff);
    totalWave += fetchWaveform(delay) * attenuation;

    // Mirror sources (4 reflections across screen edges)
    if (boundaries) {
        vec2 mirrors[4];
        mirrors[0] = vec2(-2.0 * aspect - sourcePos.x, sourcePos.y);  // Left
        mirrors[1] = vec2( 2.0 * aspect - sourcePos.x, sourcePos.y);  // Right
        mirrors[2] = vec2(sourcePos.x, -2.0 - sourcePos.y);           // Bottom
        mirrors[3] = vec2(sourcePos.x,  2.0 - sourcePos.y);           // Top

        for (int m = 0; m < 4; m++) {
            float mDist = length(uv - mirrors[m]);
            float mDelay = mDist * waveScale;
            float mAtten = exp(-mDist * mDist * falloff) * reflectionGain;
            totalWave += fetchWaveform(mDelay) * mAtten;
        }
    }
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| boundaries | bool | — | false | No | Boundaries |
| reflectionGain | float | 0.0–1.0 | 1.0 | Yes | Reflection Gain |

### Uniform Locations

Add to `Cymatics` struct in header:
```cpp
int boundariesLoc;
int reflectionGainLoc;
```

Get locations in `LoadComputeProgram()`:
```cpp
cym->boundariesLoc = rlGetLocationUniform(program, "boundaries");
cym->reflectionGainLoc = rlGetLocationUniform(program, "reflectionGain");
```

Set uniforms in `CymaticsUpdate()`:
```cpp
int boundariesInt = cym->config.boundaries ? 1 : 0;
rlSetUniform(cym->boundariesLoc, &boundariesInt, RL_SHADER_UNIFORM_INT, 1);
rlSetUniform(cym->reflectionGainLoc, &cym->config.reflectionGain,
             RL_SHADER_UNIFORM_FLOAT, 1);
```

### Param Registration

Add to `PARAM_TABLE` in `src/automation/param_registry.cpp`:
```cpp
{"cymatics.reflectionGain", 0.0f, 1.0f},
```

Add to `ParamRegistryInit()`:
```cpp
RegisterParam("cymatics.reflectionGain", &e->cymatics.reflectionGain);
```

### UI Controls

In `src/ui/imgui_effects.cpp`, add a new "Boundaries" section after "Wave" section:

```cpp
ImGui::SeparatorText("Boundaries");
ImGui::Checkbox("Boundaries##cym", &e->cymatics.boundaries);
if (e->cymatics.boundaries) {
    ModulatableSlider("Reflection Gain##cym", &e->cymatics.reflectionGain,
                      "cymatics.reflectionGain", "%.2f", modSources);
}
```

---

## Tasks

### Wave 1: Config and Uniform Infrastructure

#### Task 1.1: Config struct and uniform locations

**Files**: `src/simulation/cymatics.h`

**Build**:
1. Add `bool boundaries = false;` after `patternAngle` in `CymaticsConfig`
2. Add `float reflectionGain = 1.0f;` after `boundaries`
3. Add `int boundariesLoc;` after `sourceCountLoc` in `Cymatics` struct
4. Add `int reflectionGainLoc;` after `boundariesLoc`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: CPU uniform binding

**Files**: `src/simulation/cymatics.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `LoadComputeProgram()`, after line getting `sourceCountLoc`, add:
   ```cpp
   cym->boundariesLoc = rlGetLocationUniform(program, "boundaries");
   cym->reflectionGainLoc = rlGetLocationUniform(program, "reflectionGain");
   ```

2. In `CymaticsUpdate()`, after setting `sourceCountLoc` uniform (~line 159), add:
   ```cpp
   int boundariesInt = cym->config.boundaries ? 1 : 0;
   rlSetUniform(cym->boundariesLoc, &boundariesInt, RL_SHADER_UNIFORM_INT, 1);
   rlSetUniform(cym->reflectionGainLoc, &cym->config.reflectionGain,
                RL_SHADER_UNIFORM_FLOAT, 1);
   ```

**Verify**: Compiles.

#### Task 2.2: Shader mirror sampling

**Files**: `shaders/cymatics.glsl`
**Depends on**: Wave 1 complete

**Build**:
1. Add uniforms after `uniform int sourceCount;`:
   ```glsl
   uniform bool boundaries;
   uniform float reflectionGain;
   ```

2. Replace the source sampling loop (lines 50-59) with the algorithm from the Specification section. The new loop samples the real source first, then conditionally samples 4 mirror positions.

**Verify**: Compiles (shader compilation happens at runtime, but build should succeed).

#### Task 2.3: Param registration

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add to `PARAM_TABLE` array (alphabetical order in cymatics section):
   ```cpp
   {"cymatics.reflectionGain", 0.0f, 1.0f},
   ```

2. Add to `ParamRegistryInit()` in the cymatics section:
   ```cpp
   RegisterParam("cymatics.reflectionGain", &e->cymatics.reflectionGain);
   ```

**Verify**: Compiles.

#### Task 2.4: UI controls

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Find the Cymatics section (search for `SeparatorText("Wave")`).
2. After the Wave section controls (after `contourCount` slider, before `SeparatorText("Sources")`), add:
   ```cpp
   ImGui::SeparatorText("Boundaries");
   ImGui::Checkbox("Boundaries##cym", &e->cymatics.boundaries);
   if (e->cymatics.boundaries) {
       ModulatableSlider("Reflection Gain##cym", &e->cymatics.reflectionGain,
                         "cymatics.reflectionGain", "%.2f", modSources);
   }
   ```

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build`
- [ ] Run application, enable Cymatics
- [ ] Toggle Boundaries checkbox—pattern should show edge reflections
- [ ] Adjust Reflection Gain slider—reflections should attenuate
- [ ] Verify Reflection Gain appears in modulation param list
- [ ] Performance acceptable at 1080p (40 fetches/pixel with 8 sources)
