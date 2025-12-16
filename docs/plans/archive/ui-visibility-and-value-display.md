# UI Visibility and Value Display

Improve UI panel readability with semi-transparent background and slider value/unit display.

## Goal

Address two usability issues:
1. **Transparent UI**: Panel controls are hard to see against active visualizations
2. **Missing values**: Sliders don't show current values or units

Solution: Add dynamic semi-transparent background behind the UI panel and display values with units after all sliders.

## Current State

### Panel Rendering

The UI panel has no background - controls render directly over visualizations:

`src/ui/ui_main.cpp:105-147` - `UIDrawWaveformPanel()`:
- Line 107: `UILayoutBegin(10, startY, 180, 8, 4)` creates layout at x=10, width=180
- Returns final Y position for content below
- No background rendering

`src/ui/ui_main.cpp:32-45` - `UIState` struct:
- Tracks accordion expansion states and panel state
- No height tracking for background

### Slider Widgets

`src/ui/ui_widgets.cpp:14-20` - `DrawLabeledSlider()`:
```cpp
void DrawLabeledSlider(UILayout* l, const char* label, float* value, float min, float max)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, value, min, max);
}
```

`src/ui/ui_widgets.cpp:22-30` - `DrawIntSlider()`:
```cpp
void DrawIntSlider(UILayout* l, const char* label, int* value, int min, int max)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    float floatVal = (float)*value;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &floatVal, (float)min, (float)max);
    *value = lroundf(floatVal);
}
```

Both pass `NULL` for value display. No unit support.

### Manual Value Display

Two rotation sliders manually show values using `TextFormat()`:

`src/ui/ui_panel_waveform.cpp:79-82`:
```cpp
DrawText(TextFormat("Rot %.3f", sel->rotationSpeed), l->x + l->padding, l->y + 4, 10, GRAY);
(void)UILayoutSlot(l, 0.38f);
GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rotationSpeed, -0.05f, 0.05f);
```

`src/ui/ui_panel_spectrum.cpp:34-37` - Same pattern.

These should be replaced with the standard slider wrapper.

### Slider Callsites

| File | Calls | Lines |
|------|-------|-------|
| `src/ui/ui_panel_effects.cpp` | 22 | 22-52, 64 |
| `src/ui/ui_panel_spectrum.cpp` | 7 | 24-31, 39 |
| `src/ui/ui_panel_waveform.cpp` | 5 | 73-76, 84 |
| `src/ui/ui_color.cpp` | 2 | 52-53 |
| **Total** | **36** | |

Plus 2 manual rotation sliders to convert.

## Architecture Decision

**Pragmatic approach with dynamic height tracking**

### Background Strategy

The panel height changes when accordion sections expand/collapse. A fixed-height background would overdraw when collapsed or underdraw when expanded.

**Solution**: Track panel height from previous frame.
1. Store `lastPanelHeight` in `UIState`
2. Draw background at START using stored height
3. Calculate actual height at END and update stored value
4. One-frame convergence is imperceptible

### Value Display Strategy

Add right-aligned value text after slider within the same row:
- Use `MeasureText()` for right-alignment calculation
- Draw with `LIGHTGRAY` (dimmer than labels)
- Format: `"%.2f"` for floats, `"%d"` for ints
- Append unit string with space: `"0.50 s"`

### Unit Specification

Add `const char* unit` parameter to slider functions:
- `NULL` or `""` for unitless values
- `"dB"`, `"s"`, `"rad"`, `"px"`, `"Hz"` for labeled values

## Component Design

### UIState Modification

`src/ui/ui_main.cpp:32-45`

Add field:
```cpp
struct UIState {
    // ... existing fields ...

    // Panel background height (from previous frame)
    int lastPanelHeight;
};
```

Initialize in `UIStateInit()` at line 48:
```cpp
state->lastPanelHeight = 300;  // Reasonable default
```

### Slider Function Signatures

`src/ui/ui_widgets.h:14,18`

```cpp
void DrawLabeledSlider(UILayout* l, const char* label, float* value,
                       float min, float max, const char* unit);
void DrawIntSlider(UILayout* l, const char* label, int* value,
                   int min, int max, const char* unit);
```

