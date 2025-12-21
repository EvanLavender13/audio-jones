# Floating Panel UI Refactor

Replace cramped accordion panels with hybrid sidebar + floating windows using raygui native widgets.

## Goal

Transform the fixed 180px sidebar into a hybrid UI:
- **Sidebar**: Compact panels (Audio, Analysis) + toggle buttons for windows
- **Floating Windows**: Effects, Waveforms, and Spectrum panels in draggable, scrollable windows

Eliminates the ~25 parameter Effects panel cramming issue while preserving all existing widget logic and dropdown coordination.

## Current State

### Sidebar Architecture

`src/ui/ui_main.cpp:32-48` - `UIState`:
```cpp
struct UIState {
    PanelState panel;                    // Dropdown coordination
    WaveformPanelState* waveformPanel;   // List scroll state
    bool analysisSectionExpanded;        // Accordion states
    bool waveformSectionExpanded;
    bool spectrumSectionExpanded;
    bool audioSectionExpanded;
    bool effectsSectionExpanded;
    int lastPanelHeight;                 // Background height
};
```

`src/ui/ui_main.cpp:111-161` - Main draw function:
- Fixed x=10, width=180, stacked below preset panel
- Semi-transparent background (lines 116-117)
- 5 accordion sections via `DrawAccordionHeader()`
- Deferred dropdown rendering (lines 77-109)

### Panel Parameter Counts

| Panel | Controls | Notes |
|-------|----------|-------|
| Effects | ~29 | Blur/bloom 8 + Warp 5 + Voronoi 4 + Physarum 12 |
| Waveforms | ~11 | List + 9 settings sliders + color |
| Spectrum | ~6 | Geometry + color |
| Audio | 1 | Channel mode dropdown |
| Analysis | 2 | Beat graph + band meter |

### Layout System

`src/ui/ui_layout.cpp` - Declarative row/slot positioning:
- `UILayoutBegin(x, y, width, padding, spacing)` - Create container
- `UILayoutRow(height)` - Advance to next row
- `UILayoutSlot(widthRatio)` - Return Rectangle for widget
- `UILayoutEnd()` - Return final Y position (height measurement)

## Algorithm

### Window State Structure

```cpp
typedef struct WindowState {
    Vector2 position;       // Top-left corner (default position on launch)
    Vector2 size;           // Fixed dimensions
    Vector2 scroll;         // GuiScrollPanel scroll offset
    bool visible;           // Show/hide state
    int contentHeight;      // Measured from previous frame for scroll bounds
} WindowState;
```

### Window Wrapper Pattern

```cpp
bool UIWindowBegin(WindowState* win, const char* title, UILayout* outLayout)
{
    if (!win->visible) return false;

    Rectangle bounds = {win->position.x, win->position.y, win->size.x, win->size.y};

    // 1. Window frame with close button
    if (GuiWindowBox(bounds, title)) {
        win->visible = false;
        return false;
    }

    // 2. Handle title bar drag (top 24px)
    Rectangle titleBar = {bounds.x, bounds.y, bounds.width, 24};
    static Vector2 dragOffset;
    static bool dragging = false;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetMousePosition(), titleBar)) {
        dragging = true;
        dragOffset.x = GetMousePosition().x - bounds.x;
        dragOffset.y = GetMousePosition().y - bounds.y;
    }
    if (dragging) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            win->position.x = GetMousePosition().x - dragOffset.x;
            win->position.y = GetMousePosition().y - dragOffset.y;
        } else {
            dragging = false;
        }
    }

    // 3. Scroll panel for content area
    Rectangle panelArea = {bounds.x, bounds.y + 24, bounds.width, bounds.height - 24};
    Rectangle contentSize = {0, 0, bounds.width - 14, (float)win->contentHeight}; // -14 for scrollbar
    Rectangle view;
    GuiScrollPanel(panelArea, NULL, contentSize, &win->scroll, &view);

    // 4. Begin scissor mode for content
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

    // 5. Create layout with scroll offset applied
    *outLayout = UILayoutBegin(
        (int)view.x,
        (int)view.y - (int)win->scroll.y,
        (int)view.width, 8, 4
    );

    return true;
}

void UIWindowEnd(WindowState* win, UILayout* layout)
{
    // Measure content height for next frame
    win->contentHeight = UILayoutEnd(layout) - (int)(win->position.y + 24 - win->scroll.y);
    EndScissorMode();
}
```

### Sidebar Toggle Buttons

Replace accordion headers for floating panels with toggle buttons:

```cpp
// In UIDrawWaveformPanel() - after Audio accordion section
UILayoutRow(&l, 20);
GuiToggle(UILayoutSlot(&l, 1.0f), "Effects", &state->effectsWindow.visible);
UILayoutRow(&l, 20);
GuiToggle(UILayoutSlot(&l, 1.0f), "Waveforms", &state->waveformsWindow.visible);
```

## Architecture Decision

**Minimal Wrapper Approach**

Wrap existing panel functions in `GuiWindowBox` + `GuiScrollPanel` without modifying panel internals.

