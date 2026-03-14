# Preset Playlist

An ordered setlist of presets that the user navigates with Left/Right arrow keybinds. A dedicated ImGui window shows a transport strip (now-playing, position, prev/next buttons), a reorderable setlist, and save/load controls. Playlists are persisted as lightweight JSON files referencing preset paths.

## Classification

- **Category**: General (UI + filesystem + input)
- **Pipeline Position**: N/A — no render pipeline involvement

## Algorithm

No external algorithm. The feature combines existing codebase patterns: nlohmann/json serialization (from `preset.cpp`), `std::filesystem` directory listing (from `PresetListFiles`), ImGui drag-drop reorder, and raylib `IsKeyPressed()` input (from `main.cpp` TAB handler).

### New Files

**`src/config/playlist.h`** — Data struct and I/O API:

```
#define MAX_PLAYLIST_ENTRIES 64
#define PLAYLIST_DIR "playlists"

struct Playlist {
    char name[PRESET_NAME_MAX];
    char entries[MAX_PLAYLIST_ENTRIES][PRESET_PATH_MAX];  // Relative preset paths
    int entryCount;
    int activeIndex;  // Currently-playing index (-1 = none)
};
```

Public functions:
- `Playlist PlaylistDefault(void)` — zero-initialized, `activeIndex = -1`
- `bool PlaylistSave(const Playlist* pl, const char* filepath)` — write JSON
- `bool PlaylistLoad(Playlist* pl, const char* filepath)` — read JSON
- `int PlaylistListFiles(const char* directory, char outFiles[][PRESET_PATH_MAX], int maxFiles)` — list `.json` files in `playlists/`
- `bool PlaylistAdd(Playlist* pl, const char* presetPath)` — append entry, return false if full
- `void PlaylistRemove(Playlist* pl, int index)` — remove and shift entries down
- `void PlaylistSwap(Playlist* pl, int a, int b)` — swap two entries (for drag reorder)
- `bool PlaylistAdvance(Playlist* pl, int direction)` — move activeIndex by +1 or -1, clamped to bounds. Returns false if at boundary.

**`src/config/playlist.cpp`** — Implementation. JSON format:

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

Paths are relative to app root. Compatible with preset folder subdirectories.

**`src/ui/imgui_playlist.cpp`** — ImGui window with three zones:

Public function:
- `void ImGuiDrawPlaylistPanel(AppConfigs* configs)` — called from main loop alongside other panels

### UI Layout

The window has three vertical zones:

**Zone 1 — Transport Strip:**
A horizontal bar with backlit cyan tint (`AddRectFilled` at ~`0x15` alpha cyan behind the row).

```
◄   │    DREAMSTATE    │  3 / 7  │   ►
```

- `◄` / `►`: `SmallButton`, dimmed at boundaries (first/last item)
- Preset name: `TextColored` cyan, centered
- Position: `TextDisabled` showing `"activeIndex+1 / entryCount"`
- Empty playlist: shows `"(empty)"` with `"0 / 0"`, both buttons disabled
- No preset active: shows `"No preset loaded"` with `"- / N"`

**Zone 2 — Setlist:**
Scrollable `BeginChild` region. Each row is a `Selectable`:

```
 1   BINGBANG                          ×
 2   WOBBYBOB                          ×
 3 ▶ DREAMSTATE                        ×    ← active row (cyan highlight)
 4   ZOOP                              ×
```

- Index: `TextDisabled` left column, fixed width
- `▶` marker: cyan `TextColored`, only on active row
- Name: default text, active row in cyan
- `×` remove: `SmallButton` right-aligned, visible on hover
- Active row: `AddRectFilled` with `ACCENT_CYAN` at `0x20` alpha
- Double-click: jumps to that preset (loads it, updates `activeIndex`)
- Drag-drop reorder: `BeginDragDropSource`/`BeginDragDropTarget` with `"PLAYLIST_ITEM"` payload, cyan separator line at drop position
- Empty: centered `TextDisabled` hint text

**Zone 3 — Manage Bar:**
Three buttons on one line:

```
[+ Add Current]          [Save]    [Load]
```

- `+ Add Current`: appends loaded preset's path to playlist. Disabled (greyed) if no preset loaded.
- `Save`: toggles inline save flow — `InputText` + OK/Cancel replaces button row
- `Load`: toggles inline load flow — `BeginCombo` listing `playlists/*.json` files

Inline save flow:
```
┌────────────────────────────┐
│ My Live Set█               │  [OK] [×]
└────────────────────────────┘
```

Inline load flow:
```
Playlist:  [ My Live Set      ▼]  [×]
```

### Keybind Handling

In `main.cpp`, alongside the existing TAB handler:

```
if (!io.WantCaptureKeyboard && playlist.entryCount > 0) {
    if (IsKeyPressed(KEY_LEFT))  → PlaylistAdvance(&playlist, -1), load preset
    if (IsKeyPressed(KEY_RIGHT)) → PlaylistAdvance(&playlist, +1), load preset
}
```

Boundary behavior: at first item Left does nothing, at last item Right does nothing. No wrapping.

### Integration Points

- `main.cpp`: add `Playlist` to app state, add `ImGuiDrawPlaylistPanel(&configs)` call, add keybind handling
- Preset loading reuses existing `PresetLoad()` / `PresetToAppConfigs()` / `PostEffectClearFeedback()` flow
- `+ Add Current` needs to know the currently-loaded preset's file path (tracked in the preset browser via `loadedPresetPath` from the preset folders feature, or a new static in `imgui_presets.cpp`)

### Interaction with Preset Folders Feature

Playlist entries store relative paths like `"presets/Ambient/DREAMSTATE.json"`. This naturally supports the folder hierarchy from the preset folders feature. No special handling needed — paths are opaque strings resolved by `PresetLoad()`.

## Parameters

N/A — no modulatable parameters.

## Modulation Candidates

None currently. Future directions noted:
- **Time-based auto-advance**: a timer that calls `PlaylistAdvance(+1)` on interval
- **Modulation-driven advance**: bind advance trigger to a mod source threshold

These are out of scope for initial implementation.

## Notes

- `MAX_PLAYLIST_ENTRIES` set to 64 — generous for live setlists
- `activeIndex` is runtime state, not serialized. On playlist load, `activeIndex` resets to -1 (no preset auto-loaded)
- Playlist file is only written on explicit Save, not on every modification
- If a referenced preset file is missing/moved, loading that entry fails silently (skip with TraceLog warning). The entry remains in the playlist for the user to fix or remove.
- The `loadedPresetPath` dependency means this feature pairs with preset folders — if implementing standalone, a simpler "last loaded filename" tracker is sufficient
