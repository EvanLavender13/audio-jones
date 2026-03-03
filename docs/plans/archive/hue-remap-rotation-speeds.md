# Hue Remap Rotation Speeds

Add rotation speed parameters to hue remap's angular and linear spatial fields so the masking/shifting patterns spin over time instead of remaining static. Four new `*Speed` config fields accumulate angular offsets on the CPU and pass them as shader uniforms.

**Research**: `docs/research/hue-remap-rotation-speeds.md`

## Design

### Types

**HueRemapConfig** — add 4 fields after the existing angular/linear fields in each section:

```
float blendAngularSpeed = 0.0f;  // Blend angular rotation rate (rad/s)
float blendLinearSpeed = 0.0f;   // Blend linear rotation rate (rad/s)
float shiftAngularSpeed = 0.0f;  // Shift angular rotation rate (rad/s)
float shiftLinearSpeed = 0.0f;   // Shift linear rotation rate (rad/s)
```

Place each speed field directly after its corresponding angle/freq field:
- `blendAngularSpeed` after `blendAngularFreq`
- `blendLinearSpeed` after `blendLinearAngle`
- `shiftAngularSpeed` after `shiftAngularFreq`
- `shiftLinearSpeed` after `shiftLinearAngle`

**HueRemapEffect** — add 4 accumulators and 4 uniform locations:

```
float blendAngularAccum;   // CPU-accumulated blend angular offset
float blendLinearAccum;    // CPU-accumulated blend linear offset
float shiftAngularAccum;   // CPU-accumulated shift angular offset
float shiftLinearAccum;    // CPU-accumulated shift linear offset
int blendAngularOffsetLoc;
int blendLinearOffsetLoc;
int shiftAngularOffsetLoc;
int shiftLinearOffsetLoc;
```

Place accumulators grouped after the existing `time` field. Place uniform locations grouped after `timeLoc`.

**HUE_REMAP_CONFIG_FIELDS** — add the 4 speed fields to the macro.

### Algorithm

**CPU (Setup function)**:
```
e->blendAngularAccum += cfg->blendAngularSpeed * deltaTime;
e->blendLinearAccum  += cfg->blendLinearSpeed  * deltaTime;
e->shiftAngularAccum += cfg->shiftAngularSpeed * deltaTime;
e->shiftLinearAccum  += cfg->shiftLinearSpeed  * deltaTime;
```

Then bind each accumulator as a uniform float.

**Shader** — 4 new uniforms:
```glsl
uniform float blendAngularOffset;
uniform float blendLinearOffset;
uniform float shiftAngularOffset;
uniform float shiftLinearOffset;
```

In `main()`, compute offset-adjusted values before calling the spatial field functions:
```glsl
float angBlend = ang + blendAngularOffset;
float angShift = ang + shiftAngularOffset;
float blendLinearAngleAdj = blendLinearAngle + blendLinearOffset;
float shiftLinearAngleAdj = shiftLinearAngle + shiftLinearOffset;
```

Pass `angBlend` to `computeSpatialField` and `blendLinearAngleAdj` for its linear angle. Pass `angShift` to `computeShiftField` and `shiftLinearAngleAdj` for its linear angle.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| blendAngularSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Yes | Angular Spin##hueremap_blend |
| blendLinearSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Yes | Linear Spin##hueremap_blend |
| shiftAngularSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Yes | Angular Spin##hueremap_shift |
| shiftLinearSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Yes | Linear Spin##hueremap_shift |

### Constants

No new enums or constants needed. Uses existing `ROTATION_SPEED_MAX` from `src/config/constants.h`.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Add speed fields and accumulators to header

**Files**: `src/effects/hue_remap.h`
**Creates**: Config fields and effect struct members that `.cpp` and `.fs` depend on

