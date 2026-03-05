# Cymatics Generator Category

Create a "Cymatics" generator category for wave-visualization effects. Rename the existing Cymatics generator to "Ripple Tank," merge the Interference generator into it, and place both Ripple Tank and the new Chladni generator under this category.

## Classification

- **Category**: General (organizational restructure)

## Context

"Cymatics" is the umbrella term for all methods of visualizing sound as patterns. The existing Cymatics generator is specifically a ripple tank (propagating point-source waves), and the Interference generator is nearly identical (propagating sine waves from point sources). Meanwhile, a planned Chladni generator will visualize resonant plate mode shapes. All three are cymatics techniques that belong under one category.

## Changes

### 1. New Generator Section: Cymatics

- Section index: 16
- Section badge: `"CYM"`
- Section display name: `"Cymatics"`
- Houses: Ripple Tank, Chladni

### 2. Rename Cymatics → Ripple Tank

All references to "Cymatics" in source code (struct names, function names, config fields, enum values, file names) become "RippleTank" / "rippleTank" / "ripple_tank":

| Current | New |
|---------|-----|
| `CymaticsConfig` | `RippleTankConfig` |
| `CymaticsEffect` | `RippleTankEffect` |
| `CymaticsEffectInit()` | `RippleTankEffectInit()` |
| `CymaticsEffectSetup()` | `RippleTankEffectSetup()` |
| `CymaticsEffectRender()` | `RippleTankEffectRender()` |
| `CymaticsEffectResize()` | `RippleTankEffectResize()` |
| `CymaticsEffectUninit()` | `RippleTankEffectUninit()` |
| `CymaticsConfigDefault()` | `RippleTankConfigDefault()` |
| `CymaticsRegisterParams()` | `RippleTankRegisterParams()` |
| `TRANSFORM_CYMATICS` | `TRANSFORM_RIPPLE_TANK` |
| `src/effects/cymatics.h` | `src/effects/ripple_tank.h` |
| `src/effects/cymatics.cpp` | `src/effects/ripple_tank.cpp` |
| `shaders/cymatics.fs` | `shaders/ripple_tank.fs` |
| `EffectConfig::cymatics` | `EffectConfig::rippleTank` |
| Display name `"Cymatics"` | `"Ripple Tank"` |
| Section 13 → Section 16 | |

### 3. Merge Interference into Ripple Tank

Absorb Interference's features into the renamed Ripple Tank generator. Then delete Interference as a standalone effect.

**New fields added to RippleTankConfig from Interference:**

| Field | Type | Range | Default | Source |
|-------|------|-------|---------|--------|
| waveSource | int | 0-1 | 0 | New: 0=audio waveform, 1=parametric sine |
| waveFreq | float | 5-100 | 30.0 | From InterferenceConfig |
| waveSpeed | float | 0-10 | 2.0 | From InterferenceConfig |
| falloffType | int | 0-3 | 3 | From InterferenceConfig (0=none, 1=inverse, 2=inv-square, 3=gaussian) |
| visualMode | int | 0-2 | 0 | From InterferenceConfig (0=raw, 1=absolute, 2=contour) |
| colorMode | int | 0-2 | 0 | From InterferenceConfig (0=intensity, 1=per-source, 2=chromatic) |
| chromaSpread | float | 0-0.1 | 0.03 | From InterferenceConfig |

**Existing Ripple Tank fields that absorb Interference equivalents:**

| Ripple Tank field | Interference equivalent | Notes |
|-------------------|------------------------|-------|
| `waveScale` | `waveFreq` | Same concept, different name. Keep `waveScale` for audio mode, use `waveFreq` for sine mode |
| `falloff` | `falloffStrength` | Rename to `falloffStrength` since falloff type is now selectable |
| `sourceCount` | `sourceCount` | Identical |
| `baseRadius` | `baseRadius` | Identical |
| `lissajous` | `lissajous` | Identical |
| `boundaries` | `boundaries` | Identical |
| `reflectionGain` | `reflectionGain` | Identical |
| `contourCount` | `contourCount` | Identical |
| `visualGain` | `visualGain` | Identical |

**Shader merge:**

The Ripple Tank shader gains:
- `waveSource` uniform to switch between `fetchWaveform(delay)` (audio) and `sin(dist * freq - time + phase)` (sine)
- `falloffType` uniform with the 4-way falloff function from interference.fs
- `visualMode` uniform with the raw/absolute/contour function from interference.fs
- `colorMode` uniform with intensity/per-source/chromatic logic from interference.fs
- When `waveSource == 1` (sine mode): shader uses `time` accumulator and per-source phase offsets instead of waveform buffer

**Conditional UI:**

- When `waveSource == 0` (audio): show `waveScale`, `decayHalfLife`, `diffusionScale` (trail params needed for audio)
- When `waveSource == 1` (sine): show `waveFreq`, `waveSpeed`. Trail params still available but optional.

**Files to delete after merge:**

- `src/effects/interference.h`
- `src/effects/interference.cpp`
- `shaders/interference.fs`
- Remove `TRANSFORM_INTERFERENCE_BLEND` from `TransformEffectType` enum
- Remove `InterferenceConfig interference` from `EffectConfig`
- Remove serialization registration for `InterferenceConfig`

### 4. Chladni Generator

New effect under section 16. Fully specified in `docs/research/chladni-generator.md`.

### 5. Documentation Updates

- `docs/effects.md`: Add "Cymatics" section with Ripple Tank and Chladni entries. Remove Interference entry. Move Cymatics entry to Ripple Tank under new section.
- `docs/conventions.md`: Add section 16 = Cymatics to the category section index list after next `/sync-docs`.
- Preset files referencing `"cymatics"` config key: migrate to `"rippleTank"`. Add backwards-compatible deserialization fallback in `effect_serialization.cpp`.
- Preset files referencing `"interference"` config key: migrate to `"rippleTank"` with `waveSource = 1`. Add backwards-compatible deserialization fallback.

### 6. Section 13 Cleanup

After removing Cymatics and Interference from section 13 (Field), the remaining effects are:
- Constellation (section 13)
- Nebula (section 13)

These stay in section 13 unchanged.

## Ordering

These changes have dependencies:

1. **Cymatics physics revision** (`docs/research/cymatics-physics-revision.md`) — can be done first or folded into the rename+merge
2. **Rename Cymatics → Ripple Tank** — mechanical rename, no behavior change
3. **Merge Interference into Ripple Tank** — adds features, deletes Interference
4. **Create Cymatics category** — new section index, update registrations
5. **Chladni generator** (`docs/research/chladni-generator.md`) — independent, can be done before or after the merge

Recommended order: Physics revision → Rename → Merge → Category creation → Chladni. The physics revision and rename can be combined into one plan. The merge is the most complex step.

## Notes

- The Moire Interference effect (`src/effects/moire_interference.h`) is unrelated — it generates moiré patterns from overlapping grids, not wave interference. It stays in its current category.
- The Chladni Warp transform (`src/effects/chladni_warp.h`) is a UV displacement effect, not a generator. It stays in its current Warp category (section 1). It shares the Chladni equation math but serves a different pipeline purpose.
- Preset migration should handle missing fields gracefully via `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` (already the standard pattern — missing fields get defaults).
