# Preset Folders

Subdirectory support for the preset browser. Users organize presets into folders (e.g., `presets/Ambient/`, `presets/Glitchy/Live Sets/`). A breadcrumb bar shows the current path with click-to-navigate-back. Folders and presets are sorted separately with folders listed first.

**Research**: `docs/research/preset_folders.md`

## Design

### Types

**`PresetEntry`** (new struct in `preset.h`):
```c
struct PresetEntry {
    char name[PRESET_PATH_MAX]; // Display name (folder name or filename without .json)
    bool isFolder;              // true = directory, false = .json preset
};
```

**Constant changes** (in `preset.h`):
- `MAX_PRESET_FILES` → `MAX_PRESET_ENTRIES`, raised from 32 to 128
- New: `#define PRESET_DIR "presets"` — single source of truth for base directory

**New function** — `PresetListEntries()` (replaces `PresetListFiles()`):
```c
// List entries (folders + .json presets) in directory.
// Entries sorted: folders first (case-insensitive alpha), then presets (case-insensitive alpha).
// Returns number of entries found.
int PresetListEntries(const char *directory, PresetEntry *entries, int maxEntries);
```

Uses `fs::directory_iterator` (non-recursive — current level only). For each entry: if directory → `isFolder = true`, name = directory name. If `.json` file → `isFolder = false`, name = filename without `.json` extension. Sort folders and presets separately using case-insensitive comparison, then concatenate (folders first).

Remove `PresetListFiles()` — only `imgui_presets.cpp` calls it.

### UI State

Replace existing static state in `imgui_presets.cpp`:

```c
static PresetEntry entries[MAX_PRESET_ENTRIES];
static int entryCount = 0;
static int selectedPreset = -1;           // Index within entries[] (presets only)
static int prevSelectedPreset = -1;
static char presetName[PRESET_NAME_MAX] = "Default";
static bool initialized = false;

// New state
static char currentDir[PRESET_PATH_MAX] = "presets";      // Current browsing directory
static char loadedPresetPath[PRESET_PATH_MAX] = "";        // Full path of loaded preset (for ● indicator)
static bool creatingFolder = false;                        // Inline folder rename active
static char folderNameBuf[PRESET_NAME_MAX] = "";           // New folder name input buffer
```

### Visual Design

The preset browser is a cyberpunk terminal file navigator floating in the cosmic void. Every element reinforces the Neon Eclipse identity — folders are glowing cyan portals, the loaded preset burns green against darkness, breadcrumbs trace a luminous path.

**Color assignments**:
- Breadcrumb buttons: cyan text on transparent background. `PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0))` + `PushStyleColor(ImGuiCol_Text, Theme::ACCENT_CYAN)`. Hover gets `ACCENT_CYAN_HOVER` text.
- Breadcrumb delimiter `❯`: `TextDisabled` — the dim `#666173` creates rhythmic punctuation between glowing segments.
- Current breadcrumb segment (last): `Theme::TEXT_PRIMARY` (white `#EBE6F2`) — stands out because it's NOT cyan. Color hierarchy = clickable:cyan, current:white.
- Folder rows: `Theme::ACCENT_CYAN` text with `▸` prefix. Selectable hover tints with cyan glow.
- Preset rows: default `TEXT_PRIMARY`. The loaded preset's `●` indicator uses bright green `ImVec4(0.2f, 0.9f, 0.3f, 1.0f)` — distinct from both cyan and magenta accents so it reads as "active/live" not "navigable."
- Empty state `"(empty)"`: `Theme::TEXT_DISABLED` — fades into the void.
- Inline folder input: preceded by a cyan `▸` prefix so it looks like the folder already exists and you're naming it. The `InputText` frame uses default styling.

**Spatial composition**:
- The breadcrumb bar is a self-contained navigation strip. `Separator()` below it divides navigation from content.
- The scrollable list fills available vertical space — use `ImVec2(-1, -ImGui::GetFrameHeightWithSpacing() * 3)` to reserve room for name input + button row below. This is a key improvement over the fixed 120px height: the browser breathes with the panel size.
- Bottom controls (name input + button row) are compact and anchored to the bottom.
- The "Save Preset" / "Load Preset" section headers from the old layout are removed — the panel title "Presets" is sufficient. The breadcrumb IS the navigation context. Save controls live at the bottom, browse controls fill the rest.

### UI Layout (top to bottom)

**1. Breadcrumb bar**:
- Split `currentDir` by `'/'` into segments (e.g., `["presets", "Ambient", "Chill"]`)
- If more than 1 segment: show `◄` back button (`SmallButton`, transparent bg, cyan text). Click → pop last segment, refresh.
- `SameLine()` between each element
- For segments `[0..n-2]`: clickable `SmallButton(segment)` (transparent bg, cyan text). Click → navigate to path up to that segment, refresh.
- `TextDisabled("❯")` delimiter between segments
- Last segment: `TextColored(TEXT_PRIMARY, segment)` (current location, not clickable — white stands out from cyan clickables)
- `Separator()` below the breadcrumb strip