### Slider Implementation

`src/ui/ui_widgets.cpp:14-30`

```cpp
void DrawLabeledSlider(UILayout* l, const char* label, float* value,
                       float min, float max, const char* unit)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    Rectangle sliderRect = UILayoutSlot(l, 1.0f);
    GuiSliderBar(sliderRect, NULL, NULL, value, min, max);

    // Value display (right-aligned within slider area)
    char valueText[32];
    if (unit != NULL && unit[0] != '\0') {
        snprintf(valueText, sizeof(valueText), "%.2f %s", *value, unit);
    } else {
        snprintf(valueText, sizeof(valueText), "%.2f", *value);
    }
    int textWidth = MeasureText(valueText, 10);
    DrawText(valueText, (int)(sliderRect.x + sliderRect.width - textWidth - 4),
             l->y + 4, 10, LIGHTGRAY);
}

void DrawIntSlider(UILayout* l, const char* label, int* value,
                   int min, int max, const char* unit)
{
    UILayoutRow(l, ROW_HEIGHT);
    DrawText(label, l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, LABEL_RATIO);
    float floatVal = (float)*value;
    Rectangle sliderRect = UILayoutSlot(l, 1.0f);
    GuiSliderBar(sliderRect, NULL, NULL, &floatVal, (float)min, (float)max);
    *value = lroundf(floatVal);

    // Value display (right-aligned within slider area)
    char valueText[32];
    if (unit != NULL && unit[0] != '\0') {
        snprintf(valueText, sizeof(valueText), "%d %s", *value, unit);
    } else {
        snprintf(valueText, sizeof(valueText), "%d", *value);
    }
    int textWidth = MeasureText(valueText, 10);
    DrawText(valueText, (int)(sliderRect.x + sliderRect.width - textWidth - 4),
             l->y + 4, 10, LIGHTGRAY);
}
```

### Panel Background

`src/ui/ui_main.cpp:105-147`

At start of `UIDrawWaveformPanel()`, after line 107:
```cpp
UILayout l = UILayoutBegin(10, startY, 180, 8, 4);

// Draw semi-transparent background using previous frame's height
DrawRectangleRec({10.0f, (float)startY, 180.0f, (float)state->lastPanelHeight},
                 Fade(BLACK, 0.7f));

DeferredDropdowns dd = {0};
```

At end, before return at line 146:
```cpp
// Update panel height for next frame's background
state->lastPanelHeight = l.y - startY + l.spacing;

return l.y;
```

## File Changes

| File | Change |
|------|--------|
| `src/ui/ui_widgets.h` | Modify - Add `unit` parameter to slider declarations (lines 14, 18) |
| `src/ui/ui_widgets.cpp` | Modify - Implement value display in both slider functions (lines 14-30) |
| `src/ui/ui_main.cpp` | Modify - Add `lastPanelHeight` to UIState (line 44), init (line 53), draw background (line 108), update height (line 145) |
| `src/ui/ui_panel_effects.cpp` | Modify - Add unit strings to 22 slider calls |
| `src/ui/ui_panel_spectrum.cpp` | Modify - Add unit strings to 7 slider calls, replace manual rotation (lines 34-37) |
| `src/ui/ui_panel_waveform.cpp` | Modify - Add unit strings to 5 slider calls, replace manual rotation (lines 79-82) |
| `src/ui/ui_color.cpp` | Modify - Add unit strings to 2 slider calls |

## Build Sequence

### Phase 1: Core Infrastructure

**Widget signatures and implementation**

- [ ] Update `DrawLabeledSlider` signature in `src/ui/ui_widgets.h:14` - add `const char* unit`
- [ ] Update `DrawIntSlider` signature in `src/ui/ui_widgets.h:18` - add `const char* unit`
- [ ] Implement value display in `DrawLabeledSlider` at `src/ui/ui_widgets.cpp:14-20`
- [ ] Implement value display in `DrawIntSlider` at `src/ui/ui_widgets.cpp:22-30`
- [ ] Build (expect compile errors at callsites)

**Panel background**

