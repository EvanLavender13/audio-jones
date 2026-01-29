# Disco Ball (Phase 2: Background Light Projection)

Extends the disco ball effect to project scattered light spots onto the background area outside the sphere. Facets reflect input texture brightness as projected spots.

**Prerequisite**: `docs/plans/disco-ball.md` must be implemented first.

## Current State

After Phase 1 implementation:
- `src/config/disco_ball_config.h` - Config struct exists
- `shaders/disco_ball.fs` - Sphere rendering works
- UI, presets, and modulation already wired

## Technical Implementation

**Source**: `docs/research/disco-ball.md` (Background Light Projection section)

### Facet Iteration for Light Spots

For pixels outside the sphere, iterate front-facing facets and project light based on input texture brightness:

```glsl
vec3 bgColor = vec3(0.0);

// Subsample facets for performance (e.g., 16x8 instead of full grid)
for (int lat = 0; lat < latSteps; lat++) {
    for (int lon = 0; lon < lonSteps; lon++) {
        // Facet center in spherical coords
        float theta = (float(lat) + 0.5) / float(latSteps) * PI;
        float phi = (float(lon) + 0.5) / float(lonSteps) * TAU + sphereAngle;

        // Facet position and normal (sphere centered at origin)
        vec3 facetNormal = vec3(
            sin(theta) * cos(phi),
            cos(theta),
            sin(theta) * sin(phi)
        );

        // Skip back-facing facets (facing away from camera at +Z)
        if (facetNormal.z < 0.0) continue;

        // What does this facet reflect from input texture?
        vec3 facetRefl = reflect(vec3(0, 0, 1), facetNormal);
        vec2 facetUV = facetRefl.xy * 0.5 + 0.5;
        vec3 reflectedColor = texture(inputTex, facetUV).rgb;
        float brightness = dot(reflectedColor, vec3(0.299, 0.587, 0.114));

        // Direction from sphere center to this background pixel
        vec3 toPixel = normalize(vec3(uv - 0.5, 0.5));

        // Alignment: does facet point toward this pixel?
        float alignment = dot(facetNormal, toPixel);

        if (alignment > spotCutoff && brightness > brightnessThreshold) {
            float spot = pow((alignment - spotCutoff) / (1.0 - spotCutoff), spotFalloff);
            bgColor += reflectedColor * spot * spotIntensity;
        }
    }
}
```

### Performance Considerations

Full facet iteration at 50x25 = 1250 facets per pixel is expensive. Options:

1. **Subsample**: Use 16x8 = 128 facets (10x fewer iterations)
2. **Early-out**: Skip back-facing facets (roughly 50% culled)
3. **Threshold**: Only bright facets project light

With subsampling and early-out, expect ~64 iterations per background pixel.

## New Parameters

| Parameter | Range | Default | Purpose |
|-----------|-------|---------|---------|
| spotIntensity | 0.0 - 3.0 | 1.0 | Background light spot brightness |
| spotSize | 0.8 - 0.99 | 0.95 | Cutoff angle (higher = smaller, sharper spots) |
| spotFalloff | 1.0 - 8.0 | 4.0 | Spot edge softness (higher = softer) |
| brightnessThreshold | 0.0 - 0.5 | 0.1 | Minimum input brightness to project |

---

## Phase 1: Extend Config

**Goal**: Add light projection parameters to config.
**Files**: `src/config/disco_ball_config.h`

**Build**:
- Add fields to `DiscoBallConfig`:
  - `float spotIntensity = 1.0f;`
  - `float spotSize = 0.95f;`
  - `float spotFalloff = 4.0f;`
  - `float brightnessThreshold = 0.1f;`

**Verify**: `cmake.exe --build build` → compiles without errors.

**Done when**: Config struct has all light projection fields.

---

## Phase 2: Shader Enhancement

**Goal**: Add background light projection to shader.
**Files**: `shaders/disco_ball.fs`

**Build**:
- Add new uniforms: `spotIntensity`, `spotSize`, `spotFalloff`, `brightnessThreshold`
- Add constants: `const int LAT_STEPS = 16; const int LON_STEPS = 8;`
- After sphere miss check (`t < 0.0`), add facet iteration loop:
  - Loop over subsampled lat/lon grid
  - Skip back-facing facets (`facetNormal.z < 0.0`)
  - Sample reflected color, compute brightness
  - Compute alignment between facet normal and pixel direction
  - Accumulate spot contribution if alignment > spotSize and brightness > threshold
- Mix background light with original texture color

**Verify**: Build, enable disco ball with `spotIntensity > 0`, light spots appear outside sphere.

**Done when**: Background shows moving light spots that match ball facet reflections.

---

<!-- CHECKPOINT: Shader works, needs UI/serialization -->

## Phase 3: PostEffect Uniforms

