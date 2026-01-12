# Polar Fold Mode

Add a `polarFold` toggle to KIFS and Sine Warp effects. When enabled, UV coordinates convert to polar space and fold into a wedge before the main effect iteration. Recreates the radial symmetry that KIFS had when bundled with kaleidoscope.

## Current State

- `shaders/kifs.fs:40-46` - KIFS uses Cartesian `abs(p)` folding
- `shaders/sine_warp.fs:31-33` - Sine warp uses Cartesian `sin(p.x)` / `sin(p.y)` perturbations
- `shaders/kaleidoscope.fs:83-111` - Reference polar fold implementation (segments, angle mod, mirror)
- `src/config/kifs_config.h:6-15` - Current KIFS config (no polar fold)
- `src/config/sine_warp_config.h:6-13` - Current Sine Warp config (no polar fold)

## Technical Implementation

**Source**: `shaders/kaleidoscope.fs:83-111`, `docs/research/kifs-correct.md:21-27`

### Polar Fold Algorithm

Convert Cartesian to polar, fold angle into segment, convert back:

```glsl
const float TWO_PI = 6.28318530718;
const float PI = 3.14159265359;

vec2 polarFold(vec2 p, int segments) {
    float radius = length(p);
    float angle = atan(p.y, p.x);

    // Fold angle into segment
    float segmentAngle = TWO_PI / float(segments);
    float halfSeg = PI / float(segments);
    float c = floor((angle + halfSeg) / segmentAngle);
    angle = mod(angle + halfSeg, segmentAngle) - halfSeg;
    angle *= mod(c, 2.0) * 2.0 - 1.0;  // Mirror alternating segments

    return vec2(cos(angle), sin(angle)) * radius;
}
```

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| polarFold | bool | - | Enable polar pre-fold |
| polarFoldSegments | int | 2-12 | Wedge count for polar fold |

### KIFS Integration Point

Insert polar fold at `kifs.fs:35` (after initial rotation, before iteration loop):

```glsl
p = rotate2d(p, rotation);  // existing line 36

// NEW: Polar fold before KIFS iteration
if (polarFold) {
    p = polarFold(p, polarFoldSegments);
}

for (int i = 0; i < iterations; i++) {  // existing line 39
```

### Sine Warp Integration Point

Insert polar fold at `sine_warp.fs:25` (after centering, before octave loop):

```glsl
vec2 p = (fragTexCoord - 0.5) * 2.0;  // existing line 25

// NEW: Polar fold before turbulence cascade
if (polarFold) {
    p = polarFold(p, polarFoldSegments);
}

for (int i = 0; i < octaves; i++) {  // existing line 27
```

---

## Phase 1: Config Layer

**Goal**: Add polarFold fields to KIFS and Sine Warp configs.

**Build**:
- Modify `src/config/kifs_config.h`:
  - Add `bool polarFold = false;`
  - Add `int polarFoldSegments = 6;`
- Modify `src/config/sine_warp_config.h`:
  - Add `bool polarFold = false;`
  - Add `int polarFoldSegments = 6;`

**Done when**: Project compiles with new config fields accessible.

---

## Phase 2: Shader Updates

**Goal**: Implement polar fold in KIFS and Sine Warp shaders.

**Build**:
- Modify `shaders/kifs.fs`:
  - Add `uniform bool polarFold;` and `uniform int polarFoldSegments;`
  - Add `polarFold()` helper function (from Technical Implementation above)
  - Insert fold call after line 36 (after initial rotation), before iteration loop
- Modify `shaders/sine_warp.fs`:
  - Add `uniform bool polarFold;` and `uniform int polarFoldSegments;`
  - Add `polarFold()` helper function
  - Insert fold call after line 25 (after centering), before octave loop

**Done when**: Both shaders compile. Polar fold visibly affects output when uniforms set manually.

---

## Phase 3: Render Integration

**Goal**: Wire new uniforms through the render pipeline.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `int kifsPolarFoldLoc;` and `int kifsPolarFoldSegmentsLoc;`
  - Add `int sineWarpPolarFoldLoc;` and `int sineWarpPolarFoldSegmentsLoc;`
- Modify `src/render/post_effect.cpp`:
  - Cache uniform locations in `GetShaderUniformLocations()` for both shaders
- Modify `src/render/shader_setup.cpp`:
  - In `SetupKifs()`: Add `SetShaderValue()` calls for `polarFold` (as int) and `polarFoldSegments`
  - In `SetupSineWarp()`: Add `SetShaderValue()` calls for `polarFold` (as int) and `polarFoldSegments`

**Done when**: Changing config values affects shader output at runtime.

---

## Phase 4: UI

**Goal**: Add polar fold controls to KIFS and Sine Warp UI sections.

**Build**:
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - In KIFS section (~line 95): Add `ImGui::Checkbox("Polar Fold##kifs", &k->polarFold);`
  - When polarFold enabled: Add `ImGui::SliderInt("Segments##kifsPolar", &k->polarFoldSegments, 2, 12);`
  - In Sine Warp section: Add same controls with `##sineWarp` suffix

**Done when**: UI shows polar fold checkbox. Segments slider appears when enabled.

---

## Phase 5: Serialization & Registration

**Goal**: Enable preset save/load and modulation for new parameters.

**Build**:
- Modify `src/config/preset.cpp`:
  - Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `KifsConfig` to include `polarFold`, `polarFoldSegments`
  - Update same macro for `SineWarpConfig`
- Modify `src/automation/param_registry.cpp`:
  - Add `kifs.polarFoldSegments` entry (range 2-12)
  - Add `sineWarp.polarFoldSegments` entry (range 2-12)

**Done when**: Presets save/load polar fold settings. Segments appear in modulation UI.

---

## Phase 6: Verification

**Goal**: Validate polar fold behavior matches old radial KIFS.

**Build**:
- Test KIFS with polarFold enabled:
  - Verify 6 segments creates hexagonal radial symmetry
  - Verify 4 segments creates quadrant symmetry
  - Verify polarFold + octantFold combination works
- Test Sine Warp with polarFold enabled:
  - Verify creates concentric ripple patterns
  - Verify segments affects wedge count
- Test preset round-trip: save, reload, verify settings preserved
- Run build and fix any warnings

**Done when**: Both effects work correctly with polar fold. No regressions in non-polar mode.

---

## Implementation Notes

### Bug Fix: Drawable config not saving to preset

**Issue**: Drawable `rotationSpeed` ("Spin" in UI) reverted to 0 when saving presets.

**Root cause**: `DrawableParamsUnregister()` removed modulation routes but left stale param entries in `sParams`. When a new drawable occupied the same array slot, both old and new param IDs pointed to the same memory. `ModEngineWriteBaseValues()` wrote all bases in undefined order, potentially overwriting the correct value with 0.

**Fix** (`modulation_engine.cpp`, `drawable_params.cpp`):
- Added `ModEngineRemoveParamsMatching()` to remove param entries by prefix
- Updated `DrawableParamsUnregister()` to call both `ModEngineRemoveRoutesMatching()` and `ModEngineRemoveParamsMatching()`

### Scope Change: Removed Sine Warp polar fold

**Issue**: Polar fold on Sine Warp transformed it into a symmetry effect rather than enhancing its organic turbulence. The Cartesian sine cascades fight against radial wedge folding.

**Resolution**: Removed polar fold from Sine Warp, keeping it only on KIFS where folding is core to the effect. Created `docs/plans/radial-pulse.md` as a research stub for a future polar-native sine distortion effect that works in polar coordinates from the start.
