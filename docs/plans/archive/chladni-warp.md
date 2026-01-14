# Chladni Warp

UV displacement transform using Chladni plate equations. The gradient of the Chladni function displaces texture coordinates toward/along nodal lines, creating kaleidoscopic warping patterns. Animated n/m parameters produce morphing symmetry; audio-reactive parameters sync pattern complexity to music.

## Current State

Hook points for a new transform effect:
- `src/config/effect_config.h:38-64` - TransformEffectType enum, name mapping, order array
- `src/render/shader_setup.cpp:13-68` - GetTransformEffect dispatch
- `src/ui/imgui_effects_transforms.cpp` - Transform category UI panels
- `src/automation/param_registry.cpp:12-120` - Modulatable parameter definitions

Similar effects for reference:
- `shaders/wave_ripple.fs` - Radial sine UV displacement with gradient shading
- `shaders/gradient_flow.fs` - Sobel-based UV displacement along luminance gradient
- `src/config/wave_ripple_config.h` - Config struct pattern with origin, frequency, strength

Spectral centroid integration points:
- `src/analysis/bands.h:17-32` - BandEnergies struct
- `src/analysis/bands.cpp` - BandEnergiesProcess computes bass/mid/treb
- `src/automation/mod_sources.h:8-18` - ModSource enum

Modulation popup UI (requires redesign for 9th source):
- `src/ui/modulatable_slider.cpp:274-338` - DrawModulationPopup() function
- `src/ui/modulatable_slider.cpp:288-289` - Hardcoded 4+4 source arrays:
  ```cpp
  static const ModSource audioSources[] = { MOD_SOURCE_BASS, MOD_SOURCE_MID, MOD_SOURCE_TREB, MOD_SOURCE_BEAT };
  static const ModSource lfoSources[] = { MOD_SOURCE_LFO1, MOD_SOURCE_LFO2, MOD_SOURCE_LFO3, MOD_SOURCE_LFO4 };
  ```
- `src/ui/modulatable_slider.cpp:104-159` - DrawSourceButtonRow() renders button grid
- `src/automation/mod_sources.cpp:52-72` - ModSourceGetColor() assigns source colors

## Technical Implementation

