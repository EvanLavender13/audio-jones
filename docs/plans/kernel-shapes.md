# Kernel Shapes

Adds shaped aperture kernels to both Bokeh and Phi Blur. Four shapes — Disc (current default), Box, Hex, Star — built from polar radius functions applied to the existing golden-angle Vogel spiral. Configurable star point count and inner radius ratio. Shape rotation parameter on both effects. Phi Blur's old Rect mode is replaced by Box; both effects get identical shape combos for full parity.

**Research**: `docs/research/kernel-shapes.md`

## Design

### Types

**BokehConfig** — add fields:
```
int shape = 0;                // 0=Disc, 1=Box, 2=Hex, 3=Star
float shapeAngle = 0.0f;     // Kernel rotation in radians (0-2pi)
int starPoints = 5;           // Star point count (3-8)
float starInnerRadius = 0.4f; // Star valley depth (0.1-0.9)
```

**BokehEffect** — add uniform locations:
```
int shapeLoc;
int shapeAngleLoc;
int starPointsLoc;
int starInnerRadiusLoc;
```

**PhiBlurConfig** — replace `mode`/`angle`/`aspectRatio` with:
```
int shape = 0;                // 0=Disc, 1=Box, 2=Hex, 3=Star (replaces mode)
float shapeAngle = 0.0f;     // Kernel rotation in radians (replaces angle)
int starPoints = 5;           // Star point count (3-8)
float starInnerRadius = 0.4f; // Star valley depth (0.1-0.9)
```

Remove: `int mode`, `float angle`, `float aspectRatio`

**PhiBlurEffect** — replace `modeLoc`/`angleLoc`/`aspectRatioLoc` with:
```
int shapeLoc;
int shapeAngleLoc;
int starPointsLoc;
int starInnerRadiusLoc;
```

Remove: `modeLoc`, `angleLoc`, `aspectRatioLoc`

### Algorithm

Both shaders share identical shape functions. Each sample in the Vogel spiral has angle `theta = i * GOLDEN_ANGLE` and radius `r = sqrt(i/N) * radius`. Multiply `r` by a polar shape factor `S(theta)` to reshape from disc to any convex shape.

**Shape radius functions (GLSL — add to both shaders):**

```glsl
#define PI 3.141593

// Regular N-gon boundary: inscribed-circle-normalized
// Vertices at sector boundaries, edges at sector midpoints
float ngonRadius(float theta, int n, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle) - halfAngle;
    return cos(halfAngle) / cos(sector);
}

// Star: linear interpolation between outer tips and inner valleys
// Tips at sector=0 (radius=1.0), valleys at sector=halfAngle (radius=innerRatio)
float starRadius(float theta, int n, float innerRatio, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle);
    float t = abs(sector - halfAngle) / halfAngle;
    return mix(innerRatio, 1.0, t);
}
```

**Shape application (inside sampling loop, after computing `r`):**

```glsl
// shape: 0=Disc (no-op), 1=Box, 2=Hex, 3=Star
if (shape == 1)      r *= ngonRadius(theta, 4, shapeAngle);
else if (shape == 2)  r *= ngonRadius(theta, 6, shapeAngle);
else if (shape == 3)  r *= starRadius(theta, starPoints, starInner, shapeAngle);
```

Disc (shape==0) skips multiplication entirely — zero cost, preserves current behavior.

**Phi Blur changes**: Remove the `if (mode == 0)` Rect branch entirely. All modes now use the Vogel spiral (the old Disc branch). Remove `angle`, `aspectRatio`, `mode` uniforms. Add `shape`, `shapeAngle`, `starPoints`, `starInner` uniforms.

### Parameters

**Bokeh** (new fields):

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| shape | int | 0-3 | 0 | No | "Shape" (Combo: Disc/Box/Hex/Star) |
| shapeAngle | float | 0-2pi | 0.0 | Yes | "Shape Angle" (degrees via SliderAngleDeg) |
| starPoints | int | 3-8 | 5 | No | "Star Points" |
| starInnerRadius | float | 0.1-0.9 | 0.4 | Yes | "Inner Radius" |

**Phi Blur** (replacing mode/angle/aspectRatio):

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| shape | int | 0-3 | 0 | No | "Shape" (Combo: Disc/Box/Hex/Star) |
| shapeAngle | float | 0-2pi | 0.0 | Yes | "Shape Angle" (degrees via SliderAngleDeg) |
| starPoints | int | 3-8 | 5 | No | "Star Points" |
| starInnerRadius | float | 0.1-0.9 | 0.4 | Yes | "Inner Radius" |

### Conditional Visibility (UI)

- `shapeAngle`: hidden when shape == 0 (rotating a circle is meaningless)
- `starPoints` and `starInnerRadius`: only visible when shape == 3 (Star)

### Preset Compatibility

Old presets with `mode`/`angle`/`aspectRatio` in Phi Blur will silently ignore those fields on load — new fields get struct defaults (shape=0 Disc). No migration alias needed since Rect mode no longer exists.

---

## Tasks

### Wave 1: Config Headers

#### Task 1.1: BokehConfig shape fields

**Files**: `src/effects/bokeh.h`
**Creates**: Shape fields in BokehConfig, uniform locs in BokehEffect

