# Preset Folders

Subdirectory support for the preset browser. Users organize presets into folders (e.g., `presets/Ambient/`, `presets/Glitchy/Live Sets/`). A breadcrumb bar shows the current path and provides click-to-navigate-back. Folders and presets are sorted separately with folders listed first.

## Classification

- **Category**: General (UI + filesystem)
- **Pipeline Position**: N/A — no render pipeline involvement

## Algorithm

No external algorithm. The feature is standard `std::filesystem` directory traversal + Dear ImGui widget composition.

### Backend Changes (`preset.h` / `preset.cpp`)

**New function — `PresetListEntries()`**: Replaces `PresetListFiles()` with a version that returns both folders and `.json` files, distinguished by a type flag. Entries are sorted: folders first (alphabetical, case-insensitive), then presets (alphabetical, case-insensitive). Uses `fs::directory_iterator` (non-recursive — only lists the current level).

**New struct — `PresetEntry`**:
```
struct PresetEntry {
    char name[PRESET_PATH_MAX];   // Display name (folder name or filename without .json)
    bool isFolder;                // true = directory, false = .json preset
};
```

**Constant changes**:
- `MAX_PRESET_FILES` → `MAX_PRESET_ENTRIES` renamed, raised from 32 to 128
- `PRESET_DIR` defined as `"presets"` (single source of truth for base directory)

**Unchanged**: `PresetSave()`, `PresetLoad()`, `PresetFromAppConfigs()`, `PresetToAppConfigs()` — these already take full filepaths, no changes needed.

### UI Changes (`imgui_presets.cpp`)

**State additions**:
```
static char currentDir[PRESET_PATH_MAX] = "presets";     // Current browsing directory
static char loadedPresetPath[PRESET_PATH_MAX] = "";      // Full path of loaded preset (for ● indicator)
static bool creatingFolder = false;                       // Inline folder rename active
static char folderNameBuf[PRESET_NAME_MAX] = "";          // New folder name input buffer
```

**Replaced state**: `presetFiles[]` → `PresetEntry entries[]`, `presetFileCount` → `entryCount`.

**Layout (top to bottom)**:

1. **Breadcrumb bar**: `◄` back button (hidden at root) + clickable `SmallButton()` per path segment + `❯` delimiters. Last segment is plain text (current location). Clicking any segment navigates to that level.

2. **Scrollable list** (`BeginChild` region):
   - Folder rows: `Selectable()` with cyan text and `▸` prefix. Click → navigate into folder (update `currentDir`, refresh list, reset selection).
   - Separator: Thin `SeparatorText("")` between folders and presets (only when both exist).
   - Preset rows: `Selectable()` with default text. Click → auto-load preset (existing behavior). Currently-loaded preset shows green `●` indicator.
   - If `creatingFolder`: inline `InputText` appears in the folder zone, auto-focused. Enter → `fs::create_directory()`, refresh, clear flag. Escape → clear flag.
   - Empty directory: centered dimmed `"(empty)"` text.

3. **Name input**: `InputText` for save name (existing behavior).

4. **Button row**: `[Save]` `[+ Folder]` `[Refresh]` on one line using `SameLine()`.

**Key interactions**:

| Action | Trigger | Result |
|--------|---------|--------|
| Enter folder | Click folder `Selectable` | Append folder name to `currentDir`, refresh, `selectedPreset = -1` |
| Load preset | Click preset `Selectable` | Build full path from `currentDir + "/" + name + ".json"`, load, store in `loadedPresetPath` |
| Navigate up | Click `◄` button | Trim last segment from `currentDir`, refresh |
| Jump to ancestor | Click breadcrumb segment | Set `currentDir` to path up to that segment, refresh |
| Save | Click Save button | Save to `currentDir + "/" + presetName + ".json"`, refresh |
| New folder | Click `+ Folder` | Set `creatingFolder = true`, focus `folderNameBuf` |
| Refresh | Click Refresh button | Re-call `PresetListEntries(currentDir, ...)` |

**Breadcrumb rendering pseudocode**:
```
Split currentDir by '/' into segments (e.g., ["presets", "Ambient", "Chill"])
if (segments.size() > 1)
    if SmallButton("◄")  → pop last segment, navigate
    SameLine()
for i = 0 to segments.size()-2:
    if SmallButton(segments[i])  → navigate to path up to segment i
    SameLine()
    TextDisabled("❯")
    SameLine()
Text(segments.back())   // current dir, not clickable
```

## Parameters

N/A — no modulatable parameters.

## Notes

- Depth is unlimited but UI breadcrumb will wrap via ImGui line flow at deep nesting
- `PresetListFiles()` can be kept as a deprecated shim or removed entirely — only `imgui_presets.cpp` calls it
- Preset `.json` files are not modified — folder organization is purely filesystem-level
- Existing presets in root `presets/` continue to work unchanged (backwards compatible)
- `loadedPresetPath` enables the `●` indicator: compare each preset's full path against it
- The `selectedPreset` / `prevSelectedPreset` auto-load pattern is preserved — clicking a preset in the new Selectable list triggers the same load logic
