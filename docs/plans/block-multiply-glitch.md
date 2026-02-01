# Block Multiply Glitch Sub-Mode

Add a new "Block Multiply" sub-mode to the existing Glitch effect. Creates crystalline/prismatic feedback through recursive block-based UV folding with multiplicative cross-sampling—distinct from existing digital glitch modes which use additive blending and simple displacement.

## Specification

### Config Additions

Add to `GlitchConfig` struct in `src/config/glitch_config.h`:

```cpp
// Block Multiply: recursive block UV folding with cross-sampling
bool blockMultiplyEnabled = false;
float blockMultiplySize = 28.0f;       // Grid density (4-64)
float blockMultiplyControl = 0.1f;     // Distortion strength per iteration (0-0.5)
int blockMultiplyIterations = 6;       // Recursive passes (1-8)
float blockMultiplyIntensity = 1.0f;   // Blend with original (0-1)
```

### Shader Algorithm

Add to `shaders/glitch.fs` after Temporal Jitter, before Overlay stage:

```glsl
// Uniforms to add
uniform bool blockMultiplyEnabled;
uniform float blockMultiplySize;
uniform float blockMultiplyControl;
uniform int blockMultiplyIterations;
uniform float blockMultiplyIntensity;

// Helper function (add near top with other helpers)
vec3 spectrumOffset(float t) {
    float lo = step(t, 0.5);
    float hi = 1.0 - lo;
    float w = clamp((t - 1.0/6.0) / (4.0/6.0), 0.0, 1.0);
    w = 1.0 - abs(2.0 * w - 1.0);  // triangle ramp
    float neg_w = 1.0 - w;
    vec3 ret = vec3(lo, 1.0, hi) * vec3(neg_w, w, neg_w);
    return pow(ret, vec3(1.0 / 2.2));
}

// In main(), after Temporal Jitter, before Overlay:
if (blockMultiplyEnabled) {
    vec2 bmUv = uv;
    vec4 sum = texture(texture0, bmUv);

    for (int i = 0; i < blockMultiplyIterations; i++) {
        // Recursive UV folding through block pattern
        vec2 blockPattern = fract(blockMultiplySize * bmUv) + 0.5;
        bmUv /= pow(blockPattern, vec2(blockMultiplyControl));

        // Clamp to prevent blowout
        sum = clamp(sum, 0.15, 1.0);

        // Cross-sampling with multiply/divide alternation
        float fi = float(i);
        vec2 px = 1.0 / resolution;
        sum /= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(px.x, fi * px.y)), 0.0, 2.0);
        sum *= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(px.x, -fi * px.y)), 0.0, 2.0);
        sum *= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(-fi * px.x, fi * px.y)), 0.0, 2.0);
        sum /= 0.1 + 0.9 * clamp(texture(texture0, bmUv + vec2(-fi * px.x, -fi * px.y)), 0.0, 2.0);

        // Chromatic spectrum offset based on luminance
        float lum = length(sum.xyz);
        sum.xyz /= 1.01 - 0.025 * spectrumOffset(1.0 - lum);
        sum.xyz *= 1.0 + 0.01 * spectrumOffset(lum);
    }

    // Normalize and contrast
    sum = 0.1 + 0.9 * sum;
    sum /= length(sum);
    sum = (-0.2 + 2.0 * sum) * 0.9;

    // Blend with existing color
    col = mix(col, sum.rgb, blockMultiplyIntensity);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| blockMultiplyEnabled | bool | - | false | No | Enabled |
| blockMultiplySize | float | 4-64 | 28.0 | Yes | Block Size |
| blockMultiplyControl | float | 0-0.5 | 0.1 | Yes | Distortion |
| blockMultiplyIterations | int | 1-8 | 6 | No | Iterations |
| blockMultiplyIntensity | float | 0-1 | 1.0 | Yes | Intensity |

### Param Registry Entries

Bounds table entries:
```cpp
{"glitch.blockMultiplySize", {4.0f, 64.0f}},
{"glitch.blockMultiplyControl", {0.0f, 0.5f}},
{"glitch.blockMultiplyIntensity", {0.0f, 1.0f}},
```

Pointer registration:
```cpp
&effects->glitch.blockMultiplySize,
&effects->glitch.blockMultiplyControl,
&effects->glitch.blockMultiplyIntensity,
```

---

## Tasks

### Wave 1: Config

#### Task 1.1: Add Block Multiply fields to GlitchConfig

**Files**: `src/config/glitch_config.h`
**Creates**: Config fields that shader setup and UI will reference

**Build**:
1. Add the five new fields from Specification > Config Additions after the Temporal Jitter fields (after line 74)

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader, UI, Params (parallel)

#### Task 2.1: Add Block Multiply to glitch shader

**Files**: `shaders/glitch.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Add the 5 new uniforms after `temporalJitterGate` (line 76)
2. Add `spectrumOffset()` helper function after `rand()` (around line 130)
3. Add Block Multiply processing block after Temporal Jitter (after line 284), before Overlay stage (line 287)
4. Use the algorithm from Specification > Shader Algorithm

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Add Block Multiply UI controls

