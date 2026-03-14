---
name: ui-guide
description: Use when editing UI code in any panel or effect Draw*Params function. Enforces the Signal Stack design language — section ordering, naming vocabulary, divider patterns, widget selection, and common mistakes. Triggers on UI edits to src/ui/, src/effects/ UI sections, drawable_type_controls.cpp, or when reviewing effect UI for consistency.
---

# UI Design Language: Signal Stack

Follow these rules when writing or modifying UI code in AudioJones. The Signal Stack grammar defines how panels, effects, drawables, LFOs, and analysis organize their controls.

Reference: `docs/research/ui_design_language.md` for the full design rationale.

---

## Collapsibility Rules

**Collapsible (`DrawSectionBegin/End`)**: ONLY for individual effect entries in the Effects window. 112 effects compete for scroll space — collapsing is essential.

**Never collapsible**: Everything else. LFOs, Analysis, Drawables, and subsections within an expanded effect. If the user navigated there, they want to see it.

| Context | Divider | Collapsible? |
|---------|---------|-------------|
| Effects window: per-effect entry | `DrawSectionBegin/End()` | Yes |
| Inside an expanded effect | `SeparatorText()` | No |
| LFO panel: per-LFO module | `DrawModuleStripBegin/End()` | No |
| Analysis panel: metric groups | `SeparatorText()` | No |
| Drawables panel: selected item sections | `SeparatorText()` | No |
| Effects window: pipeline groups | `DrawGroupHeader()` | No |
| Effects window: transform categories | `DrawCategoryHeader()` | No |

---

## Panel Hierarchy

```
Level 0: Window Title (docking tab)
Level 1: DrawGroupHeader()       ← Effects window pipeline groups ONLY
Level 2: DrawCategoryHeader()    ← Effects window transform categories ONLY
Level 3: DrawSectionBegin/End()  ← Effects window per-effect ONLY
Level 4: SeparatorText()         ← Universal section divider (everywhere)
```

Most panels go straight to Level 4. Only the Effects window uses all levels.

### Forbidden Patterns

- **Never use `TextColored()` as a section header** — no affordance, no structure. Use `SeparatorText()`.
- **Never use `TreeNodeAccented()`** — retired. Convert to `SeparatorText()`.
- **Never use `ImGui::Separator()` + `ImGui::Spacing()` as a manual section break** — use `SeparatorText("Label")`.
- **Never add a `DrawGroupHeader()` that restates the window title** — "LFOS" in the LFOs window is redundant.

---

## Effect UI: Section Ordering

Every effect's `Draw<Name>Params()` organizes controls with `SeparatorText` subsections in this canonical order. Skip sections that don't apply. When present, they appear in this order:

```
1. Audio          ← FFT parameters (always first when present)
2. Geometry       ← Spatial structure
3. [Domain]       ← Effect-specific sections (0-4, names unique to the effect)
4. Animation      ← Time-based motion
5. Glow           ← Brightness/emission
6. Color          ← Color mapping
```

### Rules

- **Audio always comes first.** It defines the input signal; everything else reacts to it.
- **Geometry comes second.** The spatial foundation the effect builds on.
- **Domain-specific sections go in the middle.** Names unique to the effect (e.g., "Wave", "Raymarching", "Flocking", "Curvature"). Use domain names when no standard name fits.
- **Animation after domain.** It modulates what's defined above.
- **Glow and Color come last.** Output-stage concerns.

### Section Name Vocabulary

Use these standard names. Do NOT invent alternatives.

| Standard Name | Replaces | Content |
|---------------|----------|---------|
| `"Audio"` | — | baseFreq, maxFreq, gain, contrast, baseBright |
| `"Geometry"` | "Shape", "Grid", "Layout", "Ring Layout", "Structure" | scale, size, layers, radius, count, offset |
| `"Animation"` | "Motion", "Timing", "Dynamics", "Movement" | speed, phase, frequency, spin, drift |
| `"Glow"` | "Visual", "Rendering", "Appearance", "Bloom" | threshold, intensity, radius, softness |
| `"Color"` | "Tonemap", "Palette" | tint, saturation, hue shift |
| `"Trail"` | "Decay", "History", "Persistence" | decay, fade, accumulation |

Domain-specific names are fine when no standard name fits. The key rule: if a standard name applies, use it.

### Flat Effects Policy

- **5 or fewer params**: flat layout is fine — no sections needed.
- **6+ params**: must have `SeparatorText` sections following the canonical order.

---

## Audio Section Convention

When present, the Audio section always contains these params in this exact order:

```cpp
SeparatorText("Audio");
ModulatableSlider("Base Freq (Hz)", &cfg->baseFreq, "{name}.baseFreq", "%.1f", ms);
ModulatableSlider("Max Freq (Hz)",  &cfg->maxFreq,  "{name}.maxFreq",  "%.0f", ms);
ModulatableSlider("Gain",           &cfg->gain,      "{name}.gain",     "%.1f", ms);
ModulatableSlider("Contrast",       &cfg->curve,     "{name}.curve",    "%.2f", ms);
ModulatableSlider("Base Bright",    &cfg->baseBright, "{name}.baseBright", "%.2f", ms);
```

