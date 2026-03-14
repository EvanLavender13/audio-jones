# Preset Playlist

An ordered setlist of presets navigated with Left/Right arrow keys. A dedicated ImGui window provides a transport strip (now-playing, position, prev/next), a drag-reorderable setlist, and save/load controls. Playlists are persisted as lightweight JSON files referencing preset paths.

**Research**: `docs/research/preset_playlist.md`

## Design

### Types

**`Playlist` (in `src/config/playlist.h`):**

```c
#define MAX_PLAYLIST_ENTRIES 64
#define PLAYLIST_DIR "playlists"

struct Playlist {
    char name[PRESET_NAME_MAX];
    char entries[MAX_PLAYLIST_ENTRIES][PRESET_PATH_MAX];  // Relative preset paths
    int entryCount;
    int activeIndex;  // Currently-playing index (-1 = none)
};
```

**Exposed preset accessors (added to `imgui_panels.h`, implemented in `imgui_presets.cpp`):**

```c
const char* ImGuiGetLoadedPresetPath(void);
void ImGuiLoadPreset(const char* filepath, AppConfigs* configs);
```

**Playlist panel + keybind API (added to `imgui_panels.h`, implemented in `imgui_playlist.cpp`):**

```c
void ImGuiDrawPlaylistPanel(AppConfigs* configs);
void ImGuiPlaylistAdvance(int direction, AppConfigs* configs);
```

### State Management

Playlist state is a file-local static in `imgui_playlist.cpp`, following the same pattern as `imgui_presets.cpp` (which uses static `entries[]`, `loadedPresetPath`, etc.).

`ImGuiPlaylistAdvance()` is called from `main.cpp` keybind handling. It checks bounds internally — if at first entry and direction is -1, or at last entry and direction is +1, it does nothing. On successful advance, it calls `ImGuiLoadPreset()` to load the new preset.

### JSON Format

```json
{
  "name": "Friday Gig",
  "entries": [
    "presets/BINGBANG.json",
    "presets/Ambient/DREAMSTATE.json",
    "presets/ZOOP.json"
  ]
}
```

Paths are relative to app root. `activeIndex` is runtime state, not serialized — resets to -1 on playlist load.

### UI Layout

The window has three vertical zones. All colors use existing `Theme::` constants from `src/ui/theme.h`.

**Zone 1 — Transport Strip:**

A backlit command bar with hardware transport feel.

- Full-width background: `AddRectFilled` with `SetColorAlpha(Theme::ACCENT_CYAN_U32, 21)` (~0x15 alpha)
- Left-edge accent bar: 2px `AddRectFilled` in `Theme::ACCENT_CYAN_U32` (L-bracket motif matching `DrawCategoryHeader`)
- Top rim-light: 1px `AddLine` at strip top using `SetColorAlpha(Theme::ACCENT_CYAN_U32, 60)`
- Height: `GetFrameHeight() + 4.0f`
- `◄` / `►`: `ArrowButton("##prev", ImGuiDir_Left)` / `ArrowButton("##next", ImGuiDir_Right)` — built-in triangle rendering, no font glyph needed. Wrapped in `BeginDisabled`/`EndDisabled` at boundaries
- Preset name: `TextColored(Theme::ACCENT_CYAN, name)`, centered via `CalcTextSize` + `SetCursorPosX`
- Position counter: `TextDisabled("%d / %d", activeIndex+1, entryCount)` right of name
- Empty playlist: `TextDisabled("(empty)")` + `TextDisabled("0 / 0")`, both buttons disabled
- No preset active: `TextDisabled("No preset loaded")` + `TextDisabled("- / %d", entryCount)`

**Zone 2 — Setlist:**

Scrollable `BeginChild` region. Each row is a `Selectable`.

Active row:
- Background: `AddRectFilled` full width at `SetColorAlpha(Theme::ACCENT_CYAN_U32, 32)` (0x20 alpha)
- Play marker: small right-pointing triangle via `AddTriangleFilled()` on draw list in `Theme::ACCENT_CYAN_U32`, between index and name. Size: ~6px side length, vertically centered in row. No font glyph needed
- Name text: `TextColored(Theme::ACCENT_CYAN, name)`

