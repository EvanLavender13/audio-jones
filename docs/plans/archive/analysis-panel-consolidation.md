# Analysis Panel Consolidation

Consolidate beat detection graph and band meter into a new Analysis panel, removing sensitivity sliders.

## Goal

Simplify the UI by grouping analysis visualizations (beat detection graph, band meter) into a single "Analysis" panel positioned between Presets and Waveforms. Remove the band sensitivity sliders and set their default values to 0.5 (minimum).

## Current State

The beat detection graph and band meter live in separate panels:

- Beat graph renders in Effects panel (`src/ui/ui_panel_effects.cpp:44-45`)
- Band meter and sensitivity sliders render in Bands panel (`src/ui/ui_panel_bands.cpp:18-24`)

Current accordion order in `src/ui/ui_main.cpp:65-90`:
1. Waveforms
2. Spectrum
3. Audio
4. Effects
5. Bands

Band sensitivity defaults are 1.0 (`src/config/band_config.h:5-7`) with slider range 0.5-2.0.

## Changes

### New Panel Contents

The Analysis panel will contain:
1. Beat detection graph (40px height, moved from Effects)
2. Band meter widget (36px height, moved from Bands)

No sliders or other controls.

### New Accordion Order

1. **Analysis** (new)
2. Waveforms
3. Spectrum
4. Audio
5. Effects

The Bands panel will be deleted entirely.

### Default Sensitivity Values

Change `BandConfig` defaults from 1.0 to 0.5 (the slider minimum):

```cpp
struct BandConfig {
    float bassSensitivity = 0.5f;
    float midSensitivity = 0.5f;
    float trebSensitivity = 0.5f;
};
```

### Band Meter Widget

Remove the background rectangle from `GuiBandMeter` (`src/ui/ui_widgets.cpp:164-166`):

```cpp
// Remove these lines:
DrawRectangleRec(bounds, { 30, 30, 30, 255 });
DrawRectangleLinesEx(bounds, 1, { 60, 60, 60, 255 });
```

The widget will render directly on the panel background without its own container.

## File Changes

| File | Change |
|------|--------|
| `src/ui/ui_panel_analysis.h` | Create - declare `UIDrawAnalysisPanel` |
| `src/ui/ui_panel_analysis.cpp` | Create - render beat graph and band meter |
| `src/ui/ui_main.cpp` | Add Analysis panel first in accordion, remove Bands section, add include |
| `src/ui/ui_panel_effects.cpp` | Remove beat graph rendering (lines 44-45) |
| `src/ui/ui_panel_bands.h` | Delete |
| `src/ui/ui_panel_bands.cpp` | Delete |
| `src/config/band_config.h` | Change defaults from 1.0 to 0.5 |
| `src/ui/ui_widgets.cpp` | Remove background rectangle from `GuiBandMeter` |
| `CMakeLists.txt` | Remove `ui_panel_bands.cpp`, add `ui_panel_analysis.cpp` |

## Validation

- [ ] Analysis panel appears below Presets in accordion
- [ ] Beat graph displays in Analysis panel
- [ ] Band meter displays in Analysis panel
- [ ] Effects panel no longer shows beat graph
- [ ] No Bands panel in UI
- [ ] Band sensitivity defaults to 0.5 (verify via preset save/load)
- [ ] Band meter renders without separate background rectangle
- [ ] Project compiles without warnings
