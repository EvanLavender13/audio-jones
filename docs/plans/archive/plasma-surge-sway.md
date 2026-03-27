# Plasma Surge & Sway

Adds two behaviors from the "Thunder Dance" shader to the existing Plasma generator: a power surge oscillator that makes bolt displacement pulse irregularly, and coherent directional sway that replaces chaotic random wiggle with wind-like sweeping motion.

**Research**: `docs/research/plasma_surge_sway.md`

## Design

### Types

**PlasmaConfig additions** (after `flickerAmount`):

```
float surgeAmount = 0.0f;  // Power surge intensity (0.0-1.0)
float sway = 0.0f;         // Coherent sway vs random displacement (0.0-1.0)
float swaySpeed = 1.0f;    // Sway oscillation rate (0.0-5.0)
float swayRotationSpeed = 0.1f; // Sway direction rotation rate (0.0-2.0)
```

**PlasmaEffect additions** (after `flickerTime`):

```
float swayPhase;
float swayRotationSpeedPhase;
```

**PlasmaEffect uniform locations** (after `flickerTimeLoc`):

```
int surgeAmountLoc;
int swayLoc;
int swayPhaseLoc;
int swayRotationSpeedPhaseLoc;
```

Note: no separate `surgePhase` accumulator is needed. The research specifies `surgePhase += animSpeed * deltaTime`, which is identical to the existing `animPhase` accumulator. The shader uses `animPhase` directly in the surge formula.

**CONFIG_FIELDS macro**: append `surgeAmount, sway, swaySpeed, swayRotationSpeed` after `flickerAmount`.

### Algorithm

In the shader bolt loop, replace the current displacement line:

```glsl
displaced += (vec2(dx, dy) - 0.5) * displacement;
```

With the surge + sway computation:

```glsl
// Power surge: nested sinusoid creates irregular pulsing
float surge = sin(10.0 * animPhase + sin(20.0 * animPhase));

// Perpendicular sway vectors from rotating angle
float swayAngle = swayRotationSpeedPhase;
vec2 swayDir = vec2(cos(swayAngle), sin(swayAngle));
vec2 v1 = swayDir.yx * vec2(-1.0, 1.0) * sin(swayPhase);
vec2 v2 = swayDir.yx * vec2(1.0, -1.0) * sin(swayPhase);

// Blend random displacement with coherent sway
vec2 randomDisp = vec2(dx, dy) - 0.5;
vec2 finalDisp = randomDisp;
if (sway > 0.0) {
    float n1 = fbm(noiseCoord + v1, octaves);
    float n2 = fbm(noiseCoord + v2, octaves);
    float swayN = (n1 + n2) * 0.5 - 0.5;
    finalDisp = mix(randomDisp, vec2(swayN), sway);
}

// Apply displacement with surge multiplier
displaced += finalDisp * displacement * mix(1.0, surge, surgeAmount);
```

The `if (sway > 0.0)` guard short-circuits the two extra FBM calls when sway is off. At 8 octaves and 8 bolts, that saves 128 noise samples per fragment.

Surge multiplier `mix(1.0, surge, surgeAmount)` ranges from `1 - 2*surgeAmount` to `1.0`. At surgeAmount >= 0.5, the multiplier passes through zero, creating moments where bolts snap straight before rebounding.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| surgeAmount | float | 0.0-1.0 | 0.0 | Yes | Surge |
| sway | float | 0.0-1.0 | 0.0 | Yes | Sway |
| swaySpeed | float | 0.0-5.0 | 1.0 | Yes | Sway Speed |
| swayRotationSpeed | float | 0.0-2.0 | 0.1 | Yes | Sway Rotation |

All defaults are 0.0 for `surgeAmount` and `sway`, so existing presets load unchanged.

### Shader Uniforms

New uniforms in `plasma.fs`:

```glsl
uniform float surgeAmount;
uniform float sway;
uniform float swayPhase;
uniform float swayRotationSpeedPhase;
```

No new `surgePhase` uniform - uses existing `animPhase`.

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Add surge/sway fields to PlasmaConfig and PlasmaEffect

**Files**: `src/effects/plasma.h`
**Creates**: New config fields and effect state that Wave 2 tasks depend on

**Do**:
- Add 4 config fields (`surgeAmount`, `sway`, `swaySpeed`, `swayRotationSpeed`) after `flickerAmount`, with defaults and range comments per Design section
- Add 2 phase accumulators (`swayPhase`, `swayRotationSpeedPhase`) after `flickerTime` in `PlasmaEffect`
- Add 4 uniform location fields (`surgeAmountLoc`, `swayLoc`, `swayPhaseLoc`, `swayRotationSpeedPhaseLoc`) after `flickerTimeLoc`
- Append `surgeAmount, sway, swaySpeed, swayRotationSpeed` to `PLASMA_CONFIG_FIELDS` macro

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Integrate surge/sway into PlasmaEffect lifecycle and UI

**Files**: `src/effects/plasma.cpp`
**Depends on**: Wave 1 complete

**Do**:
- **Init**: Cache 4 new uniform locations (`surgeAmount`, `sway`, `swayPhase`, `swayRotationSpeedPhase`). Init `swayPhase` and `swayRotationSpeedPhase` to 0.
- **Setup**: Accumulate phases: `swayPhase += swaySpeed * deltaTime`, `swayRotationSpeedPhase += swayRotationSpeed * deltaTime`. Bind 4 new uniforms via `SetShaderValue`.
- **RegisterParams**: Register 4 new params: `plasma.surgeAmount` (0-1), `plasma.sway` (0-1), `plasma.swaySpeed` (0-5), `plasma.swayRotationSpeed` (0-2).
- **UI**: Add a new section after the Appearance sliders with 4 `ModulatableSlider` calls: Surge, Sway, Sway Speed, Sway Rotation. Use `"%.2f"` format. Add spacing + separator before the new section.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Add surge/sway computation to plasma shader

**Files**: `shaders/plasma.fs`
**Depends on**: Wave 1 complete (for uniform name agreement, but no #include dependency)

**Do**:
- Add 4 new uniforms after `flickerAmount`: `surgeAmount`, `sway`, `swayPhase`, `swayRotationSpeedPhase`
- In `main()`, replace the line `displaced += (vec2(dx, dy) - 0.5) * displacement;` with the Algorithm section code (surge computation, perpendicular sway vectors, FBM sway sampling with short-circuit, blended displacement with surge multiplier)
- Keep existing `dx`/`dy` FBM computation unchanged - it provides the random displacement base

**Verify**: `cmake.exe --build build` compiles and launches. Plasma with default params (surgeAmount=0, sway=0) looks identical to before. Increasing surgeAmount creates irregular pulsing. Increasing sway creates directional sweeping.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Default params (surgeAmount=0, sway=0) produce identical output to current Plasma
- [ ] surgeAmount=1.0 creates visible irregular pulsing of bolt displacement
- [ ] sway=1.0 creates coherent directional sweep instead of random noise wiggle
- [ ] All 4 new sliders appear in Plasma UI panel
- [ ] Existing presets load without changes
