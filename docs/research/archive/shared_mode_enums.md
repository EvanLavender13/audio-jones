# Shared Mode Enums & UI Combos

Shared config enums and UI combo widgets for mode selectors that recur across multiple effects. Standardizes naming, numbering, and UI so future effects reuse the same vocabulary instead of inventing ad-hoc mode lists.

## Classification

- **Category**: General (architecture / shared infrastructure)
- **Scope**: New config headers, UI combo helpers. Existing and future effects adopt incrementally.

## References

- `src/config/dual_lissajous_config.h` — Existing shared config enum/struct pattern
- `src/ui/ui_units.h` — Home for reusable UI widgets (`DrawLissajousControls`, etc.)
- `shaders/muons.fs` lines 82-106 — Canonical waveform mode switch (8 modes)
- `shaders/prism_shatter.fs` lines 38-72 — Canonical 3D fold mode switch (6 modes)
- `docs/research/protean_clouds_v2.md` — TPMS density functions (5 periodic surface modes)

## Algorithm

### Waveform Mode

Displacement waveform for domain warping or FBM turbulence. Every effect that displaces a point with a periodic function uses this enum.

**Header**: `src/config/waveform_mode.h`

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

**UI combo** in `src/ui/ui_units.h`:

```c
inline bool DrawWaveformCombo(const char *label, int *mode) {
    return ImGui::Combo(label, mode,
        "Sine\0Fract Fold\0Abs-Sin\0Triangle\0Squared Sin\0"
        "Square Wave\0Quantized\0Cosine\0");
}
```

**Current consumers**: Muons (`turbulenceMode`), Protean Clouds v2 (`turbulenceMode`), Light Medley v2 (`swirlMode`).

**GLSL pattern**: Each shader has its own switch on the uniform int. The argument and output scaling are per-effect; only the waveform function itself is consistent:

| Value | Waveform | GLSL (applied to argument `a`) |
|-------|----------|-------------------------------|
| 0 | Sine | `sin(a)` |
| 1 | Fract Fold | `fract(a / 6.2832) * 2.0 - 1.0` |
| 2 | Abs-Sin | `abs(sin(a))` |
| 3 | Triangle | `abs(fract(a / 6.2832 + 0.25) * 2.0 - 1.0) * 2.0 - 1.0` |
| 4 | Squared Sin | `sin(a) * abs(sin(a))` |
| 5 | Square Wave | `step(0.5, fract(a / 6.2832)) * 2.0 - 1.0` |
| 6 | Quantized | `floor(sin(a) * 4.0 + 0.5) / 4.0` |
| 7 | Cosine | `cos(a)` |

### Periodic Surface Mode

3D periodic scalar field evaluated at a point. Returns a density/distance value. Every effect that needs a repeating 3D shape function uses this enum. Each consumer normalizes the output to its own needs.

**Header**: `src/config/periodic_surface_mode.h`

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

**UI combo** in `src/ui/ui_units.h`:

```c
inline bool DrawPeriodicSurfaceCombo(const char *label, int *mode) {
    return ImGui::Combo(label, mode,
        "Gyroid\0Schwarz P\0Diamond\0Neovius\0Dot Noise\0");
}
```

**Current consumers**: Protean Clouds v2 (`densityMode`), Voxel March v2 (Gyroid is one surface mode), Light Medley v2 (Gyroid is one lattice mode).

**GLSL pattern**: Each shader has its own switch. The GOLD matrix constant for Dot Noise is declared per-shader.

| Value | Surface | GLSL (evaluated at point `p`) | Raw range |
|-------|---------|-------------------------------|-----------|
| 0 | Gyroid | `dot(cos(p), sin(p.yzx))` | [-3, +3] |
| 1 | Schwarz P | `cos(p.x) + cos(p.y) + cos(p.z)` | [-3, +3] |
| 2 | Diamond | `cos(p.x)*cos(p.y)*cos(p.z) + sin(p.x)*sin(p.y)*cos(p.z) + sin(p.x)*cos(p.y)*sin(p.z) + cos(p.x)*sin(p.y)*sin(p.z)` | [-2, +2] |
| 3 | Neovius | `3.0*(cos(p.x)+cos(p.y)+cos(p.z)) + 4.0*cos(p.x)*cos(p.y)*cos(p.z)` | [-13, +13] |
| 4 | Dot Noise | `dot(cos(GOLD*p), sin(PHI*p*GOLD))` | [-3, +3] |

