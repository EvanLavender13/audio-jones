# Enhanced Analysis Panel & UI Toggle

Clean up floating screen text by moving performance info into the Analysis panel, and add a hotkey to hide the entire UI for an unobstructed visualization view.

## Current State

- `src/main.cpp:187-188` - Floating raylib `DrawText()` calls for FPS and [SPACE] hint
- `src/main.cpp:23-37` - `AppContext` struct (will add `uiVisible` field)
- `src/main.cpp:158-160` - Keyboard input handling (SPACE key toggle)
- `src/main.cpp:201-209` - ImGui panel rendering block
- `src/ui/imgui_analysis.cpp:278-312` - Analysis panel implementation

## Phase 1: Add UI Visibility Toggle

**Goal**: Tab key hides/shows all ImGui panels.

**Modify**:
- `src/main.cpp` - Add `bool uiVisible` to `AppContext`, initialize to `true`
- `src/main.cpp` - Add Tab key handler after line 160 (check `!io.WantCaptureKeyboard`)
- `src/main.cpp` - Wrap lines 201-209 (ImGui block) with `if (ctx->uiVisible)`

**Done when**: Pressing Tab toggles all UI panels on/off.

---

## Phase 2: Add Hidden UI Hint

**Goal**: Show minimal hint when UI is hidden so user knows how to restore it.

**Modify**:
- `src/main.cpp` - When `!ctx->uiVisible`, draw "[Tab] Show UI" at (10, 10) using raylib `DrawText()`

**Done when**: Small hint appears in corner when UI is hidden, disappears when UI is shown.

---

## Phase 3: Add Performance Section to Analysis Panel

**Goal**: Display FPS and frame time in the Analysis panel.

**Modify**:
- `src/ui/imgui_analysis.cpp` - Add new "Performance" section at top of panel
- Use `GetFPS()` and `GetFrameTime()` from raylib
- Style: Simple text display, matching existing section header pattern (accent color)

**Done when**: Analysis panel shows FPS and frame time at the top.

---

## Phase 4: Remove Floating Screen Text

**Goal**: Clean up the main screen by removing redundant text.

**Modify**:
- `src/main.cpp:187-188` - Remove both `DrawText()` calls for FPS and [SPACE] hint

**Note**: The [SPACE] hint for Linear/Circular mode will no longer be visible. If this is a problem, consider adding a "Controls" section to the Analysis panel in a future enhancement.

**Done when**: No floating text on main visualization screen (except the hidden-UI hint when applicable).
