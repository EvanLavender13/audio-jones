# Multi-Stop Gradient Widget

Add a gradient color mode with an interactive multi-stop editor. Users can define custom color gradients that apply to waveforms, spectrum bars, and physarum agents. The widget supports adding/removing stops, repositioning via drag, and per-stop RGBA color editing.

## Current State

Gradient infrastructure exists but is unused:
- `src/render/color_config.h:15-18` - `GradientStop` struct defined (position, color)
- `src/render/color_config.h:29-30` - `gradientStops[8]` array and `gradientStopCount` in `ColorConfig`
- `src/render/color_config.h:11` - `COLOR_MODE_GRADIENT` enum value exists

Color application points that need gradient support:
- `src/render/waveform.cpp:16-28` - `GetSegmentColor()` for waveform segments
- `src/render/spectrum_bars.cpp:115-127` - `GetBandColor()` for spectrum bands
- `src/render/physarum.cpp:51-76` - `InitializeAgents()` for agent hue assignment

UI integration point:
- `src/ui/imgui_widgets.cpp:231-271` - `ImGuiDrawColorMode()` handles mode selection

Serialization:
- `src/config/preset.cpp:12-13` - `ColorConfig` JSON macro (gradient fields not included)

---

## Phase 1: Core Gradient Logic

**Goal**: Create shared gradient interpolation utility that all renderers can use.

**Build**:
- Create `src/render/gradient.h` with function declarations:
  - `GradientEvaluate(stops, count, t)` - RGB linear interpolation at position t
  - `GradientInitDefault(stops, count)` - Initialize cyan-to-magenta default
- Create `src/render/gradient.cpp` with implementations:
  - Edge case handling (t out of bounds, single stop)
  - Linear search for bracketing stops (max 8 stops, binary search unnecessary)
  - Per-channel RGB+A interpolation
- Update `CMakeLists.txt` to include new source file

**Done when**: `GradientEvaluate()` compiles and can be called from test code. Manual verification: returns cyan at t=0, magenta at t=1, blend at t=0.5.

---

## Phase 2: Renderer Integration

**Goal**: Waveforms, spectrum bars, and physarum render with gradient colors.

**Build**:
- Modify `src/render/color_config.h`:
  - Add in-class defaults for `gradientStops` array (cyan at 0.0, magenta at 1.0)
  - Set `gradientStopCount = 2` default
  - Remove "Future" comments from gradient fields
- Modify `src/render/waveform.cpp`:
  - Add `#include "gradient.h"`
  - Add `COLOR_MODE_GRADIENT` case in `GetSegmentColor()` calling `GradientEvaluate()`
- Modify `src/render/spectrum_bars.cpp`:
  - Add `#include "gradient.h"`
  - Add `COLOR_MODE_GRADIENT` case in `GetBandColor()` calling `GradientEvaluate()`
- Modify `src/render/physarum.cpp`:
  - Add `#include "gradient.h"`
  - Add `COLOR_MODE_GRADIENT` case in `InitializeAgents()`:
    - Sample gradient at `t = i / count` for each agent
    - Convert sampled RGB to HSV using existing `RGBToHSV()`
    - Assign hue to agent

**Done when**: Manually setting `color.mode = COLOR_MODE_GRADIENT` in debugger shows gradient-colored waveform/spectrum/physarum.

---

## Phase 3: Gradient Editor Widget

**Goal**: Interactive widget for editing gradient stops.

**Build**:
- Create `src/ui/gradient_editor.h` with function declaration:
  - `bool GradientEditor(const char* label, GradientStop* stops, int* count)`
- Create `src/ui/gradient_editor.cpp` with widget implementation:
  - Constants: `BAR_HEIGHT = 24px`, `HANDLE_RADIUS = 6px`, `DELETE_THRESHOLD = 40px`
  - `DrawGradientBar()` helper - sample gradient at 64+ points, render with `AddRectFilledMultiColor()`
  - `DrawStopHandles()` helper - circles at each stop position, glow on hover/active
  - Main widget logic:
    - Layout using `ItemSize()`, `ItemAdd()`, `GetID()`
    - `ButtonBehavior()` for bar interaction
    - StateStorage for drag state (which stop, if any)
    - StateStorage for color picker state (which stop has popup open)
  - Interactions:
    - Click empty bar: add stop with interpolated color at click position
    - Click handle: open `ColorPicker4()` popup for that stop
    - Drag handle horizontally: update stop position, clamp 0-1, prevent crossing neighbors
    - Drag handle off bar (>40px vertical): delete stop (except endpoints at 0.0/1.0)
  - Endpoint locking: stops at position 0.0 or 1.0 cannot be deleted
  - Visual feedback using theme colors (`Theme::GLOW_CYAN` for selected, `Theme::GLOW_MAGENTA` for dragging)
- Update `CMakeLists.txt` to include new source file

**Done when**: Widget renders in test harness, can add/remove/reposition stops, color picker opens on click.

---

## Phase 4: UI Integration

**Goal**: Gradient mode selectable in color section of all panels.

**Build**:
- Modify `src/ui/imgui_widgets.cpp`:
  - Add `#include "ui/gradient_editor.h"`
  - Add `#include "render/gradient.h"`
  - Change mode combo from `{"Solid", "Rainbow"}` to `{"Solid", "Rainbow", "Gradient"}`
  - Update combo item count from 2 to 3
  - Add `else if (color->mode == COLOR_MODE_GRADIENT)` branch:
    - If `gradientStopCount == 0`, call `GradientInitDefault()`
    - Call `GradientEditor("Gradient", color->gradientStops, &color->gradientStopCount)`
- Modify `src/ui/imgui_panels.h` (if needed):
  - Add declaration for `GradientEditor()` if making it public API

**Done when**: Selecting "Gradient" in waveform/spectrum/effects panels shows gradient editor, changes reflect in visualization.

---

## Phase 5: Preset Serialization

**Goal**: Gradient settings persist in saved presets.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GradientStop, position, color)` macro
  - Replace `ColorConfig` auto-macro with custom `to_json`/`from_json`:
    - Serialize only first `gradientStopCount` stops (not full array)
    - Include all existing fields plus `gradientStops`, `gradientStopCount`
  - Add validation in `from_json`:
    - Ensure `gradientStopCount >= 2` (fall back to default if corrupted)
    - Ensure stops sorted by position

**Done when**: Create 4-stop gradient, save preset, reload preset, gradient restored correctly. Verify JSON file contains gradient data.

---

## Phase 6: Polish

**Goal**: Refined visuals and edge case handling.

**Build**:
- Visual polish in `gradient_editor.cpp`:
  - Handle glow effects (cyan for hover, magenta for drag)
  - Locked endpoint indicator (different border or small icon)
  - Smooth gradient preview (increase sample count to 128 if needed)
- Edge case handling:
  - Max stops check: disable add when `count >= MAX_GRADIENT_STOPS`
  - Minimum spacing: prevent stops at identical positions
  - Sort stops after add/reposition (insertion sort, O(n) for 8 elements)
- Interaction polish:
  - Click-drag threshold before repositioning (avoid accidental drags when clicking to edit color)
  - Clear popup state when switching modes

**Done when**: Widget feels polished, handles all edge cases gracefully, matches synthwave theme aesthetic.