### Source
- [Paul Bourke - Chladni Plates](https://paulbourke.net/geometry/chladni/) - Core equations
- [docs/research/chladni-warp.md](../research/chladni-warp.md) - Full algorithm specification

### Core Chladni Equation (Rectangular Plate)

```glsl
float chladni(vec2 uv, float n, float m, float L) {
    float PI = 3.14159265;
    return cos(n * PI * uv.x / L) * cos(m * PI * uv.y / L)
         - cos(m * PI * uv.x / L) * cos(n * PI * uv.y / L);
}
```

Nodal lines (sand accumulation) occur where `chladni() = 0`.

### Analytical Gradient

```glsl
vec2 chladniGradient(vec2 uv, float n, float m, float L) {
    float PI = 3.14159265;
    float nx = n * PI / L;
    float mx = m * PI / L;

    float dfdx = -nx * sin(nx * uv.x) * cos(mx * uv.y)
                 +mx * sin(mx * uv.x) * cos(nx * uv.y);
    float dfdy = -mx * cos(nx * uv.x) * sin(mx * uv.y)
                 +nx * cos(mx * uv.x) * sin(nx * uv.y);

    return vec2(dfdx, dfdy);
}
```

Uses 6 trig operations vs 10 for finite differences.

### UV Displacement (Three Modes)

```glsl
vec2 chladniWarp(vec2 uv, float n, float m, float L, float strength, int mode) {
    float f = chladni(uv, n, m, L);
    vec2 grad = chladniGradient(uv, n, m, L);
    float gradLen = length(grad);

    if (gradLen < 0.001) return uv;  // Avoid div-by-zero at critical points

    vec2 dir = grad / gradLen;

    if (mode == 0) {
        // Toward nodes: sand accumulation effect
        return uv - dir * strength * f;
    } else if (mode == 1) {
        // Along nodes: stream effect perpendicular to gradient
        vec2 perp = vec2(-dir.y, dir.x);
        return uv + perp * strength * f;
    } else {
        // Intensity-based: stronger near antinodes
        return uv + dir * strength * abs(f);
    }
}
```

### Animation via CPU-Accumulated Phase

```glsl
// In shader:
uniform float animPhase;  // CPU-accumulated, not raw time
uniform float animRange;

float n_anim = n + animRange * sin(animPhase);
float m_anim = m + animRange * cos(animPhase);
```

CPU accumulates: `animPhase += deltaTime * animSpeed`

### Coordinate Centering

```glsl
vec2 centered = fragTexCoord * 2.0 - 1.0;  // Map [0,1] to [-1,1]
// Apply chladni warp to centered coords
// Remap back: uv = (warped + 1.0) * 0.5
```

### Spectral Centroid Calculation

```cpp
// In BandEnergiesProcess, after magnitude loop:
float weightedSum = 0.0f;
float totalEnergy = 0.0f;
for (int i = 1; i < binCount; i++) {
    weightedSum += (float)i * magnitude[i];
    totalEnergy += magnitude[i];
}
float centroid = (totalEnergy > 0.001f) ? weightedSum / totalEnergy : 0.0f;
bands->centroid = centroid / (float)binCount;  // Normalize to [0,1]
```

Apply same attack/release smoothing as bass/mid/treb.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| enabled | bool | - | false | Enable/disable effect |
| n | float | 1.0-12.0 | 4.0 | X-axis frequency mode |
| m | float | 1.0-12.0 | 3.0 | Y-axis frequency mode |
| plateSize | float | 0.5-2.0 | 1.0 | Plate dimension (L). Smaller = denser |
| strength | float | 0.0-0.5 | 0.1 | UV displacement magnitude |
| warpMode | int | 0-2 | 0 | 0=toward, 1=along, 2=intensity |
| animSpeed | float | 0.0-2.0 | 0.0 | Animation rate (radians/second) |
| animRange | float | 0.0-5.0 | 0.0 | Amplitude of n/m oscillation |
| preFold | bool | - | false | Enable abs(uv) symmetry |

Modulatable: n, m, strength, animRange

---

## Phase 1: Spectral Centroid Backend

**Goal**: Add centroid calculation to audio analysis pipeline.

**Build**:
- `src/analysis/bands.h`: Add `centroid` and `centroidSmooth` fields to BandEnergies struct
- `src/analysis/bands.cpp`: Compute centroid in BandEnergiesProcess() using weighted average formula. Apply same attack/release smoothing as bass/mid/treb.
- `src/automation/mod_sources.h`: Add `MOD_SOURCE_CENTROID` to enum before `MOD_SOURCE_COUNT`
- `src/automation/mod_sources.cpp`: Update ModSourcesUpdate() to read centroidSmooth, add name ("CENT") and color (suggest yellow/gold to distinguish from cyan/magenta audio band colors)

**Done when**: MOD_SOURCE_CENTROID enum exists and centroidSmooth value updates from audio.

---

## Phase 2: Modulation Popup UI Redesign

**Goal**: Redesign the modulation source selector to accommodate 9 sources (was 8).

**Context**: The current popup uses two rows of 4 buttons each (BASS/MID/TREB/BEAT + LFO1-4). Adding CENTROID breaks this 4+4 symmetry. This phase uses the **frontend-design skill** to design a clean layout.

**Design constraints**:
- Must fit 9 sources without looking unbalanced
- Existing button width is 50px, popup is ~220px wide
- Sources have semantic groupings: frequency bands (BASS/MID/TREB), analysis (BEAT/CENTROID), oscillators (LFO1-4)
- Each button shows live value indicator bar beneath it
- Color coding must remain distinct and readable

**Build**:
- Use `/frontend-design` skill to design the new popup layout
- Consider options: 3-row layout (3+2+4 or 3+3+3), collapsible sections, or reorganized groupings
- `src/ui/modulatable_slider.cpp`: Update DrawModulationPopup() with new layout
- `src/ui/modulatable_slider.cpp`: Update or replace DrawSourceButtonRow() if needed

**Done when**: Modulation popup displays all 9 sources in a visually balanced layout. CENTROID button visible and functional.

---

## Phase 3: Config and Registration

**Goal**: Create config struct and register effect in pipeline.

**Build**:
- `src/config/chladni_warp_config.h`: Create config struct with all parameters
- `src/config/effect_config.h`: Add include, enum entry `TRANSFORM_CHLADNI_WARP`, name case, order array entry, config member, IsTransformEnabled case
- `src/config/preset.cpp`: Add JSON macro, to_json/from_json entries

**Done when**: Effect appears in TransformEffectType enum and preset serialization compiles.

---

## Phase 4: Shader

**Goal**: Implement Chladni warp fragment shader.

**Build**:
- `shaders/chladni_warp.fs`: Create shader with chladni(), chladniGradient(), chladniWarp() functions. Uniforms: n, m, plateSize, strength, warpMode, animPhase, animRange, preFold

**Done when**: Shader compiles (test with placeholder load).

---

## Phase 5: PostEffect Integration

**Goal**: Load shader and wire uniforms.

**Build**:
- `src/render/post_effect.h`: Add `chladniWarpShader` and uniform location members
- `src/render/post_effect.cpp`: Load shader in LoadPostEffectShaders(), add to success check, get uniform locations, unload in PostEffectUninit()
- `src/render/shader_setup.h`: Declare SetupChladniWarp()
- `src/render/shader_setup.cpp`: Add dispatch case in GetTransformEffect(), implement SetupChladniWarp() with CPU phase accumulation

**Done when**: Effect renders when enabled via code.

---

## Phase 6: UI Panel

**Goal**: Add controls to Transforms panel.

**Build**:
- `src/ui/imgui_effects.cpp`: Add category case in GetTransformCategory() (TRANSFORM_WARP)
- `src/ui/imgui_effects_transforms.cpp`: Add section state, DrawWarpChladniWarp() helper with ModulatableSliders for n/m/strength/animRange, combo for warpMode, checkbox for preFold

**Done when**: UI controls visible and modify effect in real-time.

---

## Phase 7: Parameter Registration

**Goal**: Enable modulation routing for key parameters.

**Build**:
- `src/automation/param_registry.cpp`: Add PARAM_TABLE entries for chladniWarp.n, chladniWarp.m, chladniWarp.strength, chladniWarp.animRange with appropriate ranges. Add matching targets array entries.

**Done when**: Parameters respond to CENTROID and other mod sources.

---

## Implementation Notes

### MOD_SOURCE_CENTROID Enum Position

**Issue**: Phase 1 originally added `MOD_SOURCE_CENTROID` between `MOD_SOURCE_BEAT` and `MOD_SOURCE_LFO1`. This shifted LFO1-4 from positions 4-7 to 5-8, breaking all existing presets that used LFO modulation.

**Fix**: `MOD_SOURCE_CENTROID` must be added at position 8 (after LFO4, before COUNT) to preserve backward compatibility:
```cpp
typedef enum {
    MOD_SOURCE_BASS = 0,
    MOD_SOURCE_MID,
    MOD_SOURCE_TREB,
    MOD_SOURCE_BEAT,
    MOD_SOURCE_LFO1,      // Must stay at 4
    MOD_SOURCE_LFO2,      // Must stay at 5
    MOD_SOURCE_LFO3,      // Must stay at 6
    MOD_SOURCE_LFO4,      // Must stay at 7
    MOD_SOURCE_CENTROID,  // New sources go here (8+)
    MOD_SOURCE_COUNT
} ModSource;
```

### Physarum Affinity Calculation Regression

**Issue**: The physarum-competitive-species feature changed `computeAffinity()` in `shaders/physarum_agents.glsl`, breaking existing presets even when `repulsionStrength=0`.

**Original formula** (all trails attract, similar hue more):
```glsl
if (intensity < 0.001) return 1.0;
return hueDiff + (1.0 - intensity) * 0.3;
```

**Fix**: Blend between original and competitive formulas using `repulsionStrength`:
```glsl
if (intensity < 0.001) return mix(1.0, 0.5, repulsionStrength);
float oldAffinity = hueDiff + (1.0 - intensity) * 0.3;
float newAffinity = 0.5 - attraction * 0.5 + repulsion * 2.0;
return mix(oldAffinity, newAffinity, repulsionStrength);
```

When `repulsionStrength=0`, behavior matches pre-competitive-species exactly.