Standard ranges and defaults:

| Param | Range | Default | Format |
|-------|-------|---------|--------|
| baseFreq | 27.5 – 440 | 55 | `"%.1f"` |
| maxFreq | 1000 – 16000 | 14000 | `"%.0f"` |
| gain | 0.1 – 10 | 2.0 | `"%.1f"` |
| curve (Contrast) | 0.1 – 3 | 1.5 | `"%.2f"` |
| baseBright | 0 – 1 | **0.15** | `"%.2f"` |

Do NOT use `ModulatableSliderLog` for `baseFreq`. Use `ModulatableSlider`.

---

## Widget Selection

| Widget | When to Use |
|--------|------------|
| `ModulatableSlider()` | Float params with modulation support (most params) |
| `ModulatableSliderAngleDeg()` | Static angle params (radians stored, degrees displayed) |
| `ModulatableSliderSpeedDeg()` | Rotation speed params (rad/s stored, deg/s displayed, "/s" suffix) |
| `ModulatableSliderLog()` | Small float ranges like 0.01–1.0 (logarithmic scale) |
| `ModulatableSliderInt()` | Integer params stored as float for modulation compatibility |
| `ImGui::SliderInt()` | True integer fields not needing modulation (e.g., layer count, frequency multiplier) |
| `ImGui::Combo()` | Enum/mode selection |
| `ImGui::Checkbox()` | Boolean toggles |

### Param Registration

- ID format: `"effectName.fieldName"` (e.g., `"bloom.threshold"`, `"fluxWarp.cellScale"`)
- Bounds MUST match the range comments in the config struct header
- Angular fields use `ROTATION_OFFSET_MAX` bounds (not hardcoded `6.28f` or `PI`)
- Speed fields use `ROTATION_SPEED_MAX` bounds

---

## Common Mistakes — Do NOT Repeat

### Speed accumulation is ALWAYS on CPU

Speed values are accumulated on the CPU: `e->time += cfg->speed * deltaTime`. The shader receives accumulated time. NEVER pass speed as a shader uniform for time scaling. NEVER multiply by speed in the shader.

### Phase accumulation for time-dependent params

Any parameter that multiplies `time` in a shader MUST be CPU-accumulated as a phase instead. Pass the accumulated phase as the uniform, not the raw rate. This prevents discontinuous jumps when the user adjusts the slider at runtime.

### Angular params use degree sliders and ROTATION_OFFSET_MAX

Rotation angle params MUST use:
- `ModulatableSliderAngleDeg()` in the UI
- `ROTATION_OFFSET_MAX` bounds in RegisterParams
- `#include "config/constants.h"`

### Clamp distance before inverse-distance glow

Raymarcher distance `s` can hit zero at lattice seams → `1/s` = infinity. Always `max(s, 0.01)` before dividing.

### RegisterParams bounds must match config header

If the config struct says `float threshold = 0.8f; // (0.0-2.0)`, then `ModEngineRegisterParam` must use `0.0f, 2.0f`. Mismatched bounds cause silent modulation clipping.

### Reuse shared config types

Camera drift / Lissajous motion → use `DualLissajousConfig` with `DrawLissajousControls()`. Do NOT invent one-off panAmpX/panFreqX parameters.

### Bridge functions are NOT static

The `Setup<Name>(PostEffect* pe)` bridge function at the bottom of each effect `.cpp` must be non-static. It is referenced by the registration macro. The colocated `Draw<Name>Params` UI callback IS static.

### No magic scalars in shaders

Every constant must have a geometric or physical meaning. `0.5` = half-cell radius. `0.45` = nothing. If a scalar can't be named or derived, it's wrong.

---

## Generator Output Pattern

Generator effects use the `STANDARD_GENERATOR_OUTPUT(field)` macro for their Color/Output section. This draws gradient widget, blend intensity slider, and blend mode combo. Place immediately before the `REGISTER_GENERATOR` macro.

The generator param draw function follows the same section ordering rules as transform effects.

---

## Effects Window Structure (Reference)

The Effects window is the only panel using all 4 hierarchy levels:

```
DrawGroupHeader("FEEDBACK")        ← Level 1
  (sliders — blur, motion, etc.)
  DrawSectionBegin("Flow Field")   ← Level 3 (collapsible)
    SeparatorText("Base")          ← Level 4
    SeparatorText("Radial")
    ...
DrawGroupHeader("OUTPUT")
DrawGroupHeader("SIMULATIONS")
DrawGroupHeader("GENERATORS")
  DrawCategoryHeader("Geometric")  ← Level 2
    DrawSectionBegin("BitCrush")   ← Level 3 (collapsible)
      SeparatorText("Audio")       ← Level 4
      SeparatorText("Geometry")
      ...
DrawGroupHeader("TRANSFORMS")
  DrawCategoryHeader("Symmetry")
    DrawSectionBegin("Kaleidoscope")
      ...
```

Individual effects only control what's inside their `DrawSectionBegin` block. The dispatch system handles everything above.
