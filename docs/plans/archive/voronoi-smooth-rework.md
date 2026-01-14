# Voronoi Smooth Rework

Add smooth/bubbly Voronoi mode, replace weak effects with better ones, fix iso transparency.

## Current State

- `shaders/voronoi.fs` - 9 intensity-based effects using standard (sharp polygon) Voronoi
- `src/config/voronoi_config.h` - VoronoiConfig struct with 9 effect intensities
- `src/ui/imgui_effects_transforms.cpp:781-857` - 3x3 IntensityToggleButton grid
- `src/render/shader_setup.cpp` - uniform binding
- `src/config/preset.cpp` - JSON serialization
- `src/automation/param_registry.cpp` - modulation registration

## Changes Summary

**Add**: Smooth toggle (bubbly cells + anti-aliased contours), Organic Flow, Edge Glow
**Remove**: Edge Darken, Angle Shade
**Fix**: Edge Iso, Center Iso, Edge Detect show content through - should replace/blend instead of add

## Technical Implementation

### Smooth Voronoi Algorithm

**Source**: ShaderToy "Smooth Voronoi Contours" + IQ's smooth Voronoi

Standard Voronoi finds minimum distance (sharp polygonal cells):
```glsl
md = min(md, dist);
```

Smooth Voronoi accumulates inverse distance powers (rounded blob cells):
```glsl
// In cell search loop, instead of min():
float d = max(dot(r, r), 1e-4);
res += 1.0 / pow(d, falloff);

// After loop:
float smoothDist = pow(1.0 / res, 0.5 / falloff);
```

The `falloff` parameter (4.0-8.0 typical) controls blob roundness. Higher = sharper transitions between cells.

### Smooth Contours (Anti-Aliasing)

**Source**: ShaderToy "Smooth Voronoi Contours", IQ "Ellipse - Distance Estimation"

Anti-alias periodic patterns using numerical gradient:
```glsl
// Compute gradient magnitude of distance field
vec2 e = vec2(0.001, 0.0);
float g = length(vec2(dist - computeDist(uv - e.xy),
                      dist - computeDist(uv - e.yx))) / e.x;
float invG = 1.0 / max(g, 0.001);

// Smooth contour instead of raw sin()
float smoothFactor = resolution.y * 0.0125;
float rings = clamp(cos(dist * freq * TWO_PI) * invG * smoothFactor, 0.0, 1.0);
```

For smooth Voronoi, gradient can be approximated analytically from accumulated vectors, avoiding extra distance calculations.

### Organic Flow Displacement

**Source**: Git history commit `9e9cd23` (old mode 1)

```glsl
// Scale displacement by distance from center (smooth falloff)
displacement = mr * strength * smoothstep(0.0, edgeFalloff, length(mr));
```

Creates flowing displacement where pixels near cell centers move more, edges move less.

### Edge Glow

**Source**: Git history commit `d07d542` (original voronoi)

```glsl
// Smooth brightness falloff from edges toward centers
float glow = 1.0 - smoothstep(0.0, edgeFalloff, borderDist);
color += glow * cellColor * edgeGlowIntensity;
```

### Fix Iso/Detect Transparency

Current (additive - shows content through):
```glsl
color += rings * cellColor * intensity;
```

Fixed (blend - replaces content):
```glsl
color = mix(color, rings * cellColor, intensity);
// Or for more opacity:
color = mix(color, cellColor, rings * intensity);
```

---

## Phase 1: Shader Core

**Goal**: Implement smooth Voronoi algorithm and smooth contours in shader.

**Build**:
- Add `uniform bool smooth;` to `shaders/voronoi.fs`
- Create `smoothVoronoi()` function that accumulates inverse distance powers
- When `smooth` enabled, use smooth distance field for all effects
- Add gradient-based anti-aliasing for Edge Iso and Center Iso when smooth enabled
- Test both modes render correctly

**Done when**: Shader compiles, smooth toggle produces bubbly cells, iso rings are anti-aliased.

---

## Phase 2: Effect Swap

**Goal**: Replace Edge Darken and Angle Shade with Organic Flow and Edge Glow.

**Build**:
- In `shaders/voronoi.fs`:
  - Remove `edgeDarkenIntensity` and `angleShadeIntensity` uniforms
  - Remove their effect blocks
  - Add `organicFlowIntensity` and `edgeGlowIntensity` uniforms
  - Add Organic Flow displacement logic (center-falloff)
  - Add Edge Glow effect logic (edge brightness falloff)
- Keep UV Distort as-is (hard displacement)

**Done when**: Organic Flow creates smooth flowing displacement, Edge Glow creates edge brightness.

---

## Phase 3: Fix Transparency

**Goal**: Edge Iso, Center Iso, Edge Detect blend with content instead of showing through.

**Build**:
- In `shaders/voronoi.fs`:
  - Change Edge Iso from additive to mix-blend
  - Change Center Iso from additive to mix-blend
  - Change Edge Detect from additive to mix-blend
- Verify visual result matches expectation (voronoi pattern visible, not transparent overlay)

**Done when**: Iso and detect effects show solid voronoi pattern, not content underneath.

---

## Phase 4: Config and Pipeline

**Goal**: Update config struct and shader uniform binding.

**Build**:
- In `src/config/voronoi_config.h`:
  - Add `bool smooth = false;`
  - Remove `edgeDarkenIntensity`, `angleShadeIntensity`
  - Add `organicFlowIntensity = 0.0f`, `edgeGlowIntensity = 0.0f`
- In `src/render/shader_setup.cpp`:
  - Bind `smooth` uniform
  - Update intensity uniform names (remove old, add new)

**Done when**: Config struct matches shader uniforms, no compilation errors.

---

## Phase 5: UI and Serialization

**Goal**: Update UI grid and preset serialization.

**Build**:
- In `src/ui/imgui_effects_transforms.cpp`:
  - Add smooth checkbox after Speed slider
  - Replace "Darken" button with "Organic" (organicFlowIntensity)
  - Replace "Angle" button with "Glow" (edgeGlowIntensity)
  - Update modulation parameter names in IntensityToggleButton calls
- In `src/config/preset.cpp`:
  - Remove edgeDarkenIntensity, angleShadeIntensity serialization
  - Add smooth, organicFlowIntensity, edgeGlowIntensity serialization
- In `src/automation/param_registry.cpp`:
  - Remove old parameter registrations
  - Add new parameter registrations

**Done when**: UI shows smooth toggle and new effect buttons, presets save/load correctly.

---

## Phase 6: Preset Migration and Docs

**Goal**: Update existing presets, update research doc.

**Build**:
- Update all preset JSON files in `presets/`:
  - Remove edgeDarkenIntensity, angleShadeIntensity fields
  - Add smooth, organicFlowIntensity, edgeGlowIntensity with sensible defaults
- Update `docs/research/voronoi-shaders.md`:
  - Document smooth Voronoi algorithm
  - Document smooth contours technique
  - Update effect summary table
- Update `docs/effects.md` if voronoi section exists

**Done when**: All presets load without errors, documentation reflects new capabilities.
