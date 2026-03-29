---
name: ui-guide
description: Use when editing UI code in any panel or effect Draw*Params function. Enforces the Signal Stack design language. Triggers on UI edits to src/ui/, src/effects/ UI sections, drawable_type_controls.cpp, or when reviewing effect UI for consistency.
---

# Signal Stack UI Rules

Follow these rules when writing or modifying UI code.

---

## Divider Patterns

Three patterns for three contexts:

**`SeparatorText("Label")`** - Universal section divider. Lightweight inline text with theme border. No interaction. The default choice.

**Module Strip (`DrawModuleStripBegin/End`)** - Compact repeating items. 3px left accent bar, background tint (accent at ~8% alpha), top rim-light (1px, accent at ~24% alpha), enable indicator dot (click-to-toggle). Always visible, never collapsible. Accent cycles per item via `Theme::GetSectionAccent(i)`.

**`DrawSectionBegin/End()`** - Collapsible sections. Gradient box + left accent bar + collapse arrow. Effects window per-effect entries only.

### Collapsibility

**Collapsible**: Only per-effect entries in the Effects window.

**Never collapsible**: Everything else. If the user navigated there, they want to see it.

| Context | Use | Collapsible? |
|---------|-----|-------------|
| Per-effect entry in Effects window | `DrawSectionBegin/End()` | Yes |
| Subsections inside an expanded effect | `SeparatorText()` | No |
| LFO modules | `DrawModuleStripBegin/End()` | No |
| All other panel sections | `SeparatorText()` | No |

### Hierarchy

```
Level 0: Window Title (docking tab)
Level 1: DrawGroupHeader()       -- Effects window pipeline groups ONLY
Level 2: DrawCategoryHeader()    -- Effects window transform categories ONLY
Level 3: DrawSectionBegin/End()  -- Effects window per-effect ONLY (collapsible)
Level 4: SeparatorText()         -- Universal section divider (everywhere)
```

Most panels use only Level 0 and Level 4. Only the Effects window uses all five.

### Forbidden Patterns

- `TextColored()` as a section header
- `TreeNodeAccented()` (deprecated, convert to `SeparatorText`)
- `ImGui::Separator()` + `ImGui::Spacing()` as a manual section break
- `DrawGroupHeader()` restating the window title
- Collapsible subsections within an expanded effect

---

## Effect Section Ordering

Canonical order for `SeparatorText` subsections in `Draw<Name>Params()`. Skip what doesn't apply.

```
1. Audio       -- FFT: baseFreq, maxFreq, gain, contrast, baseBright
2. Geometry    -- Spatial: scale, size, layers, radius, count, offset
3. [Domain]    -- Effect-specific (0-4 sections, unique names)
4. Animation   -- Time: speed, phase, frequency, spin, drift
5. Glow        -- Brightness: threshold, intensity, radius, softness
6. Color       -- Color: tint, saturation, hue shift
```

**6+ params requires sections.** 5 or fewer can be flat.

---

## Section Names

| Use | Not |
|-----|-----|
| `"Audio"` | -- |
| `"Geometry"` | "Shape", "Grid", "Layout", "Structure" |
| `"Animation"` | "Motion", "Timing", "Dynamics", "Movement" |
| `"Glow"` | "Visual", "Rendering", "Appearance", "Bloom" |
| `"Color"` | "Tonemap", "Palette" |
| `"Trail"` | "Decay", "History", "Persistence" |

Domain-specific names are fine when no standard name fits (e.g., "Flocking", "Pressure", "Wave", "Raymarching").

---

## Audio Section

When present, always this order:

```cpp
SeparatorText("Audio");
ModulatableSlider("Base Freq (Hz)", &cfg->baseFreq, "{name}.baseFreq", "%.1f", ms);  // 27.5-440, default 55
ModulatableSlider("Max Freq (Hz)",  &cfg->maxFreq,  "{name}.maxFreq",  "%.0f", ms);  // 1000-16000, default 14000
ModulatableSlider("Gain",           &cfg->gain,      "{name}.gain",     "%.1f", ms);  // 0.1-10, default 2.0
ModulatableSlider("Contrast",       &cfg->curve,     "{name}.curve",    "%.2f", ms);  // 0.1-3, default 1.5
ModulatableSlider("Base Bright",    &cfg->baseBright, "{name}.baseBright", "%.2f", ms); // 0-1, default 0.15
```

Do not use `ModulatableSliderLog` for `baseFreq`.

---

## Widget Selection

| Widget | When |
|--------|------|
| `ModulatableSlider()` | Float params with modulation |
| `ModulatableSliderAngleDeg()` | Static angle (radians stored, degrees displayed) |
| `ModulatableSliderSpeedDeg()` | Rotation speed (rad/s stored, deg/s displayed) |
| `ModulatableSliderLog()` | Small float ranges like 0.01-1.0 |
| `ModulatableSliderInt()` | Integer params stored as float for modulation |
| `ImGui::SliderInt()` | True integers, no modulation |
| `ImGui::Combo()` | Enum/mode selection |
| `ImGui::Checkbox()` | Boolean toggles |

---

## Generator Output

Use `STANDARD_GENERATOR_OUTPUT(field)` macro for the Color/Output section. Place immediately before `REGISTER_GENERATOR`. Same section ordering rules apply to the params function.