Consumers apply their own normalization to match their pipeline. The raw ranges are documented so each consumer can scale appropriately.

### Fold Mode

Fractal fold operation applied inside an IFS/fractal loop. Every effect with iterative space-folding uses this enum.

**Header**: `src/config/fold_mode.h`

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

**UI combo** in `src/ui/ui_units.h`:

```c
inline bool DrawFoldCombo(const char *label, int *mode) {
    return ImGui::Combo(label, mode,
        "Box Fold\0Mandelbox\0Sierpinski\0Menger\0Kali\0Burning Ship\0");
}
```

**Current consumers**: Cyber March v2 (`foldMode`), Prism Shatter (`displacementMode` — could adopt), Scrawl (`mode` — partial overlap, 2D).

**GLSL pattern**: Each shader has its own switch. The fold replaces a single line in the fractal loop; surrounding code (axis sort, scale-translate) is per-effect.

| Value | Fold | GLSL (applied to point `p`) |
|-------|------|----------------------------|
| 0 | Box | `p = 0.3 - abs(p)` (offset varies per effect) |
| 1 | Mandelbox | `p = clamp(p,-1.0,1.0)*2.0-p; float r2=dot(p,p); if(r2<0.25) p*=4.0; else if(r2<1.0) p/=r2;` |
| 2 | Sierpinski | `p.xy -= 2.0*min(p.x+p.y,0.0)*vec2(0.5); p.xz -= 2.0*min(p.x+p.z,0.0)*vec2(0.5); p.yz -= 2.0*min(p.y+p.z,0.0)*vec2(0.5);` |
| 3 | Menger | `p=abs(p); if(p.x<p.y) p.xy=p.yx; if(p.x<p.z) p.xz=p.zx; if(p.y<p.z) p.yz=p.zy;` |
| 4 | Kali | `p = abs(p) / max(dot(p,p), 1e-6) - 0.5` (offset varies per effect) |
| 5 | Burning Ship | `vec3 a=abs(p); p=vec3(a.y*a.z, a.x*a.z, a.x*a.y);` |

Box and Kali fold offsets are noted as per-effect because the offset interacts with each effect's scale-translate step differently. The fold shape itself is canonical.

## Serialization

All three enums serialize as plain `int` fields via the existing `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro. No special `to_json`/`from_json` needed. Effect config structs store `int waveformMode = 0;` (or `foldMode`, `periodicSurfaceMode`) and list the field in their `CONFIG_FIELDS` macro.

## Adoption

- **New effects**: Include the relevant header, use the enum default, call the combo widget in colocated UI.
- **Existing effects (Muons)**: Muons already has `turbulenceMode` with the same 8 waveforms. Adopt by including `waveform_mode.h`, verifying enum values match, and replacing the hardcoded combo string with `DrawWaveformCombo()`. The shader switch stays as-is since the values already match.
- **Existing effects (Prism Shatter, Scrawl)**: Partial overlap with `FoldMode`. Can adopt incrementally; modes that don't map (Triple Product Gradient, Spiral IFS, Power Fold) stay as effect-specific extensions beyond `FOLD_COUNT`.

## Notes

- These are enums and combo widgets, not full config structs. There are no embedded fields, no update functions, no param registration helpers. Effects store a plain int.
- The GLSL implementations are duplicated per shader. There is no shared GLSL include infrastructure. The enum values and the documented GLSL patterns ensure consistency.
- When adding a new mode to any enum, add it before `_COUNT`, update the combo string, and update all consuming shaders. The combo strings and shader switches must stay in sync manually.
- Effects may support a subset of modes. An effect that only uses 3 of the 5 periodic surfaces can clamp the combo range. But the enum values must always match so presets are portable.
- Prism Shatter and Scrawl have effect-specific fold operations (Triple Product, Spiral IFS, Power Fold) that don't belong in the shared enum. Those stay as local extensions. The shared enum covers the universal set that multiple effects share.
