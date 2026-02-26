# Muons Turbulence Modes

Add a turbulence mode selector to the Muons generator, mirroring the existing distance function mode selector. Each mode replaces the FBM waveform function inside the turbulence loop, producing different spatial folding characters — from smooth sine swirls to blocky digital geometry.

**Research**: `docs/research/muons-turbulence-modes.md`

## Design

### Types

Add to `MuonsConfig`:
```c
int turbulenceMode = 0; // Turbulence waveform (0-6)
```

Add to `MuonsEffect`:
```c
int turbulenceModeLoc;
```

Add `turbulenceMode` to `MUONS_CONFIG_FIELDS` macro (after `mode`).

### Algorithm

Replace the single turbulence line inside the FBM loop with a `switch (turbulenceMode)` block. The loop structure is identical across all modes — only the waveform expression changes:

```glsl
d = 1.0;
for (int j = 1; j < turbulenceOctaves; j++) {
    d += 1.0;
    switch (turbulenceMode) {
    case 1: // Fract Fold — sharp sawtooth, crystalline/faceted filaments
        a += (fract(a * d + time * PHI) * 2.0 - 1.0).yzx / d * turbulenceStrength;
        break;
    case 2: // Abs-Sin Fold — sharp angular turns, always-positive displacement
        a += abs(sin(a * d + time * PHI)).yzx / d * turbulenceStrength;
        break;
    case 3: // Triangle Wave — smooth zigzag between sin and fract character
        a += (abs(fract(a * d + time * PHI) * 2.0 - 1.0) * 2.0 - 1.0).yzx / d * turbulenceStrength;
        break;
    case 4: // Squared Sin — soft peaks with flat valleys, organic feel
        a += (sin(a * d + time * PHI) * abs(sin(a * d + time * PHI))).yzx / d * turbulenceStrength;
        break;
    case 5: // Square Wave — hard binary on/off, blocky digital geometry
        a += (step(0.5, fract(a * d + time * PHI)) * 2.0 - 1.0).yzx / d * turbulenceStrength;
        break;
    case 6: // Quantized — grid-locked staircase structures
        a += (floor(a * d + time * PHI + 0.5)).yzx / d * turbulenceStrength;
        break;
    default: // 0: Sine (original) — smooth swirling folds
        a += sin(a * d + time * PHI).yzx / d * turbulenceStrength;
        break;
    }
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| turbulenceMode | int | 0-6 | 0 | No | `"Turbulence Mode##muons"` |

Mode labels: `"Sine"`, `"Fract Fold"`, `"Abs-Sin"`, `"Triangle"`, `"Squared Sin"`, `"Square Wave"`, `"Quantized"`

---

## Tasks

### Wave 1: All Changes

All three files have no overlap — but this is small enough that a single wave suffices.

#### Task 1.1: Config & Runtime State

**Files**: `src/effects/muons.h`

**Do**:
- Add `int turbulenceMode = 0; // Turbulence waveform (0-6)` to `MuonsConfig`, after the existing `mode` field
- Add `turbulenceMode` to `MUONS_CONFIG_FIELDS` macro, after `mode`
- Add `int turbulenceModeLoc;` to `MuonsEffect`, after `modeLoc`

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Uniform Binding & UI

**Files**: `src/effects/muons.cpp`

**Do**:
- In `MuonsEffectInit()`: cache `e->turbulenceModeLoc = GetShaderLocation(e->shader, "turbulenceMode");` after the `modeLoc` line (line 60)
- In `MuonsEffectSetup()`: bind `SetShaderValue(e->shader, e->turbulenceModeLoc, &cfg->turbulenceMode, SHADER_UNIFORM_INT);` after the existing `mode` bind (line 89)
- In `DrawMuonsParams()`: add a combo box directly after the existing `Mode##muons` combo (line 217). Follow the same pattern:
  ```cpp
  const char *turbulenceModeLabels[] = {"Sine",        "Fract Fold",
                                        "Abs-Sin",     "Triangle",
                                        "Squared Sin", "Square Wave",
                                        "Quantized"};
  ImGui::Combo("Turbulence Mode##muons", &m->turbulenceMode,
               turbulenceModeLabels, IM_ARRAYSIZE(turbulenceModeLabels));
  ```

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.3: Shader Switch Block

**Files**: `shaders/muons.fs`

**Do**:
- Add `uniform int turbulenceMode;` after the existing `uniform int mode;` (line 39)
- Replace the single turbulence line (line 69: `a += sin(a * d + time * PHI).yzx / d * turbulenceStrength;`) with the `switch (turbulenceMode)` block from the Algorithm section above

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Default mode 0 (Sine) looks identical to current behavior
- [ ] Each mode (1-6) produces visually distinct turbulence patterns
- [ ] Mode persists through preset save/load
