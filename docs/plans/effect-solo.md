# Effect Solo

Adds a solo mode to the transform pipeline so users can isolate one or more effects for tuning without disabling others. When any effect is soloed, only soloed effects execute in the output chain. All other effects remain enabled in config but are skipped during rendering. Solo state is transient — not serialized with presets.

## Design

### State

Global array in `imgui_effects_dispatch.cpp` (alongside existing `g_effectSectionOpen`):

```
bool g_effectSolo[TRANSFORM_EFFECT_COUNT] = {};
```

Declared `extern` in `imgui_effects_dispatch.h`.

Helper function in the same file:

```cpp
bool IsAnySoloActive() {
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (g_effectSolo[i]) return true;
  }
  return false;
}
```

Declared `extern` in `imgui_effects_dispatch.h`.

### Render Pipeline Change

In `RenderPipelineApplyOutput` (`render_pipeline.cpp`), the existing loop at line 255 checks `*entry.enabled`. Add a solo gate: if any solo is active, skip effects whose solo flag is false.

```cpp
// Before the loop:
const bool soloActive = IsAnySoloActive();

// Inside the loop, replace the enabled check:
if (entry.enabled != NULL && *entry.enabled) {
  if (soloActive && !g_effectSolo[effectType]) {
    continue;
  }
  // ... existing render pass logic
}
```

### UI: Pipeline List Solo Button

In the pipeline list in `imgui_effects.cpp` (line 154-256), add an "S" toggle button per row between the drag handle and the effect name. When soloed, the "S" button renders with `ACCENT_ORANGE` color. When no solo is active, buttons render dimmed (`TEXT_DISABLED`).

Button is a small `ImGui::SmallButton` styled with push/pop colors. Clicking toggles `g_effectSolo[type]`.

Also add a "Clear Solo" button above the list when any solo is active, using `ACCENT_ORANGE` styling.

### Parameters

None — no config fields, no modulation, no serialization.

---

## Tasks

### Wave 1: State and render gate

#### Task 1.1: Add solo state and render gate

**Files**: `src/ui/imgui_effects_dispatch.h`, `src/ui/imgui_effects_dispatch.cpp`, `src/render/render_pipeline.cpp`

**Do**:

1. In `imgui_effects_dispatch.h`: add `extern bool g_effectSolo[]` and `bool IsAnySoloActive()` declarations.

2. In `imgui_effects_dispatch.cpp`: add `bool g_effectSolo[TRANSFORM_EFFECT_COUNT] = {};` definition and `bool IsAnySoloActive()` implementation (loop over array, return true if any is set).

3. In `render_pipeline.cpp`:
   - Add `#include "ui/imgui_effects_dispatch.h"` (for `g_effectSolo` and `IsAnySoloActive`)
   - In `RenderPipelineApplyOutput`, before the `for` loop: `const bool soloActive = IsAnySoloActive();`
   - Inside the loop, after the `if (entry.enabled != NULL && *entry.enabled)` check passes, add: `if (soloActive && !g_effectSolo[effectType]) continue;`

**Verify**: `cmake.exe --build build` compiles. Solo array exists but is never set yet, so behavior is unchanged.

---

### Wave 2: Pipeline list UI

#### Task 2.1: Add solo buttons to pipeline list

**Files**: `src/ui/imgui_effects.cpp`

**Depends on**: Wave 1 complete

**Do**:

In `ImGuiDrawEffectsPanel`, in the pipeline list (the `BeginListBox("##PipelineList"...)` block):

1. Add `#include "ui/imgui_effects_dispatch.h"` (already has the extern declarations from Wave 1).

2. Before the `BeginListBox`, check `IsAnySoloActive()`. If true, draw a "Clear Solo" button:
   ```cpp
   if (IsAnySoloActive()) {
     ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 115, 13, 40)));
     ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT_ORANGE);
     if (ImGui::SmallButton("Clear Solo")) {
       memset(g_effectSolo, 0, sizeof(g_effectSolo));
     }
     ImGui::PopStyleColor(2);
   }
   ```

3. Inside the list, after the drag handle `"::"` text and `SameLine()`, before the effect name `Text`, add a solo toggle button:
   ```cpp
   // Solo button
   bool isSoloed = g_effectSolo[type];
   ImU32 soloColor = isSoloed ? Theme::ACCENT_ORANGE_U32 : Theme::TEXT_DISABLED_U32;
   ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(soloColor));
   ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
   ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.1f, 0.0f, 0.3f));
   ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.15f, 0.0f, 0.5f));
   char soloId[32];
   snprintf(soloId, sizeof(soloId), "S##solo%d", type);
   if (ImGui::SmallButton(soloId)) {
     g_effectSolo[type] = !g_effectSolo[type];
   }
   ImGui::PopStyleColor(4);
   ImGui::SameLine();
   ```

4. Adjust the badge right-alignment offset if needed to account for the new button width.

**Verify**: `cmake.exe --build build` compiles. Run app, enable 2+ effects, click "S" on one — only that effect renders. Click "S" on another — both render. Click "Clear Solo" — all enabled effects render again.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Solo button appears per row in pipeline list
- [ ] Clicking "S" toggles orange highlight
- [ ] When any effect is soloed, only soloed effects render
- [ ] "Clear Solo" button appears when solo is active and clears all solos
- [ ] Solo state is NOT saved in presets (transient global array)
- [ ] Preset load does not crash (solo array is independent of config)
