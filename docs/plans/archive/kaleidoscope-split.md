# Kaleidoscope Split

Split the bundled Kaleidoscope effect into four focused, single-purpose effects. Create a new Cellular category for grid-based effects.

## Current State

The kaleidoscope effect bundles four techniques with intensity sliders:
- `shaders/kaleidoscope.fs` - Single shader with `samplePolar`, `sampleKIFS`, `sampleIterMirror`, `sampleHexFold` functions blended by intensity
- `src/config/kaleidoscope_config.h:4-32` - Single config struct with 20+ fields mixing shared and technique-specific params
- `src/ui/imgui_effects_transforms.cpp:30-118` - UI with technique toggle buttons and blend mix sliders
- 5 presets use kaleidoscope, 3 have modulation routes on `kaleidoscope.twistAngle`

The shared parameters (`segments`, `rotation`, `twist`) have different semantics per technique: Polar `segments` = wedge count, Iterative Mirror `segments` = iteration count. Splitting removes this confusion.

## Technical Implementation

### Kaleidoscope (Polar) - Smooth Folding

**Source**: Mercury hg_sdf, `docs/research/kaleidoscope-new-techniques.md:36-65`

Add `smoothing` parameter for soft wedge edges using polynomial smooth absolute:

```glsl
// Polynomial smooth min
float pmin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

float pabs(float a, float k) {
    return -pmin(a, -a, k);
}

// In samplePolar():
float segmentAngle = TWO_PI / float(segments);
float halfSeg = PI / float(segments);
float c = floor((angle + halfSeg) / segmentAngle);
angle = mod(angle + halfSeg, segmentAngle) - halfSeg;
angle *= mod(c, 2.0) * 2.0 - 1.0;  // flip alternating segments

// Smooth fold (smoothing=0 gives hard edge)
float sa = halfSeg - pabs(halfSeg - abs(angle), smoothing);
angle = sign(angle) * sa;
```

| Parameter | Range | Effect |
|-----------|-------|--------|
| smoothing | 0.0-0.5 | Blend width at wedge seams. 0 = hard edge (current behavior) |

### Lattice Fold - Cell Type Support

**Source**: `docs/research/kaleidoscope-new-techniques.md:84-176`

Add `cellType` parameter supporting triangular, square, and hexagonal tilings:

```glsl
uniform int cellType;  // 3=triangle, 4=square, 6=hexagon

// Hexagonal (existing)
const vec2 HEX_S = vec2(1.0, 1.7320508);
vec4 getHex(vec2 p) {
    vec4 hC = floor(vec4(p, p - vec2(0.5, 1.0)) / HEX_S.xyxy) + 0.5;
    vec4 h = vec4(p - hC.xy * HEX_S, p - (hC.zw + 0.5) * HEX_S);
    return dot(h.xy, h.xy) < dot(h.zw, h.zw)
        ? vec4(h.xy, hC.xy) : vec4(h.zw, hC.zw + 0.5);
}

// Square
vec4 getSquare(vec2 p, float scale) {
    vec2 cell = floor(p * scale);
    vec2 local = fract(p * scale) - 0.5;
    local = abs(local);  // 4-fold symmetry
    return vec4(local, cell);
}

// Triangle (skewed coordinates)
vec4 getTriangle(vec2 p, float scale) {
    const float SQRT3 = 1.7320508;
    vec2 skew = vec2(p.x - p.y * 0.5, p.y * SQRT3 * 0.5) * scale;
    vec2 cell = floor(skew);
    vec2 local = fract(skew);
    // Determine which triangle within parallelogram
    bool upper = (local.x + local.y) > 1.0;
    if (upper) { local = 1.0 - local; cell += 1.0; }
    return vec4(local - 0.333, cell);
}
```

| Parameter | Values | Effect |
|-----------|--------|--------|
| cellType | 3 | Triangle tiling - 60deg angles, densest packing |
| cellType | 4 | Square tiling - 90deg angles, grid pattern |
| cellType | 6 | Hexagon tiling - 120deg angles, honeycomb |

---

## Phase 1: Config Layer

**Goal**: Create config structs for the 4 split effects.

