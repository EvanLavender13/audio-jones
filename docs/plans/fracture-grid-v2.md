# Fracture Grid V2

Add per-tile flip/mirror and skew transforms, plus three wave propagation modes (directional sweep, radial ripple, Manhattan cascade) to the existing Fracture Grid effect. All new params default to zero so existing presets are unaffected.

**Research**: `docs/research/fracture_grid_v2.md`

## Design

### Types

**FractureGridConfig** (add fields after `spatialBias`):

```
float flipChance = 0.0f;       // 0.0-1.0 - probability of per-tile h/v flip
float skewScale = 0.0f;        // 0.0-1.0 - max shear intensity per tile
int propagationMode = 0;        // 0=none, 1=directional, 2=radial, 3=cascade
float propagationSpeed = 5.0f;  // 0.0-20.0 - phase offset scale from distance
float propagationAngle = 0.0f;  // -PI..PI - sweep direction (radians, mode 1 only)
```

**FractureGridEffect** (add uniform locations after `spatialBiasLoc`):

```
int flipChanceLoc;
int skewScaleLoc;
int propagationModeLoc;
int propagationSpeedLoc;
int propagationAngleLoc;
```

**FRACTURE_GRID_CONFIG_FIELDS** macro: append `flipChance, skewScale, propagationMode, propagationSpeed, propagationAngle`

### Algorithm

New shader helper:

```glsl
mat2 shear2(float sx) {
    return mat2(1.0, 0.0, sx, 1.0);
}
```

New uniforms:

```glsl
uniform float flipChance;
uniform float skewScale;
uniform int propagationMode;
uniform float propagationSpeed;
uniform float propagationAngle;
```

Updated `computeTileWarp` (full replacement):

```glsl
vec2 computeTileWarp(vec2 tileId, vec2 tileCellUV, vec2 tileCellCenter, float sub) {
    vec3 h = mix(hash3(tileId), computeSpatial(tileCellCenter, waveTime), spatialBias);
    vec3 h2 = hash3(tileId + vec2(127.1, 311.7));

    // Propagation phase offset
    float propPhase = 0.0;
    if (propagationMode == 1) {
        vec2 dir = vec2(cos(propagationAngle), sin(propagationAngle));
        propPhase = dot(tileCellCenter, dir) * propagationSpeed;
    } else if (propagationMode == 2) {
        propPhase = length(tileCellCenter) * propagationSpeed;
    } else if (propagationMode == 3) {
        propPhase = (abs(tileCellCenter.x) + abs(tileCellCenter.y)) * propagationSpeed;
    }

    float phase = h.x * 6.283 + propPhase;

    // Per-tile flip
    vec2 flip = vec2(1.0);
    if (h2.x < flipChance) { flip.x = -1.0; }
    if (h2.y < flipChance) { flip.y = -1.0; }
    vec2 flippedUV = tileCellUV * flip;

    // Per-tile transforms
    vec2 offset = (h.xy - 0.5) * stagger * offsetScale
                * shapedWave(waveTime + phase, waveShape);
    float angle = (h.z - 0.5) * stagger * rotationScale
                * shapedWave(waveTime * 1.3 + phase, waveShape);
    float shearAmt = (h2.z - 0.5) * stagger * skewScale
                   * shapedWave(waveTime * 0.9 + phase, waveShape);
    float zoom = max(0.2, 1.0 + (h.y - 0.5) * stagger * zoomScale
                * shapedWave(waveTime * 0.7 + phase, waveShape));

    return tileCellCenter + rot2(angle) * shear2(shearAmt) * (flippedUV / (sub * zoom)) + offset;
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| flipChance | float | 0.0-1.0 | 0.0 | Yes | `"Flip Chance##fracgrid"` |
| skewScale | float | 0.0-1.0 | 0.0 | Yes | `"Skew##fracgrid"` |
| propagationMode | int | 0-3 | 0 | No | `"Propagation##fracgrid"` (Combo) |
| propagationSpeed | float | 0.0-20.0 | 5.0 | Yes | `"Propagation Speed##fracgrid"` |
| propagationAngle | float | -PI..PI | 0.0 | Yes | `"Propagation Angle##fracgrid"` (AngleDeg) |

### UI Layout

**Geometry section** (insert after Zoom Scale, before Tessellation):

```
ModulatableSlider("Flip Chance##fracgrid", ...)    // "%.2f"
ModulatableSlider("Skew##fracgrid", ...)           // "%.2f"
```

**Animation section** (append after Wave Shape):

```
ImGui::Combo("Propagation##fracgrid", ..., {"None", "Directional", "Radial", "Cascade"}, 4)
ModulatableSlider("Prop. Speed##fracgrid", ...)    // "%.1f"
if (propagationMode == 1) {
    ModulatableSliderAngleDeg("Prop. Angle##fracgrid", ..., ROTATION_OFFSET_MAX)
}
```

---

## Tasks

### Wave 1: Header

#### Task 1.1: Update FractureGridConfig and FractureGridEffect structs

**Files**: `src/effects/fracture_grid.h`
**Creates**: Config fields and uniform locations that .cpp and .fs depend on

**Do**:
- Add 5 new fields to `FractureGridConfig` after `spatialBias` (see Design > Types)
- Add 5 new uniform location fields to `FractureGridEffect` after `spatialBiasLoc` (see Design > Types)
- Append new fields to `FRACTURE_GRID_CONFIG_FIELDS` macro
- Follow existing field comment style: `// range description`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader and C++ (parallel)