Breadcrumb pseudocode:
```
segments = split(currentDir, '/')
// Style: transparent buttons with cyan text
PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0))
PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0.9, 0.95, 0.15))
PushStyleColor(ImGuiCol_Text, ACCENT_CYAN)
if (segments.size() > 1)
    if SmallButton("◄")  → pop last segment, navigate
    SameLine()
for i = 0 to segments.size()-2:
    if SmallButton(segments[i])  → navigate to path up to segment i
    SameLine()
    PopStyleColor(3)  // restore for delimiter
    TextDisabled("❯")
    PushStyleColor(3)  // re-push for next button
    SameLine()
PopStyleColor(3)
TextColored(TEXT_PRIMARY, segments.back())   // current dir — white, not clickable
Separator()
```

**2. Scrollable list** (`BeginChild` region, fills available space):
- Height: `ImVec2(-1, -ImGui::GetFrameHeightWithSpacing() * 3)` — reserves space for name input + button row + spacing.
- **Folder rows**: `Selectable()` with `PushStyleColor(ImGuiCol_Text, Theme::ACCENT_CYAN)` and `▸ ` prefix in display label. Click → navigate into folder (append `"/" + name` to `currentDir`, refresh, `selectedPreset = -1`).
- **Inline folder creation**: If `creatingFolder` is true, render cyan `▸` text via `TextColored`, then `SameLine()`, then `InputText("##newfolder", folderNameBuf, PRESET_NAME_MAX)` auto-focused with `SetKeyboardFocusHere(-1)`. Enter → `fs::create_directory(currentDir + "/" + folderNameBuf)`, refresh, clear flag. Escape (`IsKeyPressed(ImGuiKey_Escape)`) → clear flag.
- **Separator**: `SeparatorText("")` between folders and presets (only when both folders and presets exist).
- **Preset rows**: `Selectable()` with default text. Click → auto-load preset (build full path `currentDir + "/" + name + ".json"`). Currently-loaded preset: prefix `"● "` with `PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.3f, 1.0f))` for the full row, then `PopStyleColor`.
- **Empty directory**: If `entryCount == 0` and not `creatingFolder`, centered dimmed `"(empty)"` text using `TextDisabled`.

**3. Name input**: `InputText("Name", presetName, PRESET_NAME_MAX)` — existing behavior.

**4. Button row**: `[Save]` `[+ Folder]` `[Refresh]` on one line using `SameLine()`.
- Save: build path `currentDir + "/" + presetName + ".json"`, save, store in `loadedPresetPath`, refresh.
- `+ Folder`: set `creatingFolder = true`, clear `folderNameBuf`.
- Refresh: re-call `PresetListEntries(currentDir, ...)`.

### Interaction Table

| Action | Trigger | Result |
|--------|---------|--------|
| Enter folder | Click folder Selectable | Append folder name to `currentDir`, refresh, `selectedPreset = -1` |
| Load preset | Click preset Selectable | Build full path `currentDir/name.json`, load, store in `loadedPresetPath` |
| Navigate up | Click `◄` button | Trim last segment from `currentDir`, refresh |
| Jump to ancestor | Click breadcrumb segment | Set `currentDir` to path up to that segment, refresh |
| Save | Click Save button | Save to `currentDir/presetName.json`, refresh |
| New folder | Click `+ Folder` | Set `creatingFolder = true`, focus `folderNameBuf` |
| Confirm folder | Press Enter in folder input | `fs::create_directory()`, refresh, clear flag |
| Cancel folder | Press Escape in folder input | Clear `creatingFolder` flag |
| Refresh | Click Refresh button | Re-call `PresetListEntries(currentDir, ...)` |

### Parameters

N/A — no modulatable parameters.

---

## Tasks

### Wave 1: Backend Types

#### Task 1.1: Preset entry types and listing function

**Files**: `src/config/preset.h`, `src/config/preset.cpp`
**Creates**: `PresetEntry` struct, `PresetListEntries()` function, updated constants

**Do**:

In `preset.h`:
- Add `PresetEntry` struct (see Design > Types)
- Rename `MAX_PRESET_FILES` to `MAX_PRESET_ENTRIES`, change value from 32 to 128
- Add `#define PRESET_DIR "presets"`
- Declare `PresetListEntries()` (see Design > Types)
- Remove `PresetListFiles()` declaration

