# Phyllotaxis Effect

Cell-based transform using sunflower seed spiral patterns (Vogel's model). Finds nearest seed in a phyllotaxis arrangement, applies Voronoi-style sub-effects: UV distortion, flat fill, center iso rings, edge glow. Two animation mechanisms: divergence angle drift (morphs spiral structure) and per-cell phase pulse (ripples outward).

## Current State

Voronoi provides the reference implementation for multi-sub-effect cellular transforms:
- `src/config/voronoi_config.h:6-24` - config struct with sub-effect intensities
- `shaders/voronoi.fs` - hash-based cell centers, multi-effect shader
- `src/ui/imgui_effects_transforms.cpp:971-1057` - toggle-button UI pattern

Research doc: `docs/research/phyllotaxis.md` - Vogel's model, seed finding algorithm, sub-effect specs.

## Technical Implementation

### Source
- [docs/research/phyllotaxis.md](../research/phyllotaxis.md) - primary reference
- [GM Shaders Mini: Phi](https://mini.gmshaders.com/p/phi) - GLSL implementation
- [Shadertoy: Phyllotaxis](https://www.shadertoy.com/view/4d2Xzw) - nearest-seed search

### Core Algorithm

**Constants:**
```glsl
#define PHI 1.618033988749
#define GOLDEN_ANGLE 2.39996322972865  // radians ≈ 137.5°
#define TWO_PI 6.28318530718
```

**Seed Position (Vogel's Model):**
```glsl
vec2 seedPosition(float i, float scale, float angle) {
    float r = scale * sqrt(i);
    float theta = i * angle;
    return r * vec2(cos(theta), sin(theta));
}
```

**Optimized Nearest Seed Search:**
```glsl
// Estimate starting index from radius: n ≈ (r/scale)²
float estimatedN = (r / scale) * (r / scale);
int searchRadius = 20;
int startIdx = max(0, int(estimatedN) - searchRadius);
int endIdx = min(maxSeeds, int(estimatedN) + searchRadius);

// Search ~40 seeds per pixel around estimate
for (int i = startIdx; i <= endIdx; i++) {
    vec2 seedPos = seedPosition(float(i), scale, divergenceAngle);
    float dist = length(p - seedPos);
    if (dist < nearestDist) {
        nearestDist = dist;
        nearestIndex = float(i);
        nearestPos = seedPos;
    }
}
```

**Max Seeds Calculation (fixed internal):**
```glsl
// Auto-calculate from scale to cover screen corners
// At scale=0.1, screen diagonal ~1.4, so maxSeeds ≈ (1.4/0.1)² = 196
int maxSeeds = int((1.5 / scale) * (1.5 / scale)) + 50;  // +50 buffer
```

### Sub-Effects

All sub-effects use `cell.dist` (distance to seed center) and `cell.toCenter` (normalized direction).

**UV Distortion:**
```glsl
if (uvDistortIntensity > 0.0) {
    vec2 displacement = cell.toCenter * uvDistortIntensity * 0.1;
    finalUV = uv + displacement;
}
```

**Flat Fill (Stained Glass):**
```glsl
if (flatFillIntensity > 0.0) {
    vec2 seedUV = cell.pos + 0.5;
    vec3 seedColor = texture(input, seedUV).rgb;
    float fillMask = smoothstep(cellRadius, cellRadius * 0.5, cell.dist);
    color = mix(color, seedColor, fillMask * flatFillIntensity);
}
```

**Center Iso Rings:**
```glsl
if (centerIsoIntensity > 0.0) {
    float phase = cell.dist * isoFrequency;
    float rings = abs(sin(phase * TWO_PI * 0.5));
    vec3 seedColor = texture(input, cell.pos + 0.5).rgb;
    color = mix(color, seedColor, rings * centerIsoIntensity);
}
```

**Edge Glow:**
```glsl
if (edgeGlowIntensity > 0.0) {
    float edgeness = smoothstep(cellRadius * 0.3, cellRadius, cell.dist);
    color += edgeness * edgeGlowIntensity;
}
```

### Animation

**Divergence Angle Drift:**
CPU accumulates `angleTime += deltaTime * angleSpeed`, shader receives current angle:
```glsl
uniform float divergenceAngle;  // Base 2.39996 + animated offset
```
Shifting from golden angle creates rotating spoke patterns (Fibonacci spirals emerge).

**Per-Cell Phase Pulse:**
CPU accumulates `phaseTime += deltaTime * phaseSpeed`, shader animates per-cell:
```glsl
float cellPhase = phaseTime + cell.index * 0.1;  // Phase offset by index
float pulse = 0.5 + 0.5 * sin(cellPhase);
// Apply pulse to sub-effect intensities for rippling effect
```

### Uniforms

| Uniform | Type | Purpose |
|---------|------|---------|
| `resolution` | vec2 | Screen dimensions |
| `scale` | float | Seed spacing (0.02-0.15) |
| `divergenceAngle` | float | Angle between seeds (base + animation) |
| `phaseTime` | float | Per-cell animation time |
| `cellRadius` | float | Size of effect region per cell |
| `isoFrequency` | float | Ring density |
| `uvDistortIntensity` | float | UV displacement amount |
| `flatFillIntensity` | float | Stained glass fill |
| `centerIsoIntensity` | float | Center rings |
| `edgeGlowIntensity` | float | Edge brightness |

---

## Phase 1: Config and Registration

**Goal**: Create config struct and register effect in pipeline.

**Build**:
- Create `src/config/phyllotaxis_config.h` with:
  - `enabled`, `scale`, `angleSpeed`, `phaseSpeed`
  - `cellRadius`, `isoFrequency`
  - `uvDistortIntensity`, `flatFillIntensity`, `centerIsoIntensity`, `edgeGlowIntensity`
- Modify `src/config/effect_config.h`:
  - Add include
  - Add `TRANSFORM_PHYLLOTAXIS` to enum
  - Add name case in `TransformEffectName()`
  - Add to `TransformOrderConfig::order` array
  - Add `PhyllotaxisConfig phyllotaxis` member
  - Add `IsTransformEnabled()` case

**Done when**: Effect enum exists, config struct defined with defaults.

---

## Phase 2: Shader

**Goal**: Implement phyllotaxis shader with all sub-effects.

**Build**:
- Create `shaders/phyllotaxis.fs`:
  - Vogel's model seed positioning
  - Optimized nearest-seed search with index estimation
  - Auto-calculated maxSeeds from scale
  - Four sub-effects: UV distort, flat fill, center iso, edge glow
  - Per-cell phase pulse animation
  - Early-out when all intensities zero

**Done when**: Shader compiles, implements algorithm from research doc.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader phyllotaxisShader`
  - Add uniform location ints (10 total)
  - Add `float phyllotaxisAngleTime` and `float phyllotaxisPhaseTime` accumulators
- Modify `src/render/post_effect.cpp`:
  - Add `LoadShader()` call in `LoadPostEffectShaders()`
  - Add to success check
  - Add `GetShaderLocation()` calls in `GetShaderUniformLocations()`
  - Add to `SetResolutionUniforms()` if needed
  - Add `UnloadShader()` in `PostEffectUninit()`

**Done when**: Shader loads at startup, uniform locations cached.

---

## Phase 4: Shader Setup

**Goal**: Wire config values to shader uniforms.

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare `void SetupPhyllotaxis(PostEffect* pe)`
- Modify `src/render/shader_setup.cpp`:
  - Add case in `GetTransformEffect()` returning phyllotaxis entry
  - Implement `SetupPhyllotaxis()`:
    - Accumulate `phyllotaxisAngleTime` and `phyllotaxisPhaseTime` on CPU
    - Compute `divergenceAngle = GOLDEN_ANGLE + angleOffset` from accumulated time
    - Bind all uniforms

**Done when**: Config values reach shader via setup function.

---

## Phase 5: UI Panel

**Goal**: Add phyllotaxis controls to Cellular category.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add case in `GetTransformCategory()` returning Cellular
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionPhyllotaxis = false`
  - Add `DrawCellularPhyllotaxis()` helper with:
    - Enabled checkbox with `MoveTransformToEnd()` on enable
    - `ModulatableSlider` for scale, angleSpeed, phaseSpeed
    - Sub-effect toggle buttons (matching Voronoi's pattern)
    - Blend mix sliders when multiple sub-effects active
    - Tree node for iso settings (frequency, cellRadius)
  - Add call in `DrawCellularCategory()` after Lattice Fold

**Done when**: UI appears in Cellular category, controls modify config.

---

## Phase 6: Serialization and Modulation

**Goal**: Enable preset save/load and audio-reactive modulation.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhyllotaxisConfig, ...)`
  - Add to_json entry
  - Add from_json entry
- Modify `src/automation/param_registry.cpp`:
  - Add PARAM_TABLE entries for modulatable params:
    - `phyllotaxis.scale`, `phyllotaxis.angleSpeed`, `phyllotaxis.phaseSpeed`
    - `phyllotaxis.cellRadius`, `phyllotaxis.isoFrequency`
    - Four intensity params
  - Add matching entries to `targets[]` array (same indices)

**Done when**: Presets save/load phyllotaxis settings, modulation routes work.

---

## Phase 7: Verification

**Goal**: Confirm complete integration.

**Verify**:
- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Cellular" category badge
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling adds it to pipeline list
- [ ] UI controls modify effect in real-time
- [ ] Divergence angle drift creates rotating spirals
- [ ] Phase pulse creates outward ripples
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
- [ ] Build succeeds with no warnings

**Done when**: All verification items pass.

---

## Implementation Notes

### Sub-Effects (Post-Phase 6 Expansion)

The initial plan specified 4 sub-effects. Expanded to match voronoi's full 9-effect suite:

| Effect | Description | Pulse Modulation |
|--------|-------------|------------------|
| Distort | UV displacement toward cell center | Breathing effect |
| Organic | UV distort with edge mask | Wave-like flow |
| Edge Iso | Rings from cell borders, black between | Outward expansion |
| Ctr Iso | Rings from seed centers, black between | Outward expansion |
| Fill | Stained glass with dark edges | Edge darkness pulse |
| Glow | Black cells, colored edges from texture | Edge brightness |
| Ratio | Edge/center distance shading | None |
| Determ | Cross product of border/center vectors | None |
| Detect | Blob patches at cell boundaries | None |

### Voronoi Fixes Applied

During phyllotaxis development, several voronoi sub-effects were improved:

1. **Center Iso**: Removed `* centerDist` fade multiplier - now uniform intensity like Edge Iso
2. **Edge Glow**: Changed from additive cell color to `mix(color, edgeOnly, intensity)` where `edgeOnly = edge * texture(fragTexCoord).rgb` - black cells with colored edges
3. **Both Isos**: Changed to `isoColor = rings * cellColor` then mix - black between rings at full intensity
4. **Edge Detect**: Changed to `detectColor = edge * cellColor` then mix - black background with colored blobs

### Geometry Differences

Phyllotaxis uses approximate edge distance: `edgeDist = (secondDist - nearestDist) * 0.5`. Voronoi computes exact border via segment projection. The approximation works well for artistic purposes.

For determinant effect, phyllotaxis normalizes both vectors before cross product (gives sine of angle, 0-1 range). Voronoi uses raw vectors scaled by 4x.
