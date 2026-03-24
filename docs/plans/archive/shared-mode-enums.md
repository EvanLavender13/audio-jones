# Shared Mode Enums & UI Combos

Three shared enums (`WaveformMode`, `PeriodicSurfaceMode`, `FoldMode`) with corresponding UI combo widgets. Standardizes mode selectors that recur across multiple effects so future effects reuse the same vocabulary instead of inventing ad-hoc mode lists. Muons adopts `WaveformMode` as the first consumer.

**Research**: `docs/research/shared_mode_enums.md`

## Design

### Types

**`WaveformMode`** (`src/config/waveform_mode.h`):

```c
typedef enum {
    WAVEFORM_SINE = 0,       // Smooth swirling folds
    WAVEFORM_FRACT_FOLD,     // Crystalline/faceted (sawtooth)
    WAVEFORM_ABS_SIN,        // Sharp valleys, always-positive
    WAVEFORM_TRIANGLE,       // Zigzag between sin and fract character
    WAVEFORM_SQUARED_SIN,    // Soft peaks, flat valleys
    WAVEFORM_SQUARE_WAVE,    // Blocky digital geometry
    WAVEFORM_QUANTIZED,      // Staircase structures
    WAVEFORM_COSINE,         // Phase-shifted sine, different fold geometry
    WAVEFORM_COUNT
} WaveformMode;
```

**`PeriodicSurfaceMode`** (`src/config/periodic_surface_mode.h`):

```c
typedef enum {
    PERIODIC_SURFACE_GYROID = 0,    // Smooth interconnected channels (TPMS)
    PERIODIC_SURFACE_SCHWARZ_P,     // Cubic lattice with spherical voids (TPMS)
    PERIODIC_SURFACE_DIAMOND,       // Tetrahedral lattice (TPMS)
    PERIODIC_SURFACE_NEOVIUS,       // Thick-walled complex cubic lattice (TPMS)
    PERIODIC_SURFACE_DOT_NOISE,     // Aperiodic blobs (gold-ratio gyroid)
    PERIODIC_SURFACE_COUNT
} PeriodicSurfaceMode;
```

**`FoldMode`** (`src/config/fold_mode.h`):

```c
typedef enum {
    FOLD_BOX = 0,          // Box reflection (abs + offset)
    FOLD_MANDELBOX,        // Box clamp + sphere inversion
    FOLD_SIERPINSKI,       // Tetrahedral plane reflections
    FOLD_MENGER,           // Abs + full 3-axis sort
    FOLD_KALI,             // Sphere inversion (abs/dot)
    FOLD_BURNING_SHIP,     // Abs cross-products
    FOLD_COUNT
} FoldMode;
```

### UI Combo Widgets

All three go in `src/ui/ui_units.h`. Each is an `inline bool` function that wraps `ImGui::Combo()`.

**`DrawWaveformCombo`**:

```c
inline bool DrawWaveformCombo(const char *label, int *mode) {
    return ImGui::Combo(label, mode,
        "Sine\0Fract Fold\0Abs-Sin\0Triangle\0Squared Sin\0"
        "Square Wave\0Quantized\0Cosine\0");
}
```

**`DrawPeriodicSurfaceCombo`**:

```c
inline bool DrawPeriodicSurfaceCombo(const char *label, int *mode) {
    return ImGui::Combo(label, mode,
        "Gyroid\0Schwarz P\0Diamond\0Neovius\0Dot Noise\0");
}
```

**`DrawFoldCombo`**:

```c
inline bool DrawFoldCombo(const char *label, int *mode) {
    return ImGui::Combo(label, mode,
        "Box Fold\0Mandelbox\0Sierpinski\0Menger\0Kali\0Burning Ship\0");
}
```

### Muons Adoption

Muons already has `int turbulenceMode = 0` with the same 8 waveforms in the same order. Adoption changes:

- `muons.h`: Add `#include "config/waveform_mode.h"`. Fix range comment from `(0-6)` to `(0-7)`.
- `muons.cpp` UI section: Replace the hardcoded `turbulenceModeLabels[]` array + `ImGui::Combo()` call with `DrawWaveformCombo("Turbulence Mode##muons", &m->turbulenceMode)`.

No shader changes needed - the integer values already match the enum.

---

## Tasks

### Wave 1: Enum Headers

#### Task 1.1: WaveformMode header

**Files**: `src/config/waveform_mode.h` (create)
**Creates**: `WaveformMode` enum used by ui_units.h combo and Muons header

**Do**: Create header with include guard `WAVEFORM_MODE_H`. One-line top comment: `// Displacement waveform modes for domain warping and FBM turbulence`. Define the `WaveformMode` typedef enum exactly as shown in Design > Types. No includes needed.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: PeriodicSurfaceMode header

**Files**: `src/config/periodic_surface_mode.h` (create)
**Creates**: `PeriodicSurfaceMode` enum for future effect adoption

**Do**: Create header with include guard `PERIODIC_SURFACE_MODE_H`. One-line top comment: `// 3D periodic scalar field modes (TPMS and variants)`. Define the `PeriodicSurfaceMode` typedef enum exactly as shown in Design > Types. No includes needed.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.3: FoldMode header

**Files**: `src/config/fold_mode.h` (create)
**Creates**: `FoldMode` enum for future effect adoption

**Do**: Create header with include guard `FOLD_MODE_H`. One-line top comment: `// Fractal fold operations for iterative space-folding effects`. Define the `FoldMode` typedef enum exactly as shown in Design > Types. No includes needed.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: UI Combos & Adoption

#### Task 2.1: UI combo widgets

**Files**: `src/ui/ui_units.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Add the three combo widget functions exactly as shown in Design > UI Combo Widgets. Place them after the existing `ModulatableSliderInt()` function and before `DrawLissajousControls()`. No new `#include` needed - `imgui.h` is already included.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Muons adoption

**Files**: `src/effects/muons.h` (modify), `src/effects/muons.cpp` (modify)
**Depends on**: Wave 1 complete, Task 2.1 complete

**Do**:
- In `muons.h`: Add `#include "config/waveform_mode.h"`. Fix the `turbulenceMode` range comment from `(0-6)` to `(0-7)`.
- In `muons.cpp` UI section: Replace the `turbulenceModeLabels[]` array and `ImGui::Combo("Turbulence Mode##muons", ...)` call with `DrawWaveformCombo("Turbulence Mode##muons", &m->turbulenceMode)`. Remove the two lines defining/using `turbulenceModeLabels`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All three enum headers exist in `src/config/`
- [ ] `ui_units.h` has all three `Draw*Combo` functions
- [ ] Muons uses `DrawWaveformCombo` instead of hardcoded labels
- [ ] Muons turbulenceMode range comment says `(0-7)`