In `preset.cpp`:
- Implement `PresetListEntries()`: use `fs::directory_iterator`, collect folders (`isFolder = true`, name = dir name) and `.json` files (`isFolder = false`, name = stem without extension). Sort each group case-insensitive alphabetically using `std::sort` with a comparator that uses `strcasecmp` (or `_stricmp` on MSVC — use `#ifdef _MSC_VER`). Concatenate sorted folders then sorted presets into output array. Create directory if it doesn't exist (matching existing `PresetListFiles` behavior).
- Remove `PresetListFiles()` implementation

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: UI Rewrite

#### Task 2.1: Preset panel with folder browsing

**Files**: `src/ui/imgui_presets.cpp`
**Depends on**: Wave 1 complete

**Do**:

Rewrite `imgui_presets.cpp` to support folder browsing. Follow the Design section for state variables, layout, visual design, and interactions.

1. **Replace state**: Swap `presetFiles[]` / `presetFileCount` with `PresetEntry entries[]` / `entryCount`. Add `currentDir`, `loadedPresetPath`, `creatingFolder`, `folderNameBuf` (see Design > UI State).

2. **Update `RefreshPresetList()`**: Call `PresetListEntries(currentDir, entries, MAX_PRESET_ENTRIES)` instead of `PresetListFiles()`.

3. **Add breadcrumb bar**: Implement as described in Design > UI Layout > 1, using the color assignments from Design > Visual Design. Use a local `std::vector<std::string>` or manual `'/'`-splitting to extract path segments. Include `<string>` and `<vector>` for this (STL in .cpp is allowed per conventions). Style breadcrumb `SmallButton`s with transparent background + cyan text. Current segment in `TEXT_PRIMARY` (white). `Separator()` below the breadcrumb strip.

4. **Replace `BeginListBox` with `BeginChild`**: Use `BeginChild("##presetList", ImVec2(-1, -ImGui::GetFrameHeightWithSpacing() * 3), true)` — fills available space, reserves room for name input + button row below. This replaces the fixed 120px height.

5. **Folder rows**: Iterate `entries[]` where `isFolder == true`. Format display as `"▸ " + name`. Use `PushStyleColor(ImGuiCol_Text, Theme::ACCENT_CYAN)` for cyan text. On click: append `"/" + name` to `currentDir`, refresh, reset `selectedPreset = -1`.

6. **Inline folder creation**: After folder rows (before separator), if `creatingFolder`: render cyan `▸` via `TextColored(Theme::ACCENT_CYAN, "▸")`, then `SameLine()`, then `InputText("##newfolder", folderNameBuf, PRESET_NAME_MAX)` with `SetKeyboardFocusHere(-1)`. Enter (`InputTextFlags_EnterReturnsTrue`): build path, `fs::create_directory()`, refresh, clear flag. Escape (`IsKeyPressed(ImGuiKey_Escape)`): clear flag.

7. **Separator**: If both folders and presets exist, draw `SeparatorText("")`.

8. **Preset rows**: Iterate `entries[]` where `isFolder == false`. Build full path `currentDir + "/" + name + ".json"`. If path matches `loadedPresetPath`, prefix with `"● "` and push green style `ImVec4(0.2f, 0.9f, 0.3f, 1.0f)` for the full Selectable row. On click: load preset (same auto-load pattern as current code), store path in `loadedPresetPath`.

9. **Empty state**: If `entryCount == 0` and not `creatingFolder`, centered `TextDisabled("(empty)")`.

10. **Remove old section headers**: Drop the "Save Preset" / "Load Preset" `TextColored` headers. The panel title "Presets" is sufficient context; the breadcrumb provides navigation context.

11. **Name input + Button row**: `InputText("Name", ...)` then `[Save]` `SameLine()` `[+ Folder]` `SameLine()` `[Refresh]`. Save builds path using `currentDir`, stores in `loadedPresetPath`. `+ Folder` sets `creatingFolder = true` and clears `folderNameBuf`.

12. **Include `<filesystem>`** for `fs::create_directory`. Add `namespace fs = std::filesystem;` at file scope.

**Verify**: `cmake.exe --build build` compiles. Manual test: create `presets/TestFolder/` directory, verify it appears with `▸` prefix and cyan text, clicking navigates into it, `◄` navigates back.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Root `presets/` directory shows existing presets (backwards compatible)
- [ ] Folders appear with `▸` prefix in cyan, sorted before presets
- [ ] Clicking a folder navigates into it
- [ ] Breadcrumb `◄` button navigates up one level
- [ ] Clicking breadcrumb segments jumps to that level
- [ ] `+ Folder` creates inline input, Enter creates directory, Escape cancels
- [ ] Saving a preset saves to `currentDir`
- [ ] Currently-loaded preset shows green `●` indicator
- [ ] Empty folders show `"(empty)"` text