**Build**:
- Create `src/config/kifs_config.h` with: `enabled`, `iterations`, `segments`, `scale`, `offsetX`, `offsetY`, `rotationSpeed`, `twistAngle`
- Create `src/config/iterative_mirror_config.h` with: `enabled`, `iterations` (renamed from segments), `rotationSpeed`, `twistAngle`
- Create `src/config/lattice_fold_config.h` with: `enabled`, `cellType`, `cellScale` (renamed from hexScale), `rotationSpeed`
- Modify `src/config/kaleidoscope_config.h`: Strip to Polar-only fields, add `smoothing`, remove intensity sliders and technique-specific params
- Modify `src/config/effect_config.h`:
  - Add includes for 3 new config headers
  - Add `TRANSFORM_KIFS`, `TRANSFORM_ITERATIVE_MIRROR`, `TRANSFORM_LATTICE_FOLD` to enum before `TRANSFORM_EFFECT_COUNT`
  - Add names in `TransformEffectName()` switch
  - Add default order entries in `TransformOrderConfig::order[]`
  - Add `KifsConfig kifs`, `IterativeMirrorConfig iterativeMirror`, `LatticeFoldConfig latticeFold` to `EffectConfig`

**Done when**: Project compiles with new configs accessible via `effects.kifs`, `effects.iterativeMirror`, `effects.latticeFold`.

---

## Phase 2: Shaders

**Goal**: Split kaleidoscope.fs into 4 focused shader files.

**Build**:
- Create `shaders/kifs.fs`:
  - Extract `sampleKIFS()` function (lines 137-167)
  - Keep `rotate2d()`, `mirror()` utilities
  - Uniforms: `segments`, `rotation`, `time`, `twistAngle`, `iterations`, `scale`, `kifsOffset`
- Create `shaders/iterative_mirror.fs`:
  - Extract `sampleIterMirror()` function (lines 172-196)
  - Keep `mirror()` utility
  - Uniforms: `iterations`, `rotation`, `time`, `twistAngle`
- Create `shaders/lattice_fold.fs`:
  - Extract `sampleHexFold()`, `getHex()` functions
  - Add `getSquare()`, `getTriangle()` from research doc
  - Add `cellType` uniform and dispatch
  - Uniforms: `cellType`, `cellScale`, `rotation`, `time`
- Modify `shaders/kaleidoscope.fs`:
  - Keep only `samplePolar()` function (lines 101-132)
  - Add `pmin()`, `pabs()` from research doc
  - Add `smoothing` uniform
  - Modify fold logic to use smooth folding when smoothing > 0
  - Remove all other technique code and uniforms

**Done when**: All 4 shaders compile independently. Can test by manually loading each shader.

---

## Phase 3: Render Integration