Normal row:
- Index: `TextDisabled("%2d", index+1)` fixed-width left column
- Name: default `ImGui::Text` (uses `Theme::TEXT_PRIMARY`)
- No play marker — blank space maintains alignment

Remove button:
- `SmallButton("x")` right-aligned via `SameLine(contentWidth - buttonWidth)`
- Only rendered when row is hovered (`IsItemHovered()` on the `Selectable`)

Drag-drop reorder:
- Source: `BeginDragDropSource` on `Selectable`, payload type `"PLAYLIST_ITEM"`, payload = `int` index
- Drag tooltip: `Text("%s", entryName)`
- Target: `BeginDragDropTarget` on each row
- Drop indicator: `AddLine` across full width using `Theme::ACCENT_CYAN_U32`, 2px thickness

Double-click:
- `ImGuiSelectableFlags_AllowDoubleClick` on each `Selectable`
- On double-click: load preset via `ImGuiLoadPreset()`, update `activeIndex`

Empty state:
- Centered `TextDisabled("Drag presets here or click + Add Current")`

**Zone 3 — Manage Bar:**

- `Button("+ Add Current")` left-aligned; `Button("Save")` and `Button("Load")` right-aligned via `SameLine(width - saveLoadWidth)`
- `+ Add Current`: wrapped in `BeginDisabled`/`EndDisabled` when `ImGuiGetLoadedPresetPath()[0] == '\0'`

Inline save flow (replaces button row when `saving == true`):
- `InputText("##playlistSave", nameBuf, PRESET_NAME_MAX)` ~70% width
- `SameLine` + `SmallButton("OK")` triggers `PlaylistSave`
- `SameLine` + `SmallButton("x")` cancels
- Auto-focus: `SetKeyboardFocusHere(0)` on first frame
- Enter submits via `ImGuiInputTextFlags_EnterReturnsTrue`

Inline load flow (replaces button row when `loading == true`, mutually exclusive with saving):
- `BeginCombo("Playlist", currentName)` listing `PlaylistListFiles` results
- Each `Selectable` triggers `PlaylistLoad` + resets `activeIndex` to -1
- `SameLine` + `SmallButton("x")` cancels

### Keybind Integration

In `main.cpp`, alongside the existing TAB handler, before the `if (ctx->uiVisible)` block:

```
if (!io.WantCaptureKeyboard) {
    if (IsKeyPressed(KEY_LEFT))  ImGuiPlaylistAdvance(-1, &configs);
    if (IsKeyPressed(KEY_RIGHT)) ImGuiPlaylistAdvance(+1, &configs);
}
```

This requires moving `AppConfigs configs = {...}` construction above the `if (ctx->uiVisible)` block so keybinds work even when UI is hidden.

### Parameters

N/A — no modulatable parameters.

### Constants

- `MAX_PLAYLIST_ENTRIES`: 64
- `PLAYLIST_DIR`: `"playlists"`
- Drag-drop payload type: `"PLAYLIST_ITEM"`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Playlist header

**Files**: `src/config/playlist.h` (create)
**Creates**: `Playlist` struct, all public function declarations

**Do**: Create the header with:
- Include guard `PLAYLIST_H`
- `#include "preset.h"` (for `PRESET_NAME_MAX`, `PRESET_PATH_MAX`)
- `#include <stdbool.h>`
- `MAX_PLAYLIST_ENTRIES` (64) and `PLAYLIST_DIR` ("playlists") defines
- `Playlist` struct as specified in Design section
- Function declarations: `PlaylistDefault`, `PlaylistSave`, `PlaylistLoad`, `PlaylistListFiles`, `PlaylistAdd`, `PlaylistRemove`, `PlaylistSwap`, `PlaylistAdvance`
- One-line comments on each function, matching preset.h style

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Playlist implementation

