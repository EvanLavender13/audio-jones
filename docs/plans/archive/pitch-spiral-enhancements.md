# Pitch Spiral Enhancements

Adds perpetual rotation, organic radial breathing, and a parametric shape exponent to the existing pitch spiral generator. Defaults preserve current behavior — zero rotation, zero breath, Archimedean curvature.

**Research**: `docs/research/pitch_spiral_enhancements.md`

## Design

### Types

**PitchSpiralConfig** — add 4 fields after the existing `tiltAngle`:

```
// Animation
float rotationSpeed = 0.0f;  // Spin rate (rad/s), positive = CCW
float breathRate = 1.0f;     // Breathing oscillation frequency (Hz)
float breathDepth = 0.0f;    // Radial expansion amplitude (fraction). 0 = disabled
float shapeExponent = 1.0f;  // Power-law curvature. 1.0 = Archimedean
```

**PitchSpiralEffect** — add 2 accumulators and 4 uniform locations:

```
float rotationAccum;   // CPU-accumulated rotation phase
float breathAccum;     // CPU-accumulated breathing phase
int rotationAccumLoc;
int breathAccumLoc;
int breathDepthLoc;
int shapeExponentLoc;
```

### Algorithm

All changes insert at STEP 1 of the shader, before `angle = atan(...)`. No changes to STEP 2-4.

1. **Breathing**: Compute `breathScale = 1.0 + breathDepth * sin(breathAccum)`, multiply UV by it before polar conversion
2. **Rotation**: Add `rotationAccum` to the angle after `atan()`
3. **Shape exponent**: Replace `radius` with `pow(radius, shapeExponent)` in the offset formula

CPU side: accumulate both phases each frame in `PitchSpiralEffectSetup()`:
- `e->rotationAccum += cfg->rotationSpeed * deltaTime`
- `e->breathAccum += cfg->breathRate * deltaTime`

Remove the `(void)deltaTime` line since deltaTime is now used.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| rotationSpeed | float | -3.0 to 3.0 | 0.0 | Yes | Rotation Speed (deg/s via SliderAngleDeg) |
| breathRate | float | 0.1 to 5.0 | 1.0 | Yes | Breath Rate (Hz) |
| breathDepth | float | 0.0 to 0.5 | 0.0 | Yes | Breath Depth |
| shapeExponent | float | 0.3 to 3.0 | 1.0 | Yes | Shape Exponent |

### Constants

No new enums or transform types. The effect already exists as `TRANSFORM_PITCH_SPIRAL_BLEND`.

---

## Tasks

### Wave 1: Config & Shader

#### Task 1.1: Add config fields and effect state

**Files**: `src/effects/pitch_spiral.h`
**Creates**: New config fields and effect struct members that all other tasks depend on

**Do**:
- Add the 4 config fields to `PitchSpiralConfig` after `tiltAngle` (before the gradient section), under an `// Animation` comment
- Add `float rotationAccum` and `float breathAccum` to `PitchSpiralEffect` (after `gradientLUT`, before uniform locs)
- Add `int rotationAccumLoc`, `int breathAccumLoc`, `int breathDepthLoc`, `int shapeExponentLoc` to `PitchSpiralEffect`

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Update shader

**Files**: `shaders/pitch_spiral.fs`

**Do**:
- Add 4 new uniforms: `rotationAccum`, `breathAccum`, `breathDepth`, `shapeExponent`
- In `main()` STEP 1, after UV computation and before `angle = atan(...)`:
  1. Apply breathing: `float breathScale = 1.0 + breathDepth * sin(breathAccum); uv *= breathScale;`
  2. After the `atan` line: add `angle += rotationAccum;`
  3. In the offset formula line, replace `radius` with `pow(radius, shapeExponent)`: `float offset = pow(radius, shapeExponent) + (angle / TAU) * spiralSpacing;`
- Apply breath and rotation in both tilt and non-tilt branches (the `uv` variable is set in both paths)

**Verify**: Shader syntax check via build.

---

### Wave 2: C++ Integration (parallel tasks — no file overlap)

#### Task 2.1: Update effect module

**Files**: `src/effects/pitch_spiral.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `PitchSpiralEffectInit()`: cache the 4 new uniform locations via `GetShaderLocation()`
- In `PitchSpiralEffectSetup()`:
  - Remove `(void)deltaTime;` line
  - Accumulate `e->rotationAccum += cfg->rotationSpeed * deltaTime`
  - Accumulate `e->breathAccum += cfg->breathRate * deltaTime`
  - Bind all 4 new uniforms via `SetShaderValue()`
- In `PitchSpiralRegisterParams()`: register all 4 new params with ranges from the parameter table
  - `pitchSpiral.rotationSpeed`: -3.0 to 3.0
  - `pitchSpiral.breathRate`: 0.1 to 5.0
  - `pitchSpiral.breathDepth`: 0.0 to 0.5
  - `pitchSpiral.shapeExponent`: 0.3 to 3.0

**Verify**: Compiles.

#### Task 2.2: Update preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `rotationSpeed`, `breathRate`, `breathDepth`, `shapeExponent` to the existing `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PitchSpiralConfig, ...)` macro call

**Verify**: Compiles.

#### Task 2.3: Update UI panel

**Files**: `src/ui/imgui_effects_generators.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `DrawGeneratorsPitchSpiral()`, add a new "Animation" section (separator + label, same pattern as the existing "Tilt" section) after the Tilt section and before the Color section
- Add sliders:
  - `ModulatableSliderAngleDeg("Rotation Speed##pitchspiral", &ps->rotationSpeed, "pitchSpiral.rotationSpeed", modSources)` — displays deg/s
  - `ModulatableSlider("Breath Rate (Hz)##pitchspiral", &ps->breathRate, "pitchSpiral.breathRate", "%.2f", modSources)`
  - `ModulatableSlider("Breath Depth##pitchspiral", &ps->breathDepth, "pitchSpiral.breathDepth", "%.3f", modSources)`
  - `ModulatableSlider("Shape Exponent##pitchspiral", &ps->shapeExponent, "pitchSpiral.shapeExponent", "%.2f", modSources)`

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Default values (rotationSpeed=0, breathDepth=0, shapeExponent=1.0) produce identical output to current shader
- [ ] Rotation visibly spins the spiral when rotationSpeed != 0
- [ ] Breathing visibly pulses when breathDepth > 0
- [ ] shapeExponent < 1 compresses core, > 1 expands core
- [ ] All 4 params appear in UI under "Animation" section
- [ ] All 4 params respond to LFO modulation
- [ ] Presets save/load the new fields correctly