**Do**:
1. Add `blendAngularSpeed` after `blendAngularFreq` with range comment `// Blend angular rotation rate (-PI to PI rad/s)`
2. Add `blendLinearSpeed` after `blendLinearAngle` with range comment `// Blend linear rotation rate (-PI to PI rad/s)`
3. Add `shiftAngularSpeed` after `shiftAngularFreq` with same pattern
4. Add `shiftLinearSpeed` after `shiftLinearAngle` with same pattern
5. Add 4 `float` accumulators to `HueRemapEffect` after `time`: `blendAngularAccum`, `blendLinearAccum`, `shiftAngularAccum`, `shiftLinearAccum`
6. Add 4 `int` uniform locations to `HueRemapEffect` after `timeLoc`: `blendAngularOffsetLoc`, `blendLinearOffsetLoc`, `shiftAngularOffsetLoc`, `shiftLinearOffsetLoc`
7. Add the 4 speed fields to `HUE_REMAP_CONFIG_FIELDS` macro — insert each after its corresponding existing field (e.g., `blendAngularFreq, blendAngularSpeed, blendLinear, ...`)

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: CPU accumulation, uniforms, params, and UI

**Files**: `src/effects/hue_remap.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. **Init**: Add 4 `GetShaderLocation` calls for `"blendAngularOffset"`, `"blendLinearOffset"`, `"shiftAngularOffset"`, `"shiftLinearOffset"`. Initialize all 4 accumulators to `0.0f`.
2. **Setup**: Before the existing uniform binding block, accumulate offsets:
   - `e->blendAngularAccum += cfg->blendAngularSpeed * deltaTime;` (and 3 more)
   - Bind each accumulator with `SetShaderValue(..., SHADER_UNIFORM_FLOAT)` after the blend/shift spatial uniform sections respectively.
3. **RegisterParams**: Add 4 `ModEngineRegisterParam` calls:
   - `"hueRemap.blendAngularSpeed"`, `"hueRemap.blendLinearSpeed"`, `"hueRemap.shiftAngularSpeed"`, `"hueRemap.shiftLinearSpeed"`
   - Bounds: `-ROTATION_SPEED_MAX` to `+ROTATION_SPEED_MAX`
   - Place each in its respective blend/shift param section after the corresponding angle registration.
4. **UI**: Add 4 `ModulatableSliderSpeedDeg` calls in `DrawHueRemapParams`:
   - `"Angular Spin##hueremap_blend"` after the `Angular Freq` slider in Blend Spatial section
   - `"Linear Spin##hueremap_blend"` after the `Linear Angle` slider in Blend Spatial section
   - `"Angular Spin##hueremap_shift"` after the `Angular Freq` slider in Shift Spatial section
   - `"Linear Spin##hueremap_shift"` after the `Linear Angle` slider in Shift Spatial section
   - Pattern: `ModulatableSliderSpeedDeg("Label", &hr->field, "hueRemap.field", ms);`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Shader offset uniforms and adjusted angles

**Files**: `shaders/hue_remap.fs`
**Depends on**: Wave 1 complete

**Do**:
1. Add 4 uniform declarations after the existing shift spatial uniforms block:
   ```glsl
   // Rotation speed offsets (accumulated on CPU)
   uniform float blendAngularOffset;
   uniform float blendLinearOffset;
   uniform float shiftAngularOffset;
   uniform float shiftLinearOffset;
   ```
2. In `main()`, after computing `ang`, compute adjusted angles:
   ```glsl
   float angBlend = ang + blendAngularOffset;
   float angShift = ang + shiftAngularOffset;
   float blendLinearAngleAdj = blendLinearAngle + blendLinearOffset;
   float shiftLinearAngleAdj = shiftLinearAngle + shiftLinearOffset;
   ```
3. In the `computeSpatialField` call (blend): pass `angBlend` instead of `ang`, pass `blendLinearAngleAdj` instead of `blendLinearAngle`.
4. In the `computeShiftField` call (shift): pass `angShift` instead of `ang`, pass `shiftLinearAngleAdj` instead of `shiftLinearAngle`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Default behavior unchanged (all speeds default to 0, no rotation)
- [ ] Setting blendAngularSpeed to nonzero with blendAngular > 0 makes blend mask spin
- [ ] Setting shiftAngularSpeed to nonzero with shiftAngular > 0 makes shift pattern spin
- [ ] Linear speeds rotate the gradient direction over time
- [ ] Speed sliders appear after their corresponding Angular Freq / Linear Angle sliders
- [ ] Preset save/load round-trips all 4 new fields
