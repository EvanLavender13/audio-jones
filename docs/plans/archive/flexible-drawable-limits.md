# Flexible Drawable Limits

Remove per-type drawable constraints. Allow 0-16 drawables of any combination (waveforms, spectrums, shapes).

## Current State

Constraints enforced at multiple layers:

- `src/render/waveform.h:12` - `MAX_WAVEFORMS 8`
- `src/render/shape.h:8` - `MAX_SHAPES 4`
- `src/render/drawable.h:12` - `MAX_DRAWABLES 16` (keep this)
- `src/render/drawable.cpp:160-186` - `DrawableValidate` enforces 1 spectrum max, per-type limits
- `src/ui/imgui_drawables.cpp:42,59,76` - Add buttons disabled by type limits
- `src/ui/imgui_drawables.cpp:94-97` - Delete disabled if only 1 waveform
- `src/config/preset.cpp:253-256` - Load forces `drawableCount >= 1`
- `src/config/preset.cpp:277-288` - `PresetDefault()` creates 1 waveform

Buffers sized for current limits:

- `src/render/drawable.h:17` - `waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED]`
- `src/render/drawable.h:19` - `SpectrumBars* spectrumBars` (single instance)

---

## Phase 1: Expand Buffer Constants

**Goal**: Increase per-type limits to match MAX_DRAWABLES.

**Build**:
- `waveform.h`: Change `MAX_WAVEFORMS` from 8 to 16
- `shape.h`: Change `MAX_SHAPES` from 16 to 16
- `drawable.h`: Expand `waveformExtended` array to `[MAX_DRAWABLES][WAVEFORM_EXTENDED]`

**Done when**: Code compiles with expanded arrays.

---

## Phase 2: Support Multiple Spectrums

**Goal**: Allow up to 16 spectrum drawables with independent smoothing state.

**Build**:
- `drawable.h`: Replace `SpectrumBars* spectrumBars` with `SpectrumBars* spectrumBars[MAX_DRAWABLES]`
- `drawable.cpp` `DrawableStateInit`: Initialize all spectrum pointers to NULL (lazy allocation)
- `drawable.cpp` `DrawableStateUninit`: Free all non-NULL spectrum instances
- `drawable.cpp` `DrawableProcessSpectrum`: Loop through all DRAWABLE_SPECTRUM types, allocate SpectrumBars on first use, process each with its own smoothing
- `drawable.cpp` `DrawableRenderSpectrum`: Pass correct SpectrumBars instance based on spectrum index

**Done when**: Multiple spectrums render with independent smoothing.

---

## Phase 3: Remove UI Constraints

**Goal**: Allow adding/deleting any drawable type freely.

**Build**:
- `imgui_drawables.cpp:42`: Remove `waveformCount >= MAX_WAVEFORMS` from add waveform disable condition
- `imgui_drawables.cpp:59`: Remove `hasSpectrum` from add spectrum disable condition (keep only `*count >= MAX_DRAWABLES`)
- `imgui_drawables.cpp:76`: Remove `shapeCount >= MAX_SHAPES` from add shape disable condition
- `imgui_drawables.cpp:94-97`: Remove minimum waveform check from delete logic; allow delete if `*selected >= 0 && *selected < *count`
- `imgui_drawables.cpp:193-197`: Show Enabled checkbox for waveforms (remove type check, show for all types)

**Done when**: Can add 16 of any single type, delete all drawables.

---

## Phase 4: Update DrawableValidate

**Goal**: Remove per-type validation since UI now enforces only total limit.

**Build**:
- `drawable.cpp` `DrawableValidate`: Remove spectrum count check, remove waveform/shape limit checks; validate only `count <= MAX_DRAWABLES`
- `drawable.h:47-49`: Update comment to reflect new behavior

**Done when**: `DrawableValidate` passes for any combination up to 16 total.

---

## Phase 5: Update Preset Handling

**Goal**: Allow empty drawable list in presets and on startup.

**Build**:
- `preset.cpp:253-256` `from_json`: Remove `if (p.drawableCount < 1) { p.drawableCount = 1; }` check; allow 0
- `preset.cpp:265-267`: Remove fallback that creates default drawable when array missing
- `preset.cpp:277-288` `PresetDefault`: Set `drawableCount = 0`, remove `drawables[0] = Drawable{}`
- `imgui_drawables.cpp:176-217`: Guard settings panel with `if (*selected >= 0 && *selected < *count)` - show nothing when empty

**Done when**: App starts with empty drawable list, presets save/load with 0 drawables.

---

## Phase 6: Cleanup Unused Helpers

**Goal**: Remove helper functions that only existed for constraint checking.

**Build**:
- `imgui_drawables.cpp`: Remove local variables `waveformCount`, `hasSpectrum`, `shapeCount` if no longer used
- `drawable.cpp`/`drawable.h`: Keep `DrawableCountByType` and `DrawableHasType` (still useful for UI labeling)

**Done when**: No dead code related to removed constraints.