#### Task 2.1: Implement shader enhancements

**Files**: `shaders/fracture_grid.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add 5 new uniforms after `spatialBias` (see Design > Algorithm)
- Add `shear2()` helper after `rot2()`
- Replace `computeTileWarp` with the version from Design > Algorithm (full replacement, not a patch)
- Keep all other functions (`hash3`, `rot2`, `shapedWave`, `computeSpatial`, `tessRect`, `tessHex`, `tessTri`) and `main()` unchanged

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Wire up C++ uniforms, registration, and UI

**Files**: `src/effects/fracture_grid.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `FractureGridEffectInit`: add 5 `GetShaderLocation` calls for new uniforms after `spatialBiasLoc`
- In `FractureGridEffectSetup`: add 5 `SetShaderValue` calls for new uniforms after `spatialBias`. `propagationMode` is `SHADER_UNIFORM_INT`, rest are `SHADER_UNIFORM_FLOAT`
- In `FractureGridRegisterParams`: register `flipChance`, `skewScale`, `propagationSpeed`, `propagationAngle` (not `propagationMode` - it's an int combo, not modulatable). Use `ROTATION_OFFSET_MAX` bounds for `propagationAngle`
- In `DrawFractureGridParams`:
  - Geometry section: add Flip Chance and Skew sliders after Zoom Scale, before Tessellation combo (see Design > UI Layout)
  - Animation section: add Propagation combo, Propagation Speed slider, and conditional Propagation Angle slider after Wave Shape (see Design > UI Layout)
  - Propagation mode names array: `{"None", "Directional", "Radial", "Cascade"}`

**Verify**: `cmake.exe --build build` compiles. Run the app and confirm new sliders appear in the Fracture Grid effect panel.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Existing Fracture Grid presets load without errors (new fields get defaults)
- [ ] Flip Chance at 0 = no change, at 1 = all tiles flipped
- [ ] Skew at 0 = no change, increasing skew visibly shears tiles
- [ ] Propagation None = existing behavior
- [ ] Propagation Directional sweeps tiles along the angle
- [ ] Propagation Radial ripples outward from center
- [ ] Propagation Cascade creates diamond-shaped wavefronts
- [ ] Propagation Angle slider only visible when mode is Directional

---

## Implementation Notes

- **skewScale range 0-2, multiplied by `sub` in shader**: The shear matrix operates on `flippedUV / (sub * zoom)`, so at high subdivision the vector is tiny and shear is invisible. Multiplying shearAmt by `sub` makes the visual intensity subdivision-independent.
- **Propagation mode 0 labeled "Noise"** not "None": Mode 0 uses hash-derived phase per tile - it IS a behavior (random/noise), not the absence of one.
- **propagationSpeed is CPU-accumulated**: Added `propagationPhase` accumulator to `FractureGridEffect`, accumulated as `propagationPhase += propagationSpeed * deltaTime`. Shader receives both `propagationSpeed` (spatial frequency) and `propagationPhase` (temporal animation). Formula: `distance * propagationSpeed + propagationPhase`.