**Goal**: Add uniform locations for new parameters.
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Build**:
- Add to `PostEffect` struct:
  - `int discoBallSpotIntensityLoc;`
  - `int discoBallSpotSizeLoc;`
  - `int discoBallSpotFalloffLoc;`
  - `int discoBallBrightnessThresholdLoc;`
- Get locations in `GetShaderUniformLocations()`

**Verify**: `cmake.exe --build build` → compiles without errors.

**Done when**: All new uniform locations cached.

---

## Phase 4: Shader Setup Extension

**Goal**: Bind new uniforms in setup function.
**Files**: `src/render/shader_setup.cpp`

**Build**:
- Extend `SetupDiscoBall()` to set:
  - `spotIntensity`
  - `spotSize`
  - `spotFalloff`
  - `brightnessThreshold`

**Verify**: Build, uniforms reach shader (light spots respond to parameter changes).

**Done when**: Setup function passes all light projection uniforms.

---

## Phase 5: UI Extension

**Goal**: Add light projection controls to UI.
**Files**: `src/ui/imgui_effects_cellular.cpp`

**Build**:
- In `DrawCellularDiscoBall()`, add collapsible section for light projection:
  ```cpp
  if (TreeNodeAccented("Light Spots##disco", categoryGlow)) {
      ModulatableSlider("Intensity##spot", &cfg->spotIntensity, "discoBall.spotIntensity", "%.2f", modSources);
      ModulatableSlider("Size##spot", &cfg->spotSize, "discoBall.spotSize", "%.2f", modSources);
      ModulatableSlider("Falloff##spot", &cfg->spotFalloff, "discoBall.spotFalloff", "%.1f", modSources);
      ModulatableSlider("Threshold##spot", &cfg->brightnessThreshold, "discoBall.brightnessThreshold", "%.2f", modSources);
      TreeNodeAccentedPop();
  }
  ```

**Verify**: UI shows Light Spots section, sliders control projection.

**Done when**: All light parameters adjustable in UI.

---

## Phase 6: Preset Extension

**Goal**: Serialize new parameters.
**Files**: `src/config/preset.cpp`

**Build**:
- Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `DiscoBallConfig` to include:
  `spotIntensity, spotSize, spotFalloff, brightnessThreshold`

**Verify**: Save preset with light settings, reload, values preserved.

**Done when**: All light parameters serialize correctly.

---

## Phase 7: Parameter Registration Extension

**Goal**: Enable modulation of light parameters.
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add to `PARAM_TABLE`:
  ```cpp
  {"discoBall.spotIntensity",       {0.0f, 3.0f}},
  {"discoBall.spotSize",            {0.8f, 0.99f}},
  {"discoBall.spotFalloff",         {1.0f, 8.0f}},
  {"discoBall.brightnessThreshold", {0.0f, 0.5f}},
  ```
- Add matching pointers to `targets[]` array

**Verify**: LFO routes to `discoBall.spotIntensity` modulate light brightness.

**Done when**: All light parameters respond to modulation.

---

<!-- CHECKPOINT: Feature complete -->

## Verification Checklist

- [ ] Light spots appear outside sphere when spotIntensity > 0
- [ ] Spots move/rotate with ball rotation
- [ ] Spots reflect bright areas of input texture
- [ ] spotSize controls spot angular spread
- [ ] spotFalloff controls spot edge softness
- [ ] brightnessThreshold filters dim facets
- [ ] Performance acceptable (no major frame drops)
- [ ] Preset save/load preserves all new settings
- [ ] All new parameters respond to modulation

## Performance Notes

If frame rate drops significantly:
1. Reduce `LAT_STEPS`/`LON_STEPS` constants in shader
2. Increase `brightnessThreshold` to skip more facets
3. Consider making step counts configurable parameters

## Post-Implementation Notes

### Removed: `spotSize` parameter (2026-01-29)

**Reason**: Parameter did nothing useful. Light spot size should derive from ball's `tileSize`, not a separate control.

**Changes**:
- Removed `spotSize` from config, shader, UI, presets, param registry
- Shader now uses `tileSize` to determine facet grid for light projection

### Changed: Light projection algorithm (2026-01-29)

**Reason**: Original per-pixel facet lookup created rigid radial patterns instead of scattered discrete spots like real disco balls.

**Changes**:
- `shaders/disco_ball.fs`: Replaced facet lookup with facet iteration approach
  - Iterates facets based on `tileSize` grid (matches ball rendering)
  - Projects each facet's reflection to background plane
  - Draws discrete spots with gaussian falloff at projected positions
  - Spots accumulate/overlap naturally
- Spot radius derived from `tileSize * sphereRadius * 0.8`
- `spotFalloff` controls gaussian spread (renamed to "Softness" in UI)
