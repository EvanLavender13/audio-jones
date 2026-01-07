# Voronoi Flexible Effects

Replace the discrete mode dropdown with 9 combinable intensity sliders. Each effect uses the same voronoi computation but renders differently. Users blend effects by adjusting individual intensities.

## Current State

- `src/config/voronoi_config.h` - VoronoiConfig with mode enum (3 UV distortion modes)
- `shaders/voronoi.fs` - UV distortion shader with mode branching
- `src/ui/imgui_effects.cpp:238-257` - Combo dropdown for mode selection
- `src/automation/param_registry.cpp:27-30` - 4 params registered (strength, scale, speed, edgeFalloff)
- `src/render/shader_setup.cpp:31-44` - SetupVoronoi passes uniforms

## Technical Implementation

### Voronoi Data Structure

The shader computes a `vec4` containing two vectors:
- `xy` = vector from pixel to nearest cell border
- `zw` = vector from pixel to cell center

Plus cell identification for color sampling:
```glsl
vec2 cellID = ip + mg;  // Grid cell containing nearest point
vec2 cellCenterUV = (cellID + 0.5) / vec2(scale * aspect, scale);
vec3 cellColor = texture(texture0, cellCenterUV).rgb;
```

### Effect Formulas

Each effect transforms voronoi data into a color contribution with a fixed blend operation.

#### 1. UV Distortion (existing behavior)
```glsl
// Blend: UV replacement
vec2 displacement = mr * uvDistortIntensity;
displacement /= vec2(scale * aspect, scale);
vec2 finalUV = fragTexCoord + displacement;
```

#### 2. Edge Iso Rings
```glsl
// Blend: Additive
float edgeDist = length(voronoiData.xy);
float rings = abs(sin(edgeDist * isoFrequency)) * edgeDist;
color += vec3(rings) * edgeIsoIntensity;
```

#### 3. Center Iso Rings
```glsl
// Blend: Additive
float centerDist = length(voronoiData.zw);
float rings = abs(sin(centerDist * isoFrequency));
color += rings * cellColor * centerIsoIntensity;
```

#### 4. Flat Fill (Stained Glass)
```glsl
// Blend: Mix/Replace
float fillMask = smoothstep(0.0, edgeFalloff, length(voronoiData.xy));
color = mix(color, cellColor, fillMask * flatFillIntensity);
```

#### 5. Edge Darken (Leadframe)
```glsl
// Blend: Multiply
float edgeDist = length(voronoiData.xy);
float darken = smoothstep(0.0, edgeFalloff, edgeDist);
color *= mix(1.0, darken, edgeDarkenIntensity);
```

#### 6. Angle Shade
```glsl
// Blend: Mix
vec2 toBorder = normalize(voronoiData.xy + 0.0001);
vec2 toCenter = normalize(voronoiData.zw + 0.0001);
float angle = abs(dot(toBorder, toCenter));
color = mix(color, angle * cellColor, angleShadeIntensity);
```

#### 7. Determinant Shade
```glsl
// Blend: Mix
// 2D cross product magnitude of the two vectors
float det = abs(voronoiData.x * voronoiData.w - voronoiData.z * voronoiData.y);
color = mix(color, det * cellColor, determinantIntensity);
```

#### 8. Distance Ratio
```glsl
// Blend: Mix
float ratio = length(voronoiData.xy) / (length(voronoiData.zw) + 0.001);
color = mix(color, ratio * cellColor, ratioIntensity);
```

#### 9. Edge Detect
```glsl
// Blend: Additive (for glow) or Mix (for lines)
float edge = smoothstep(0.0, edgeFalloff,
    length(voronoiData.xy) - length(voronoiData.zw));
color += edge * cellColor * edgeDetectIntensity;
```

### Shared Parameters

- `scale` - Cell density (existing)
- `speed` - Animation rate (existing)
- `edgeFalloff` - Edge softness for multiple effects (existing)
- `isoFrequency` - Ring density for iso effects (new)

---

## Phase 1: Config and Shader Foundation

**Goal**: Replace mode enum with intensity floats in config and shader.

**Build**:
- Modify `voronoi_config.h`: Remove `VoronoiMode` enum, add 9 intensity floats (0.0-1.0 range), add `isoFrequency` param
- Modify `voronoi.fs`:
  - Remove mode uniform and branching
  - Add 9 intensity uniforms
  - Add `isoFrequency` uniform
  - Restructure to compute voronoi data once, then apply all effects
  - Compute `cellCenterUV` and sample `cellColor` from image
  - Output: `finalUV` for distortion, then color modifications

**Done when**: Shader compiles, setting any single intensity to 1.0 produces that effect.

---

## Phase 2: Uniform Locations and Setup

**Goal**: Wire new uniforms through the rendering pipeline.

**Build**:
- Modify `src/render/post_effect.h`: Add uniform location ints for all new uniforms
- Modify `src/render/post_effect.cpp`: Get uniform locations in `GetShaderUniformLocations()`
- Modify `src/render/shader_setup.cpp`: Update `SetupVoronoi()` to pass all intensity uniforms

**Done when**: All uniforms reach the shader correctly.

---

## Phase 3: UI Sliders

**Goal**: Replace mode dropdown with intensity sliders.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Remove mode Combo
  - Add `ModulatableSlider` for each intensity (0.0-1.0 range)
  - Add slider for `isoFrequency`
  - Group related effects with visual separators

**Done when**: UI shows all sliders, adjusting them affects the output.

---

## Phase 4: Param Registry

**Goal**: Enable modulation for new parameters.

**Build**:
- Modify `src/automation/param_registry.cpp`:
  - Add entries for all 9 intensity params
  - Add entry for `isoFrequency`
  - Update `targets[]` array with new config pointers

**Done when**: All new params appear in modulation dropdown and respond to automation.

---

## Phase 5: Preset Serialization

**Goal**: Save/load new params in presets.

**Build**:
- Modify `src/config/preset.cpp`:
  - Remove mode enum serialization
  - Add serialization for all intensity floats
  - Add serialization for `isoFrequency`
  - Handle migration: old presets with `mode` get default intensities

**Done when**: Presets save and load correctly, old presets still work.

---

## Phase 6: Polish and Defaults

**Goal**: Tune default values and verify combinations work well.

**Build**:
- Set sensible defaults in `voronoi_config.h` (all intensities at 0.0 except maybe one)
- Test effect combinations for visual quality
- Verify performance with multiple effects active
- Update `docs/research/voronoi-shaders.md` with new effect descriptions

**Done when**: Effect combinations produce pleasing results, no visual artifacts.