**Do**: Add the four new fields to BokehConfig (shape, shapeAngle, starPoints, starInnerRadius) and four new uniform location ints to BokehEffect, as specified in the Design section.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: PhiBlurConfig shape parity

**Files**: `src/effects/phi_blur.h`
**Creates**: Unified shape fields replacing mode/angle/aspectRatio

**Do**: Remove `mode`, `angle`, `aspectRatio` from PhiBlurConfig. Add `shape`, `shapeAngle`, `starPoints`, `starInnerRadius` as specified in Design. In PhiBlurEffect, remove `modeLoc`, `angleLoc`, `aspectRatioLoc` and add `shapeLoc`, `shapeAngleLoc`, `starPointsLoc`, `starInnerRadiusLoc`.

**Verify**: `cmake.exe --build build` compiles (will have errors from .cpp files — expected, fixed in Wave 2).

---

### Wave 2: Shaders, Effect Modules, UI

#### Task 2.1: Bokeh shader — shape functions

**Files**: `shaders/bokeh.fs`
**Depends on**: Wave 1 complete

**Do**: Add `uniform int shape`, `uniform float shapeAngle`, `uniform int starPoints`, `uniform float starInner` uniforms. Add the `ngonRadius` and `starRadius` GLSL functions from the Algorithm section above the `main()` function. Inside the sampling loop, after computing `r`, apply the shape branching from the Algorithm section. Disc (shape==0) should skip — no multiplication.

**Verify**: Shader compiles (build succeeds).

#### Task 2.2: Phi Blur shader — replace Rect with shapes

**Files**: `shaders/phi_blur.fs`
**Depends on**: Wave 1 complete

**Do**: Remove `uniform int mode`, `uniform float angle`, `uniform float aspectRatio`. Add `uniform int shape`, `uniform float shapeAngle`, `uniform int starPoints`, `uniform float starInner`. Add the same `ngonRadius` and `starRadius` GLSL functions from the Algorithm section. Remove the entire `if (mode == 0)` Rect branch — all sampling now uses the Vogel spiral (the old `else` Disc branch). Apply the same shape branching as bokeh.fs after computing `r`. Remove the `rot` matrix and related `ca`/`sa` variables since Rect mode is gone.

**Verify**: Shader compiles (build succeeds).

#### Task 2.3: Bokeh effect module — uniforms and params

**Files**: `src/effects/bokeh.cpp`
**Depends on**: Wave 1 complete

**Do**: In `BokehEffectInit`, cache the four new uniform locations (`shape`, `shapeAngle`, `starPoints`, `starInner`). In `BokehEffectSetup`, bind the four new uniforms from config fields. In `BokehRegisterParams`, register `bokeh.shapeAngle` (0.0–ROTATION_OFFSET_MAX) and `bokeh.starInnerRadius` (0.1–0.9) as modulatable. Include `config/constants.h` for ROTATION_OFFSET_MAX. Follow existing patterns in the file.

**Verify**: Build succeeds.

#### Task 2.4: Phi Blur effect module — replace mode uniforms

**Files**: `src/effects/phi_blur.cpp`
**Depends on**: Wave 1 complete

**Do**: In `PhiBlurEffectInit`, remove caching of `mode`/`angle`/`aspectRatio` locations. Cache `shape`, `shapeAngle`, `starPoints`, `starInner` locations. In `PhiBlurEffectSetup`, remove binding of mode/angle/aspectRatio. Bind the four new shape uniforms. In `PhiBlurRegisterParams`, remove `phiBlur.angle` and `phiBlur.aspectRatio` registrations. Register `phiBlur.shapeAngle` (0.0–ROTATION_OFFSET_MAX) and `phiBlur.starInnerRadius` (0.1–0.9).

**Verify**: Build succeeds.

#### Task 2.5: Optical UI — shape controls for both effects

**Files**: `src/ui/imgui_effects_optical.cpp`
**Depends on**: Wave 1 complete

**Do**: In `DrawOpticalBokeh`, after the existing Brightness slider, add: Shape combo (Disc/Box/Hex/Star), then conditionally: `shapeAngle` via `ModulatableSliderAngleDeg` (hidden when shape==0), `starPoints` via `ImGui::SliderInt` and `starInnerRadius` via `ModulatableSlider` (both only when shape==3). In `DrawOpticalPhiBlur`, replace the Mode combo with the same Shape combo (Disc/Box/Hex/Star). Remove the `if (p->mode == 0)` block that showed angle/aspectRatio controls. Add the same conditional shape controls as Bokeh. Both sections should have identical shape UI structure for parity. Use `"##bokeh"` and `"##phiBlur"` suffixes respectively.

**Verify**: Build succeeds. Run app, verify shape combos appear for both effects.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Bokeh: Shape combo shows Disc/Box/Hex/Star; Disc preserves original behavior
- [ ] Bokeh: Box/Hex produce visible polygon-shaped blur kernels at high radius
- [ ] Bokeh: Star shows starPoints/innerRadius controls; star shape visible
- [ ] Bokeh: shapeAngle rotates non-disc shapes
- [ ] Phi Blur: Same shape combo, same controls, same visual behavior as Bokeh
- [ ] Phi Blur: Old Rect mode gone — no mode combo, no aspectRatio
- [ ] Old presets load without errors (Phi Blur fields get defaults)