**Rationale:**
- Zero changes to `UIDrawEffectsPanel()`, `UIDrawWaveformListGroup()`, etc.
- Reuses `UILayout` system unchanged
- Preserves `PanelState` dropdown coordination
- Uses raygui native widgets only

**Trade-offs:**
| Approach | Pros | Cons |
|----------|------|------|
| Full UI rewrite | Clean slate | Rewrites 500+ lines, breaks presets |
| Window manager | Generic system | Over-engineering for 2 windows |
| **Thin wrapper** | **Max reuse, min change** | Manual z-order | **Chosen** |

## Component Design

### New Files

**`src/ui/ui_window.h`**
```cpp
#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "raylib.h"
#include "ui_layout.h"
#include <stdbool.h>

typedef struct WindowState {
    Vector2 position;
    Vector2 size;
    Vector2 scroll;
    bool visible;
    int contentHeight;
} WindowState;

// Begin window with scroll panel. Returns false if closed/hidden.
// On success, outLayout is initialized for content drawing.
bool UIWindowBegin(WindowState* win, const char* title, UILayout* outLayout);

// End window, measure content height, close scissor mode.
void UIWindowEnd(WindowState* win, UILayout* layout);

// Check if mouse is over window (for input blocking).
bool UIWindowIsHovered(WindowState* win);

#endif // UI_WINDOW_H
```

**`src/ui/ui_window.cpp`**
- Implement `UIWindowBegin()` with GuiWindowBox + drag handling + GuiScrollPanel
- Implement `UIWindowEnd()` with content height measurement
- Implement `UIWindowIsHovered()` for z-order input blocking

### Modified Files

**`src/ui/ui_main.h`** - Add to UIState:
```cpp
WindowState effectsWindow;
WindowState waveformsWindow;
WindowState spectrumWindow;
```

**`src/ui/ui_main.cpp`**

1. Initialize windows in `UIStateInit()`:
```cpp
state->effectsWindow = {
    .position = {400, 100},
    .size = {260, 600},
    .scroll = {0, 0},
    .visible = true,
    .contentHeight = 600
};
state->waveformsWindow = {
    .position = {680, 100},
    .size = {240, 400},
    .scroll = {0, 0},
    .visible = true,
    .contentHeight = 400
};
state->spectrumWindow = {
    .position = {200, 100},
    .size = {220, 350},
    .scroll = {0, 0},
    .visible = true,
    .contentHeight = 350
};
```

2. Add window draw functions:
```cpp
static void DrawEffectsWindow(UIState* state, AppConfigs* configs)
{
    UILayout l;
    if (!UIWindowBegin(&state->effectsWindow, "Effects", &l)) return;

    EffectsPanelDropdowns dd = UIDrawEffectsPanel(&l, &state->panel, configs->effects);

    // Deferred dropdowns inside scissor
    DrawDeferredDropdown(dd.lfoWaveform, configs->effects->rotationLFO.enabled,
                         "Sine;Triangle;Saw;Square;S&&H",
                         &configs->effects->rotationLFO.waveform,
                         &state->panel.lfoWaveformDropdownOpen);

    int physarumColorMode = (int)configs->effects->physarum.color.mode;
    DrawDeferredDropdown(dd.physarumColor, configs->effects->physarum.enabled,
                         "Solid;Rainbow", &physarumColorMode,
                         &state->panel.physarumColorModeDropdownOpen);
    configs->effects->physarum.color.mode = (ColorMode)physarumColorMode;

    UIWindowEnd(&state->effectsWindow, &l);
}

static void DrawWaveformsWindow(UIState* state, AppConfigs* configs)
{
    UILayout l;
    if (!UIWindowBegin(&state->waveformsWindow, "Waveforms", &l)) return;

    UIDrawWaveformListGroup(&l, state->waveformPanel, configs->waveforms,
                            configs->waveformCount, configs->selectedWaveform);

    if (*configs->selectedWaveform >= 0 &&
        *configs->selectedWaveform < *configs->waveformCount) {
        WaveformConfig* sel = &configs->waveforms[*configs->selectedWaveform];
        Rectangle colorDropdown = UIDrawWaveformSettingsGroup(&l, &state->panel, sel,
                                                               *configs->selectedWaveform);

        int colorMode = (int)sel->color.mode;
        DrawDeferredDropdown(colorDropdown, true, "Solid;Rainbow",
                             &colorMode, &state->panel.colorModeDropdownOpen);
        sel->color.mode = (ColorMode)colorMode;
    }

    UIWindowEnd(&state->waveformsWindow, &l);
}
```

3. Modify `UIDrawWaveformPanel()`:
- Remove `waveformSectionExpanded` and `effectsSectionExpanded` usage
- Add toggle buttons for window visibility after Audio section
- Call `DrawEffectsWindow()` and `DrawWaveformsWindow()` at end

### Minimal/Flat Aesthetic

Apply flat styling via `GuiSetStyle()` in `UIStateInit()`:

```cpp
GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(DARKGRAY));
GuiSetStyle(TOGGLE, BASE_COLOR_NORMAL, ColorToInt(DARKGRAY));
```