**Goal**: Wire new shaders into the render pipeline.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader kifsShader`, `iterativeMirrorShader`, `latticeFoldShader`
  - Add uniform location members for each shader (follow existing `kaleido*Loc` pattern)
  - Add `float currentKifsRotation`, `currentIterMirrorRotation`, `currentLatticeFoldRotation` for CPU accumulation
  - Remove technique intensity uniforms from kaleidoscope
  - Add `kaleidoSmoothingLoc` uniform
- Modify `src/render/post_effect.cpp`:
  - Load 3 new shaders in `LoadPostEffectShaders()`
  - Cache uniform locations in `GetShaderUniformLocations()`
  - Add to shader validation check
  - Unload in `PostEffectUninit()`
- Modify `src/render/shader_setup.h`:
  - Declare `SetupKifs()`, `SetupIterativeMirror()`, `SetupLatticeFold()`
- Modify `src/render/shader_setup.cpp`:
  - Add 3 dispatch entries in `GetTransformEffect()` switch
  - Implement setup functions for each new shader
  - Simplify `SetupKaleido()` to Polar-only params
- Modify `src/render/render_pipeline.cpp`:
  - Add rotation accumulation for 3 new effects (same pattern as existing kaleidoscope)

**Done when**: Each effect renders when enabled. Visual output matches original kaleidoscope technique behavior.

---

## Phase 4: Parameter Registration

**Goal**: Register modulation parameters for the new effects.

**Build**:
- Modify `src/automation/param_registry.cpp`:
  - Add entries to `PARAM_TABLE[]`:
    - `kifs.rotationSpeed`, `kifs.twistAngle`
    - `iterativeMirror.rotationSpeed`, `iterativeMirror.twistAngle`
    - `latticeFold.rotationSpeed`, `latticeFold.cellScale`
    - `kaleidoscope.smoothing` with range `{0.0f, 0.5f}`
  - Add corresponding targets in `ParamRegistryInit()`
  - Remove old kaleidoscope intensity params (`polarIntensity`, `kifsIntensity`, `iterMirrorIntensity`, `hexFoldIntensity`, `hexScale`)

**Done when**: New params appear in modulation UI. Modulation routes bind and work correctly.

---

## Phase 5: Serialization

**Goal**: Add JSON serialization for new effect configs.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macros for `KifsConfig`, `IterativeMirrorConfig`, `LatticeFoldConfig`
  - Update `KaleidoscopeConfig` macro to match new Polar-only fields
  - In `to_json(EffectConfig)`: Add conditional serialization for new effects
  - In `from_json(EffectConfig)`: Add `j.value()` calls for new effects

**Done when**: Presets save/load correctly with new effects. Old presets load without crash (new effects default to disabled).

---

## Phase 6: UI

**Goal**: Create UI sections for split effects and Cellular category.

**Build**:
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add section state bools: `sectionKifs`, `sectionIterativeMirror`, `sectionLatticeFold`
  - In `DrawSymmetryCategory()`:
    - Replace technique toggle buttons with simple Kaleidoscope section (segments, spin, twist, smoothing)
    - Add KIFS section with: iterations, segments, scale, offset X/Y, rotation, twist
    - Add Iterative Mirror section with: iterations, rotation, twist
  - Create `DrawCellularCategory()`:
    - Move Voronoi section from `DrawWarpCategory()`
    - Add Lattice Fold section with: cellType dropdown (Triangle/Square/Hex), cellScale, rotation
- Modify `src/ui/imgui_effects_transforms.h`:
  - Declare `DrawCellularCategory()`
- Modify `src/ui/imgui_effects.cpp`:
  - Add `DrawCellularCategory()` call after `DrawWarpCategory()`

**Done when**: Each effect has its own UI section. Cellular category contains Voronoi and Lattice Fold.

---

## Phase 7: Preset Migration

**Goal**: Update existing presets to use split effects.

**Build**:
- Manually edit each preset file that uses kaleidoscope (5 presets: GLITCHYBOB, SMOOTHBOB, WOBBYBOB, WINNY, BINGBANG):
  - Map `polarIntensity > 0` -> set `kaleidoscope.enabled = true`
  - Map `kifsIntensity > 0` -> set `kifs.enabled = true` with technique params
  - Map `iterMirrorIntensity > 0` -> set `iterativeMirror.enabled = true`
  - Map `hexFoldIntensity > 0` -> set `latticeFold.enabled = true, cellType = 6`
  - Update modulation routes from `kaleidoscope.*` to appropriate effect prefix
- Move `docs/research/kaleidoscope-split.md` to `docs/plans/archive/`

**Done when**: All existing presets load and produce expected visuals. No console warnings about missing parameters.

---

## Phase 8: Verification

**Goal**: Comprehensive testing of split effects.

**Build**:
- Test each effect in isolation:
  - Kaleidoscope: Verify smoothing=0 matches original Polar behavior. Test smoothing > 0 for soft edges.
  - KIFS: Verify fractal folding matches original. Test iteration count variations.
  - Iterative Mirror: Verify iteration count matches old segment behavior.
  - Lattice Fold: Test cellType 3/4/6 produce triangle/square/hex patterns.
- Test transform order: Verify effects execute in user-defined order
- Test modulation: Verify routes work with new param IDs
- Test preset save/load cycle
- Run `/lint` to catch any clang-tidy warnings

**Done when**: All effects work correctly. No regressions from original kaleidoscope behavior.