**Files**: `src/config/playlist.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement all functions declared in `playlist.h`. Follow `preset.cpp` patterns exactly:
- `#include "playlist.h"` + `<cstring>`, `<filesystem>`, `<fstream>`, `<nlohmann/json.hpp>`
- `namespace fs = std::filesystem;`
- `PlaylistDefault()`: zero-init, `activeIndex = -1`, empty name
- `PlaylistSave()`: serialize to JSON with `j["name"]` and `j["entries"]` array (only first `entryCount` entries). Use `j.dump(2)` + `std::ofstream`. Create parent directory if needed via `fs::create_directories`. Return false on failure (try/catch).
- `PlaylistLoad()`: `std::ifstream` + `json::parse`. Populate `name` via `strncpy`. Iterate `entries` array, `strncpy` each path. Set `entryCount` from array size (clamped to `MAX_PLAYLIST_ENTRIES`). Set `activeIndex = -1`. Return false on failure.
- `PlaylistListFiles()`: same pattern as `PresetListEntries` but simpler — only `.json` files, no folders, no sorting needed. Use `fs::directory_iterator` on `PLAYLIST_DIR`. `fs::create_directories` if not exists.
- `PlaylistAdd()`: if `entryCount >= MAX_PLAYLIST_ENTRIES` return false. `strncpy` path into `entries[entryCount++]`. Return true.
- `PlaylistRemove()`: shift entries down from `index+1` to `entryCount-1`. Decrement `entryCount`. Adjust `activeIndex`: if removed index < activeIndex, decrement it; if removed index == activeIndex, set to -1.
- `PlaylistSwap()`: bounds check, then swap `entries[a]` and `entries[b]` via temp buffer. Adjust `activeIndex` if it equals a or b.
- `PlaylistAdvance()`: if `entryCount == 0` return false. Compute `newIndex = activeIndex + direction`. If `newIndex < 0 || newIndex >= entryCount` return false. Set `activeIndex = newIndex`. Return true.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Expose preset accessors

**Files**: `src/ui/imgui_panels.h` (modify), `src/ui/imgui_presets.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:

In `imgui_panels.h`, add these declarations alongside the existing `ImGuiDrawPresetPanel`:
```c
const char* ImGuiGetLoadedPresetPath(void);
void ImGuiLoadPreset(const char* filepath, AppConfigs* configs);
void ImGuiDrawPlaylistPanel(AppConfigs* configs);
void ImGuiPlaylistAdvance(int direction, AppConfigs* configs);
```

In `imgui_presets.cpp`:
- Add a new public function `ImGuiGetLoadedPresetPath()` that returns the file-local `loadedPresetPath` static.
- Rename the existing `static void LoadPreset(...)` to `void ImGuiLoadPreset(...)` (remove `static`, add `ImGui` prefix). Update the one call site inside `DrawPresetList` to use the new name.

**Verify**: `cmake.exe --build build` compiles. Existing preset panel still works.

---

#### Task 2.3: Playlist UI panel

**Files**: `src/ui/imgui_playlist.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the playlist panel following `imgui_presets.cpp` patterns. This is the largest task — implement all three UI zones as described in the Design / UI Layout section.

Includes:
- `"config/app_configs.h"`, `"config/playlist.h"`, `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/theme.h"`, `<stdio.h>`, `<string.h>`, `<filesystem>`

File-local statics:
- `static Playlist playlist;` — the runtime playlist state
- `static bool initialized = false;`
- `static bool saving = false;`, `static bool loading = false;`, `static bool focusSaveInput = false;`
- `static char saveNameBuf[PRESET_NAME_MAX] = "";`
- `static char playlistFiles[MAX_PLAYLIST_ENTRIES][PRESET_PATH_MAX];`
- `static int playlistFileCount = 0;`