## File Changes

| File | Change |
|------|--------|
| `src/ui/ui_window.h` | Create - WindowState struct, UIWindowBegin/End/IsHovered |
| `src/ui/ui_window.cpp` | Create - Window wrapper implementation |
| `src/ui/ui_main.h` | Modify - Add WindowState fields to UIState |
| `src/ui/ui_main.cpp` | Modify - Init windows, add DrawEffectsWindow/DrawWaveformsWindow, add toggle buttons, remove Effects/Waveforms accordions |
| `src/ui/ui_panel_effects.cpp` | No change |
| `src/ui/ui_panel_waveform.cpp` | No change |
| `CMakeLists.txt` | Modify - Add ui_window.cpp to sources |

## Build Sequence

### Phase 1: Window Infrastructure
**Estimated: 1 session**

- [x] Create `src/ui/ui_window.h` with WindowState struct
- [x] Create `src/ui/ui_window.cpp` with UIWindowBegin/End implementation
- [x] Implement title bar drag detection
- [x] Implement GuiScrollPanel integration with scissor mode
- [x] Implement UIWindowIsHovered for input blocking
- [x] Add to CMakeLists.txt
- [x] Test: single window with dummy content, verify drag + scroll + close

### Phase 2: Effects Window
**Estimated: 1 session**

- [x] Add `WindowState effectsWindow` to UIState
- [x] Initialize with default position (400, 100) and size (260, 600)
- [x] Implement `DrawEffectsWindow()` static function
- [x] Move UIDrawEffectsPanel call inside UIWindowBegin/End
- [x] Move LFO/Physarum deferred dropdowns inside window
- [x] Test: all sliders, toggles, dropdowns work in floating window
- [x] Test: scroll reveals all controls when fully expanded

### Phase 3: Waveforms Window
**Estimated: 1 session**

- [x] Add `WindowState waveformsWindow` to UIState
- [x] Initialize with default position (680, 100) and size (240, 400)
- [x] Implement `DrawWaveformsWindow()` static function
- [x] Move UIDrawWaveformListGroup and UIDrawWaveformSettingsGroup inside
- [x] Move waveform color dropdown inside window
- [x] Test: New button, list selection, settings, color picker all work

### Phase 3b: Spectrum Window
**Estimated: 1 session**

- [x] Add `WindowState spectrumWindow` to UIState
- [x] Initialize with default position (200, 100) and size (220, 350)
- [x] Implement `DrawSpectrumWindow()` static function
- [x] Move UIDrawSpectrumPanel call inside UIWindowBegin/End
- [x] Move spectrum color dropdown inside window
- [x] Remove Spectrum accordion section from sidebar
- [x] Add "Spectrum" toggle button to sidebar
- [x] Test: all sliders, color controls work in floating window

### Phase 4: Sidebar Integration
**Estimated: 1 session**

- [x] Remove `waveformSectionExpanded` and `effectsSectionExpanded` from UIState
- [x] Remove Effects and Waveforms accordion sections from UIDrawWaveformPanel
- [x] Add "Effects" and "Waveforms" toggle buttons to sidebar
- [x] Wire toggle buttons to WindowState.visible fields
- [x] Adjust sidebar background height calculation
- [x] Apply flat styling via GuiSetStyle
- [x] Test: toggle buttons show/hide windows

### Phase 5: Polish
**Estimated: 1 session**

- [x] Implement z-order: last-clicked window draws on top
- [x] Disable sidebar controls when dragging window
- [x] Verify no visual glitches with scissor clipping during scroll
- [x] Test preset save/load still works (window state not persisted)
- [x] Test with all effects enabled (Physarum, Voronoi, Warp, LFO)
- [x] Test with 8 waveforms in list

## Validation

### Functional
- [ ] Effects window displays all ~29 parameters with vertical scroll
- [ ] Waveforms window displays list (80px) + settings (9 controls) with scroll
- [ ] Windows draggable by title bar, cannot resize
- [ ] Close button hides window; toggle button shows/hides
- [ ] Sidebar toggle buttons correctly reflect window visibility
- [ ] Window positions reset to defaults on app restart

### Integration
- [ ] Analysis/Spectrum/Audio accordions still work in sidebar
- [ ] Deferred dropdowns render correctly within window scissor regions
- [ ] Dropdown open/close coordination works across windows and sidebar
- [ ] Clicking through inactive window blocked (z-order input handling)
- [ ] Preset save/load works unchanged

### Visual
- [ ] No scissor clipping artifacts during scroll
- [ ] Scrollbar appears only when content exceeds window height
- [ ] Flat/minimal aesthetic applied (thin borders, clean typography)

## References

- `src/ui/ui_layout.cpp:43-46` - UILayoutEnd pattern for height measurement
- `src/ui/ui_main.cpp:116-117` - Background rendering pattern
- `src/ui/ui_common.cpp` - Deferred dropdown coordination
- raygui scroll_panel example - GuiScrollPanel usage
- raygui portable_window example - GuiWindowBox drag pattern
