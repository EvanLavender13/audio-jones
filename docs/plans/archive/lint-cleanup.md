# Lint Cleanup

Fix all straightforward clang-tidy and cppcheck warnings from the 2026-03-17 lint run. Dead code removal, const-correctness, missing braces, implicit conversions, and an unused parameter.

## Design

### Scope

Six categories of fixes across 7 source files. No behavioral changes — all fixes are mechanical code quality improvements.

| # | Category | Files | Count |
|---|----------|-------|-------|
| 1 | Dead variable | `curl_advection.cpp` | 1 |
| 2 | Dead functions | `profiler.cpp/h`, `trail_map.cpp/h` | 3 fns |
| 3 | Missing braces | `imgui_playlist.cpp`, `imgui_presets.cpp` | ~15 |
| 4 | Const correctness | `imgui_playlist.cpp`, `imgui_presets.cpp` | ~40 |
| 5 | Unused parameter | `imgui_playlist.cpp` | 1 |
| 6 | Implicit bool conversion | `imgui_playlist.cpp`, `imgui_presets.cpp` | 4 |
| 7 | Unchecked returns (cert-err33-c) | `imgui_playlist.cpp`, `imgui_presets.cpp` | ~10 |

Already suppressed (no action): `bugprone-integer-division` in `analysis_pipeline.cpp:89`, `readability-isolate-declaration` in `imgui_widgets.cpp:201`.

---

## Tasks

### Wave 1: Dead code removal (no file overlap between tasks)

#### Task 1.1: Remove unused `float h` in curl_advection.cpp

**Files**: `src/effects/curl_advection.cpp`

**Do**: Line 183 declares `float h;` in the `COLOR_MODE_PALETTE` branch. It is never passed to any function or read. Delete it.

**Verify**: Compiles.

#### Task 1.2: Remove `ProfilerUninit` dead function

**Files**: `src/render/profiler.cpp`, `src/render/profiler.h`

**Do**: Remove `ProfilerUninit` definition (profiler.cpp:31-36) and declaration (profiler.h:41). No callers exist anywhere in the codebase.

**Verify**: Compiles.

#### Task 1.3: Remove `TrailMapBeginDraw` and `TrailMapEndDraw` dead functions

**Files**: `src/simulation/trail_map.cpp`, `src/simulation/trail_map.h`

**Do**: Remove `TrailMapBeginDraw` definition (trail_map.cpp:215-221), `TrailMapEndDraw` definition (trail_map.cpp:223-228), and both declarations + comments (trail_map.h:39-43). No callers exist.

**Verify**: Compiles.

---

### Wave 2: Playlist and presets cleanup (two independent files)

#### Task 2.1: Fix all warnings in `imgui_playlist.cpp`

**Files**: `src/ui/imgui_playlist.cpp`

**Do**:

1. **Missing braces** (~10 instances): Add `{}` around single-statement `if` bodies at lines 72-77 (prev button disabled guards), 84-89 (next button disabled guards), 301, 340-341, 345-346, 390-391.

2. **Const correctness** (~25 instances): Add `const` to all variables flagged by clang-tidy that are assigned once and never modified. Every `float`, `bool`, `ImVec2`, `ImU32`, and `int` local that is not reassigned gets `const`.

3. **Implicit bool conversions** (3 instances):
   - Line 30: `slash ? slash + 1 : path` → `(slash != nullptr) ? slash + 1 : path`
   - Line 36: `if (dot && strcmp(...))` → `if (dot != nullptr && strcmp(...) == 0)`
   - Line 250: `if (payload)` → `if (payload != nullptr)`

4. **Unused parameter**: Line 279 `DrawManageBar(AppConfigs *configs)` — `configs` is never used in the function body. Remove the parameter and update the call site at line 386: `DrawManageBar(configs)` → `DrawManageBar()`.

5. **Implicit bool→ImGuiChildFlags**: Line 148 `ImGui::BeginChild("##setlist", ..., true)` → replace `true` with `ImGuiChildFlags_Borders`.

6. **cert-err33-c**: Add `(void)` cast to unchecked `snprintf` return values at lines 115, 131, 208, 299, 322.

**Verify**: Compiles.

#### Task 2.2: Fix all warnings in `imgui_presets.cpp`

**Files**: `src/ui/imgui_presets.cpp`

**Do**:

1. **Missing braces** (~5 instances): Add `{}` around single-statement `if`/`else` bodies at lines 59, 148, 150, 156, 202.

2. **Const correctness** (~15 instances): Add `const` to all flagged variables: `s`, `pos`, `newPath` (×2), `controlsH`, `playlistH`, `reserveBelow`, `folderPath`, `isLoaded`, `textWidth`, `windowWidth`, `width`, `inputWidth`, `submitted`.

3. **Implicit bool→ImGuiChildFlags**: Line 139 `ImGui::BeginChild("##presetList", ..., true)` → replace `true` with `ImGuiChildFlags_Borders`.

4. **cert-err33-c**: Add `(void)` cast to unchecked `snprintf` return values at lines 68, 160, 165, 206, 261.

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` compiles with no new warnings
- [ ] `./scripts/lint.sh` shows reduced warning count
- [ ] No behavioral changes — all fixes are mechanical