- [ ] Add `int lastPanelHeight;` to UIState struct at `src/ui/ui_main.cpp:44`
- [ ] Initialize `state->lastPanelHeight = 300;` in `UIStateInit()` at `src/ui/ui_main.cpp:53`
- [ ] Draw background after `UILayoutBegin()` at `src/ui/ui_main.cpp:108`
- [ ] Update `lastPanelHeight` before return at `src/ui/ui_main.cpp:145`

### Phase 2: Callsite Updates

**Effects panel** (`src/ui/ui_panel_effects.cpp`)

| Line | Label | Unit |
|------|-------|------|
| 22 | Blur | `NULL` |
| 23 | Half-life | `"s"` |
| 24 | Bloom | `NULL` |
| 25 | Chroma | `"px"` |
| 26 | Zoom | `NULL` |
| 27 | Rotation | `"rad"` |
| 28 | Desat | `NULL` |
| 29 | Kaleido | `NULL` |
| 32 | Voronoi | `NULL` |
| 34 | V.Scale | `NULL` |
| 35 | V.Speed | `NULL` |
| 36 | V.Edge | `NULL` |
| 44 | P.Agents | `NULL` |
| 45 | P.Sensor | `"px"` |
| 46 | P.Angle | `"rad"` |
| 47 | P.Turn | `"rad"` |
| 48 | P.Step | `"px"` |
| 49 | P.Deposit | `NULL` |
| 50 | P.Decay | `"s"` |
| 51 | P.Diffuse | `NULL` |
| 52 | P.Boost | `NULL` |
| 64 | Rate | `"Hz"` |

**Spectrum panel** (`src/ui/ui_panel_spectrum.cpp`)

| Line | Label | Unit |
|------|-------|------|
| 24 | Radius | `NULL` |
| 25 | Height | `NULL` |
| 26 | Width | `NULL` |
| 29 | Smooth | `NULL` |
| 30 | Min dB | `"dB"` |
| 31 | Max dB | `"dB"` |
| 34-37 | Rotation | Replace with `DrawLabeledSlider(l, "Rotation", &config->rotationSpeed, -0.05f, 0.05f, "rad");` |
| 39 | Offset | `"rad"` |

**Waveform panel** (`src/ui/ui_panel_waveform.cpp`)

| Line | Label | Unit |
|------|-------|------|
| 73 | Radius | `NULL` |
| 74 | Height | `NULL` |
| 75 | Thickness | `"px"` |
| 76 | Smooth | `NULL` |
| 79-82 | Rotation | Replace with `DrawLabeledSlider(l, "Rotation", &sel->rotationSpeed, -0.05f, 0.05f, "rad");` |
| 84 | Offset | `"rad"` |

**Color panel** (`src/ui/ui_color.cpp`)

| Line | Label | Unit |
|------|-------|------|
| 52 | Sat | `NULL` |
| 53 | Bright | `NULL` |

### Phase 3: Build and Test

- [ ] Build project: `cmake.exe --build build`
- [ ] Run application and verify background appears
- [ ] Test accordion collapse/expand - background should resize
- [ ] Verify all slider values display correctly
- [ ] Check value alignment on various slider positions
- [ ] Verify units appear with correct spacing

## Validation

- [ ] Semi-transparent background visible behind entire UI panel
- [ ] Background height adjusts when accordion sections expand/collapse
- [ ] All 36 sliders display current values
- [ ] Values update in real-time during drag
- [ ] Float values show 2 decimal places
- [ ] Integer values show whole numbers
- [ ] Unit strings appear with proper spacing
- [ ] Text is readable (LIGHTGRAY on dark background)
- [ ] No layout shifts or text overlap
- [ ] Manual rotation displays removed (replaced with standard sliders)
- [ ] 70% opacity provides sufficient contrast without obscuring visuals

## References

- Existing manual value display pattern: `src/ui/ui_panel_waveform.cpp:79-82`
- Custom widget background: `src/ui/ui_widgets.cpp:35` (`{30, 30, 30, 255}` in `GuiBeatGraph`)
- raylib text measurement: `MeasureText()` API
- raylib color fade: `Fade(Color, float alpha)`
- Unit definitions from config headers: `src/config/effect_config.h:7-28`
