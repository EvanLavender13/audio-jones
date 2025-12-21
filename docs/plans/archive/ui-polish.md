# UI Polish: Neon Eclipse Theme

Transform the generic gray Dear ImGui interface into a synthwave-inspired "Neon Eclipse" aesthetic with custom font, organized layout, and glow effects that complement the psychedelic visualizer.

## Current State

- `src/ui/imgui_panels.cpp:4-32` - `ImGuiApplyDarkTheme()` with generic grayscale colors
- `src/ui/imgui_analysis.cpp:14-22` - Already has gradient/glow patterns with custom colors
- `src/ui/imgui_analysis.cpp:44-129` - `DrawBeatGraph()` with intensity coloring
- `src/ui/imgui_analysis.cpp:159-289` - `DrawBandMeter()` with gradient bars
- `src/ui/imgui_widgets.cpp:18-125` - `HueRangeSlider()` custom widget
- `src/ui/imgui_effects.cpp:21-65` - Effects panel with 4 collapsing sections
- `src/main.cpp:129-132` - ImGui setup, no custom font
- `build/_deps/imgui-src/misc/fonts/Roboto-Medium.ttf` - Available font

## Phase 1: Theme Foundation

**Goal**: Replace grayscale theme with Neon Eclipse colors and load custom font.

**Build**:
- Create `src/ui/theme.h` with color constants namespace (BG_DEEP, BG_MID, CYAN, MAGENTA, ORANGE, TEXT, GLOW_CYAN)
- Modify `imgui_panels.cpp` to use Theme:: constants in renamed `ImGuiApplyNeonTheme()`
- Modify `main.cpp` to load Roboto-Medium.ttf after `rlImGuiSetup()`
- Update `imgui_panels.h` with renamed function declaration

**Done when**: All panels display cyan/magenta accents on deep blue-black base with Roboto font.

---

## Phase 2: Widget Helpers

**Goal**: Add reusable drawing functions and update Analysis panel colors.

**Build**:
- Add `DrawGradientBox()` to `imgui_widgets.cpp` using `AddRectFilledMultiColor()`
- Add `DrawGlow()` for expanded rect with glow color
- Add `DrawSectionHeader()` with colored bar and collapse state
- Add declarations to `imgui_panels.h`
- Update `imgui_analysis.cpp` color constants (lines 14-22) to use Theme::
- Update band colors (lines 132-136) to cyan/white/magenta

**Done when**: Analysis panel uses theme colors, helpers are available for other panels.

---

## Phase 3: Panel Polish

**Goal**: Apply section headers and gradients to remaining panels.

**Build**:
- Modify `imgui_effects.cpp` to use `DrawSectionHeader()` for Domain Warp, Voronoi, Physarum, LFO sections with rotating accent colors
- Modify `imgui_waveforms.cpp` to add section headers for Geometry/Animation/Color groups
- Modify `imgui_spectrum.cpp` to add gradient background
- Apply consistent backgrounds to Audio and Presets panels

**Done when**: All 6 panels have cohesive Neon Eclipse appearance with organized visual hierarchy.