Static helper functions:
- `static void RefreshPlaylistFiles(void)` — calls `PlaylistListFiles`
- `static const char* EntryDisplayName(const char* path)` — extracts filename without extension from a preset path (find last `/`, strip `.json`)
- `static void DrawTransportStrip(void)` — Zone 1 per design specs
- `static void DrawSetlist(AppConfigs* configs)` — Zone 2 per design specs
- `static void DrawManageBar(AppConfigs* configs)` — Zone 3 per design specs

Public functions:
- `void ImGuiDrawPlaylistPanel(AppConfigs* configs)` — `ImGui::Begin("Playlist")`, call three draw helpers, `ImGui::End()`. Init on first call.
- `void ImGuiPlaylistAdvance(int direction, AppConfigs* configs)` — call `PlaylistAdvance(&playlist, direction)`, if true call `ImGuiLoadPreset(playlist.entries[playlist.activeIndex], configs)`.

Implement all UI interactions described in the Design section: transport buttons, setlist selectables with drag-drop reorder, double-click to load, hover-reveal remove button, active row highlighting, inline save/load flows, empty states.

When `ImGuiLoadPreset` fails (missing/moved preset file), log a `TraceLog(LOG_WARNING, ...)` and leave the entry in the playlist for the user to fix or remove. Do not crash or remove the entry automatically.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Main loop integration

**Files**: `src/main.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:

1. Move the `AppConfigs configs = {...}` construction from inside `if (ctx->uiVisible)` to just before it, so keybinds work even when UI is hidden.

2. Add keybind handling after the existing TAB handler, before `if (ctx->uiVisible)`:
```c
if (!io.WantCaptureKeyboard) {
    if (IsKeyPressed(KEY_LEFT))  ImGuiPlaylistAdvance(-1, &configs);
    if (IsKeyPressed(KEY_RIGHT)) ImGuiPlaylistAdvance(+1, &configs);
}
```

3. Add `ImGuiDrawPlaylistPanel(&configs);` call inside the `if (ctx->uiVisible)` block, after `ImGuiDrawPresetPanel(&configs)`.

No new includes needed — `imgui_panels.h` is already included.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/config/playlist.cpp` and `src/ui/imgui_playlist.cpp` to the source file list. Follow the existing grouping — config sources near other `src/config/*.cpp` files, UI sources near other `src/ui/*.cpp` files.

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `playlists/` directory auto-created on first save
- [ ] Save/Load round-trips: save a playlist, close app, reopen, load it — entries preserved
- [ ] `+ Add Current` appends the loaded preset's relative path
- [ ] Left/Right arrow keys advance through playlist and load presets
- [ ] Left at first entry / Right at last entry does nothing (no wrap)
- [ ] Keybinds work when UI is hidden (TAB toggled off)
- [ ] Drag-drop reorder updates entry order and adjusts activeIndex
- [ ] Double-click setlist row loads that preset
- [ ] Remove button (`x`) removes entry and adjusts activeIndex
- [ ] Transport strip shows correct state for: empty playlist, non-empty with no active, non-empty with active
- [ ] Missing preset file: loading fails silently, entry stays in playlist

---

## Implementation Notes

**Setlist row rendering:** Uses a single invisible `Selectable("##row")` per row with `AllowOverlap` for click-through to the remove button. All visual elements (index, play triangle, name text, row backgrounds) are drawn via `AddText`/`AddTriangleFilled`/`AddRectFilled` at exact pixel positions derived from `GetItemRectMin()`/`GetItemRectMax()`. Row hover uses `IsMouseHoveringRect` on the row bounds rather than `IsItemHovered()` — the latter returns false when the mouse is over the overlapping `SmallButton("x")`, causing it to flicker and become unclickable. `Header`/`HeaderHovered`/`HeaderActive` style colors are pushed to transparent for the entire loop so the Selectable never draws its own backgrounds.

**Preset panel save flow:** Updated to match the playlist's inline save pattern — click "Save" replaces the button row with `InputText` + OK + x, then collapses back. `ImGuiLoadPreset()` cancels the save flow to prevent saving stale state after switching presets.