**Files**: `src/ui/imgui_effects_retro.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add a new TreeNode section after Temporal (after line 175, before `ImGui::Spacing()` on line 177):
```cpp
if (TreeNodeAccented("Block Multiply##glitch", categoryGlow)) {
    ImGui::Checkbox("Enabled##blockmultiply", &g->blockMultiplyEnabled);
    if (g->blockMultiplyEnabled) {
        ModulatableSlider("Block Size##blockmultiply", &g->blockMultiplySize,
                          "glitch.blockMultiplySize", "%.1f", modSources);
        ModulatableSlider("Distortion##blockmultiply", &g->blockMultiplyControl,
                          "glitch.blockMultiplyControl", "%.3f", modSources);
        ImGui::SliderInt("Iterations##blockmultiply", &g->blockMultiplyIterations, 1, 8);
        ModulatableSlider("Intensity##blockmultiply", &g->blockMultiplyIntensity,
                          "glitch.blockMultiplyIntensity", "%.2f", modSources);
    }
    TreeNodeAccentedPop();
}
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Add Block Multiply uniforms to shader setup

**Files**: `src/render/shader_setup_retro.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Find the `SetupGlitch()` function
2. Add uniform bindings after the temporal jitter uniforms:
```cpp
SetShaderValue(pe->glitch, pe->glitchBlockMultiplyEnabledLoc, &e->glitch.blockMultiplyEnabled, SHADER_UNIFORM_INT);
SetShaderValue(pe->glitch, pe->glitchBlockMultiplySizeLoc, &e->glitch.blockMultiplySize, SHADER_UNIFORM_FLOAT);
SetShaderValue(pe->glitch, pe->glitchBlockMultiplyControlLoc, &e->glitch.blockMultiplyControl, SHADER_UNIFORM_FLOAT);
SetShaderValue(pe->glitch, pe->glitchBlockMultiplyIterationsLoc, &e->glitch.blockMultiplyIterations, SHADER_UNIFORM_INT);
SetShaderValue(pe->glitch, pe->glitchBlockMultiplyIntensityLoc, &e->glitch.blockMultiplyIntensity, SHADER_UNIFORM_FLOAT);
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Add Block Multiply uniform locations to PostEffect

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `post_effect.h`, add location fields to PostEffect struct after temporal jitter locations:
```cpp
int glitchBlockMultiplyEnabledLoc;
int glitchBlockMultiplySizeLoc;
int glitchBlockMultiplyControlLoc;
int glitchBlockMultiplyIterationsLoc;
int glitchBlockMultiplyIntensityLoc;
```

2. In `post_effect.cpp`, in `PostEffectInit()`, add GetShaderLocation calls after temporal jitter locations:
```cpp
pe->glitchBlockMultiplyEnabledLoc = GetShaderLocation(pe->glitch, "blockMultiplyEnabled");
pe->glitchBlockMultiplySizeLoc = GetShaderLocation(pe->glitch, "blockMultiplySize");
pe->glitchBlockMultiplyControlLoc = GetShaderLocation(pe->glitch, "blockMultiplyControl");
pe->glitchBlockMultiplyIterationsLoc = GetShaderLocation(pe->glitch, "blockMultiplyIterations");
pe->glitchBlockMultiplyIntensityLoc = GetShaderLocation(pe->glitch, "blockMultiplyIntensity");
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Register Block Multiply params for modulation

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add bounds entries to `PARAM_TABLE` after `glitch.temporalJitterGate` (around line 154):
```cpp
{"glitch.blockMultiplySize", {4.0f, 64.0f}},
{"glitch.blockMultiplyControl", {0.0f, 0.5f}},
{"glitch.blockMultiplyIntensity", {0.0f, 1.0f}},
```

2. Add pointer registrations in `ParamRegistryInit()` after `&effects->glitch.temporalJitterGate` (around line 487):
```cpp
&effects->glitch.blockMultiplySize,
&effects->glitch.blockMultiplyControl,
&effects->glitch.blockMultiplyIntensity,
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Add Block Multiply to preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Find the glitch serialization section (search for `blockMaskTintB` or `temporalJitterGate`)
2. Add JSON serialization for the new fields after temporal jitter:
```cpp
// In SavePreset, glitch section:
j["glitch"]["blockMultiplyEnabled"] = e.glitch.blockMultiplyEnabled;
j["glitch"]["blockMultiplySize"] = e.glitch.blockMultiplySize;
j["glitch"]["blockMultiplyControl"] = e.glitch.blockMultiplyControl;
j["glitch"]["blockMultiplyIterations"] = e.glitch.blockMultiplyIterations;
j["glitch"]["blockMultiplyIntensity"] = e.glitch.blockMultiplyIntensity;

// In LoadPreset, glitch section:
LOAD_FIELD(e.glitch.blockMultiplyEnabled, "glitch", "blockMultiplyEnabled");
LOAD_FIELD(e.glitch.blockMultiplySize, "glitch", "blockMultiplySize");
LOAD_FIELD(e.glitch.blockMultiplyControl, "glitch", "blockMultiplyControl");
LOAD_FIELD(e.glitch.blockMultiplyIterations, "glitch", "blockMultiplyIterations");
LOAD_FIELD(e.glitch.blockMultiplyIntensity, "glitch", "blockMultiplyIntensity");
```

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build`
- [ ] Run application, enable Glitch effect
- [ ] Navigate to Glitch > Block Multiply section in UI
- [ ] Enable Block Multiply, verify crystalline/prismatic visual appears
- [ ] Adjust Block Size slider—higher values create finer grid
- [ ] Adjust Distortion slider—higher values increase folding intensity
- [ ] Adjust Iterations slider—more iterations deepen the recursive effect
- [ ] Adjust Intensity slider—blends between original and processed
- [ ] Test modulation on Block Size, Distortion, and Intensity params
- [ ] Save preset, reload, verify Block Multiply settings persist
