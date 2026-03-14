# Playlist Section in Presets Panel

Merge the standalone Playlist window into the Presets panel as a non-collapsible section. The two docked windows become one panel with a `SeparatorText("Playlist")` divider and zero visual gap between preset controls and the playlist section.

## Design

### Layout (top to bottom within Presets window)

1. Breadcrumbs (existing, unchanged)
2. Preset list child (height fills remaining space above controls)
3. Preset controls — 1 row: Save | + Folder | Refresh
4. `SeparatorText("Playlist")` — section divider
5. Transport strip (prev/next arrows, current preset name, counter)
6. Setlist child (fixed height: `PLAYLIST_SETLIST_ROWS` rows)
7. Manage bar — 1 row: + Add Current | Save | Load

### Height Calculation

The preset list child uses negative height to reserve space for everything below it. The setlist uses a fixed row count so the visual size matches what it had as a standalone window.

```
PLAYLIST_SETLIST_ROWS = 8

playlistOverhead = separatorText + transportStrip + manageBar
                 ≈ 3 * GetFrameHeightWithSpacing()
setlistH         = PLAYLIST_SETLIST_ROWS * GetFrameHeightWithSpacing()
controlsH        = GetFrameHeightWithSpacing()

presetListH = -(controlsH + playlistOverhead + setlistH)
```

The constant `PLAYLIST_SETLIST_ROWS` is defined in `imgui_panels.h` so both `imgui_presets.cpp` (for reservation) and `imgui_playlist.cpp` (for the child size) use the same value.

### Function Signature Change

```
// Before
void ImGuiDrawPlaylistPanel(AppConfigs *configs);

// After
void ImGuiDrawPlaylistSection(AppConfigs *configs);
```

The section function draws content directly (no `ImGui::Begin/End` window). Called from `ImGuiDrawPresetPanel` after `DrawPresetControls`.

---

## Tasks

### Wave 1: Header Update

#### Task 1.1: Update imgui_panels.h

**Files**: `src/ui/imgui_panels.h`
**Creates**: Updated declaration that Wave 2 tasks depend on

**Do**:
- Replace `void ImGuiDrawPlaylistPanel(AppConfigs *configs);` with `void ImGuiDrawPlaylistSection(AppConfigs *configs);`
- Add `#define PLAYLIST_SETLIST_ROWS 8` near the top of the file (after includes, before struct forward declarations)

**Verify**: `cmake.exe --build build` compiles (will fail on unresolved references — expected until Wave 2).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Convert playlist from window to section

**Files**: `src/ui/imgui_playlist.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Rename `ImGuiDrawPlaylistPanel` → `ImGuiDrawPlaylistSection`
- Remove `ImGui::Begin("Playlist")` / `ImGui::End()` — the function now draws section content only
- Add `ImGui::SeparatorText("Playlist");` as the first draw call (after the initialization check)
- In `DrawSetlist`: change child height from `ImVec2(-1, -ImGui::GetFrameHeightWithSpacing())` to `ImVec2(-1, PLAYLIST_SETLIST_ROWS * ImGui::GetFrameHeightWithSpacing())`
- Everything else (transport strip, setlist, manage bar, static state, `ImGuiPlaylistAdvance`) stays unchanged

**Verify**: Compiles.

---

#### Task 2.2: Embed playlist section in preset panel

**Files**: `src/ui/imgui_presets.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `DrawPresetList`: replace the height calculation `ImVec2(-1, -ImGui::GetFrameHeightWithSpacing() * 3)` with a calculated reservation that accounts for the playlist section below:
  ```cpp
  float controlsH = ImGui::GetFrameHeightWithSpacing();
  float playlistH = (PLAYLIST_SETLIST_ROWS + 3) * ImGui::GetFrameHeightWithSpacing();
  float reserveBelow = controlsH + playlistH;
  ```
  Then use `ImVec2(-1, -reserveBelow)` for the child size.
- In `ImGuiDrawPresetPanel`: add `ImGuiDrawPlaylistSection(configs);` after the `DrawPresetControls(configs);` call (before `ImGui::End()`)

**Verify**: Compiles.

---

#### Task 2.3: Remove standalone playlist window call

**Files**: `src/main.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Remove the line `ImGuiDrawPlaylistPanel(&configs);` (line ~306)
- The keybinds for `ImGuiPlaylistAdvance` (LEFT/RIGHT arrow keys) stay — they are unaffected

**Verify**: Compiles. Full build succeeds with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Presets panel shows preset list, controls, then playlist section with no gap
- [ ] Preset list height adjusts correctly — no empty space, no overflow
- [ ] Setlist displays entries, drag-drop reorder works
- [ ] Transport strip prev/next and LEFT/RIGHT keybinds work
- [ ] + Add Current, Save, Load playlist flows work
- [ ] Preset save, folder creation, refresh flows work
- [ ] No standalone Playlist window appears
