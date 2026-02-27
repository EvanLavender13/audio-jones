# Prism Shatter — Displacement Modes

Add a switchable displacement operation to the Prism Shatter raymarcher's `scan()` loop. Six modes replace the `newPos` computation with different IFS folds and field gradients, giving the crystal geometry distinct structural characters. Single int mode selector, no new float parameters.

**Research**: `docs/research/prism-shatter-displacement-modes.md`

## Design

### Types

**PrismShatterConfig** — add field:
```
int displacementMode = 0; // Displacement operation (0-5)
```

**PrismShatterEffect** — add field:
```
int displacementModeLoc;
```

**PRISM_SHATTER_CONFIG_FIELDS** — append `displacementMode` to the macro.

### Algorithm

The shader's `scan()` function computes `newPos` from `posMod` (the sine-evaluated position). The new `uniform int displacementMode` selects which operation via a `switch` statement:

```glsl
uniform int displacementMode;

// Inside scan(), replace the fixed newPos computation with:
vec3 newPos;
switch (displacementMode) {
case 1: // Absolute Fold
    newPos = abs(posMod) - 1.0;
    break;
case 2: // Mandelbox Fold
    newPos = clamp(posMod, -1.0, 1.0) * 2.0 - posMod;
    float r2 = dot(newPos, newPos);
    if (r2 < 0.25) newPos *= 4.0;
    else if (r2 < 1.0) newPos /= r2;
    break;
case 3: // Sierpinski Fold
    newPos = posMod;
    newPos.xy -= 2.0 * min(newPos.x + newPos.y, 0.0) * vec2(0.5);
    newPos.xz -= 2.0 * min(newPos.x + newPos.z, 0.0) * vec2(0.5);
    newPos.yz -= 2.0 * min(newPos.y + newPos.z, 0.0) * vec2(0.5);
    break;
case 4: // Menger Fold
    newPos = abs(posMod);
    if (newPos.x < newPos.y) newPos.xy = newPos.yx;
    if (newPos.x < newPos.z) newPos.xz = newPos.zx;
    if (newPos.y < newPos.z) newPos.yz = newPos.zy;
    break;
case 5: // Burning Ship Fold
    vec3 a = abs(posMod);
    newPos.x = a.y * a.z;
    newPos.y = a.x * a.z;
    newPos.z = a.x * a.y;
    break;
default: // 0: Triple Product Gradient (original)
    newPos.x = posMod.y * posMod.z;
    newPos.y = posMod.x * posMod.z;
    newPos.z = posMod.x * posMod.y;
    break;
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| displacementMode | int | 0-5 | 0 | No | "Displacement##prismShatter" |

### Constants

Mode names for `ImGui::Combo`:
```
"Triple Product\0Absolute Fold\0Mandelbox\0Sierpinski\0Menger\0Burning Ship\0"
```

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Add displacementMode to config and effect structs

**Files**: `src/effects/prism_shatter.h`
**Creates**: `displacementMode` config field and `displacementModeLoc` effect field that .cpp and .fs depend on

**Do**:
- Add `int displacementMode = 0; // Displacement operation (0-5)` to `PrismShatterConfig`, in the Geometry section after `iterations`
- Add `int displacementModeLoc;` to `PrismShatterEffect`, after `iterationsLoc`
- Append `displacementMode` to `PRISM_SHATTER_CONFIG_FIELDS` macro

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Wire uniform and add UI combo

**Files**: `src/effects/prism_shatter.cpp`
**Depends on**: Wave 1 complete

**Do** (follow muons.cpp mode pattern):
- In `PrismShatterEffectInit`: add `e->displacementModeLoc = GetShaderLocation(e->shader, "displacementMode");`
- In `PrismShatterEffectSetup`: add `SetShaderValue(e->shader, e->displacementModeLoc, &cfg->displacementMode, SHADER_UNIFORM_INT);`
- In `DrawPrismShatterParams`, in the Geometry section after Iterations slider: add `ImGui::Combo("Displacement##prismShatter", &cfg->displacementMode, "Triple Product\0Absolute Fold\0Mandelbox\0Sierpinski\0Menger\0Burning Ship\0");`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Add displacement mode switch to shader

**Files**: `shaders/prism_shatter.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add `uniform int displacementMode;` after the existing uniform declarations
- In `scan()`, replace the three fixed `newPos` assignment lines (lines 37-39) with the `switch` statement from the Algorithm section above

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Mode 0 (Triple Product) looks identical to current behavior
- [ ] All 6 modes produce distinct visuals
- [ ] Mode persists through preset save/load
- [ ] UI combo appears in Geometry section
