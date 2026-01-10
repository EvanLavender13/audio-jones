# Effects Panel Modularization

Split `imgui_effects.cpp` into stable and active sections. Enhance subcategory headers with L-bracket treatment, improve listbox scannability, and polish label/TreeNode styling.

## Current State

- `src/ui/imgui_effects.cpp:1-777` - monolithic effects panel
- `src/ui/imgui_widgets.cpp:75-105` - `DrawCategoryHeader` (too subtle)
- `src/ui/imgui_panels.h:37` - header declaration

**Static state to relocate:**
- `sectionKaleidoscope`, `sectionPoincareDisk` (Symmetry)
- `sectionSineWarp`, `sectionTextureWarp`, `sectionGradientFlow`, `sectionWaveRipple`, `sectionMobius`, `sectionVoronoi` (Warp)
- `sectionInfiniteZoom`, `sectionRadialStreak` (Motion)
- `sectionPixelation`, `sectionGlitch`, `sectionToon`, `sectionHeightfieldRelief` (Style)

---

## Phase 1: File Split

**Goal**: Extract transform subcategories to `imgui_effects_transforms.cpp`.

**Build**:

Create `src/ui/imgui_effects_transforms.h`:
- Include guards and forward declarations
- Declare 4 functions: `DrawSymmetryCategory`, `DrawWarpCategory`, `DrawMotionCategory`, `DrawStyleCategory`

Create `src/ui/imgui_effects_transforms.cpp`:
- Move section state booleans for transform categories (14 bools)
- Implement `DrawSymmetryCategory` containing:
  - `DrawCategoryHeader("Symmetry", Theme::GLOW_CYAN)`
  - Kaleidoscope section (lines 281-382)
  - Poincare Disk section (lines 386-410)
- Implement `DrawWarpCategory` containing:
  - `DrawCategoryHeader("Warp", Theme::GLOW_MAGENTA)`
  - Sine Warp, Texture Warp, Gradient Flow, Wave Ripple, Mobius, Voronoi sections (lines 417-628)
- Implement `DrawMotionCategory` containing:
  - `DrawCategoryHeader("Motion", Theme::GLOW_ORANGE)`
  - Infinite Zoom, Radial Blur sections (lines 635-660)
- Implement `DrawStyleCategory` containing:
  - `DrawCategoryHeader("Style", Theme::GLOW_CYAN)`
  - Pixelation, Glitch, Toon, Heightfield Relief sections (lines 667-773)

Modify `src/ui/imgui_effects.cpp`:
- Add `#include "ui/imgui_effects_transforms.h"`
- Remove relocated section booleans (keep `sectionPhysarum`, `sectionCurlFlow`, `sectionAttractorFlow`, `sectionFlowField`)
- Replace lines 276-773 (Symmetry through Style) with 4 function calls:
  ```
  DrawSymmetryCategory(e, modSources);
  DrawWarpCategory(e, modSources);
  DrawMotionCategory(e, modSources);
  DrawStyleCategory(e, modSources);
  ```

Update `CMakeLists.txt`:
- Add `src/ui/imgui_effects_transforms.cpp` to source list

**Done when**: Build succeeds, Effects panel renders identically.

---

## Phase 2: Category Header Enhancement

**Goal**: Make subcategory headers visually distinct with L-bracket treatment.

**Build**:

Modify `DrawCategoryHeader` in `src/ui/imgui_widgets.cpp:75-105`:

**Current implementation:**
- 3px vertical bar on left edge
- Colored text

**New implementation - L-bracket with horizontal rule:**

```cpp
void DrawCategoryHeader(const char* label, ImU32 accentColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    const float lineHeight = ImGui::GetTextLineHeight();
    const float headerHeight = lineHeight + style.FramePadding.y * 2 + 4.0f; // Extra padding
    const float accentBarWidth = 4.0f;  // Increased from 3px
    const float ruleLength = 0.6f;      // 60% of width

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;

    // Background tint (accent color at ~8% alpha)
    draw->AddRectFilled(
        pos,
        ImVec2(pos.x + width, pos.y + headerHeight),
        SetColorAlpha(accentColor, 20)
    );

    // Vertical accent bar (L-bracket vertical part)
    draw->AddRectFilled(
        pos,
        ImVec2(pos.x + accentBarWidth, pos.y + headerHeight),
        accentColor
    );

    // Horizontal rule (L-bracket horizontal part)
    const float ruleY = pos.y + headerHeight - 1.5f;
    draw->AddLine(
        ImVec2(pos.x + accentBarWidth, ruleY),
        ImVec2(pos.x + width * ruleLength, ruleY),
        SetColorAlpha(accentColor, 150),
        1.5f
    );

    // Label text in accent color
    const float textX = pos.x + accentBarWidth + style.FramePadding.x + 4.0f;
    const float textY = pos.y + style.FramePadding.y + 2.0f;
    const ImU32 textColor = (accentColor & 0x00FFFFFF) | 0xFF000000;
    draw->AddText(ImVec2(textX, textY), textColor, label);

    // Advance cursor
    ImGui::Dummy(ImVec2(width, headerHeight));
    ImGui::Spacing();
}
```

**Visual result:**
```
┌────────────────────────────────────
│▌████████████████████░░░░░░░░░░░░░░  ← 8% alpha background tint
│▌ Symmetry ──────────                ← L-bracket: 4px bar + 60% rule
│▌____________________________________
```

**Done when**: Subcategory headers display L-bracket treatment with visible hierarchy distinction from group headers and section headers.

---

## Phase 3: UI Polish

**Goal**: Apply remaining UI/UX improvements for consistency and scannability.

**Build**:

### 3a. "Techniques" and "Effects" Labels

In Kaleidoscope section (`imgui_effects_transforms.cpp`), the `ImGui::Text("Techniques")` at line 296 is unstyled.

Change:
```cpp
ImGui::Text("Techniques");
```
To:
```cpp
ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Techniques");
```

Apply same treatment to `ImGui::Text("Effects")` in Voronoi section (line 545).

### 3b. Effect Order Listbox Enhancement

In `imgui_effects.cpp`, the transform order listbox (lines 236-273) lacks visual differentiation.

Add alternating row backgrounds and left-edge enabled indicator:

```cpp
if (ImGui::BeginListBox("##EffectOrderList", ImVec2(-FLT_MIN, 80))) {
    ImDrawList* draw = ImGui::GetWindowDrawList();

    for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
        const TransformEffectType type = e->transformOrder[i];
        const char* name = TransformEffectName(type);

        bool isEnabled = false;
        // ... existing switch to determine isEnabled ...

        // Alternating row background
        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        ImVec2 rowMax = ImVec2(rowMin.x + ImGui::GetContentRegionAvail().x,
                               rowMin.y + ImGui::GetTextLineHeightWithSpacing());
        if (i % 2 == 1) {
            draw->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 8));
        }

        // Left-edge enabled indicator (2px bar)
        if (isEnabled) {
            draw->AddRectFilled(
                rowMin,
                ImVec2(rowMin.x + 2.0f, rowMax.y),
                Theme::ACCENT_CYAN_U32
            );
        }

        if (!isEnabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }

        if (ImGui::Selectable(name, selectedTransformEffect == i)) {
            selectedTransformEffect = i;
        }

        if (!isEnabled) {
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndListBox();
}
```

### 3c. TreeNode Expanded Accent

Create helper function in `imgui_widgets.cpp` for TreeNodes with accent indicator:

```cpp
bool TreeNodeAccented(const char* label, ImU32 accentColor)
{
    const bool open = ImGui::TreeNode(label);
    if (open) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        // Draw subtle accent bar at current indent level
        draw->AddRectFilled(
            ImVec2(pos.x - 6.0f, pos.y),
            ImVec2(pos.x - 4.0f, pos.y + ImGui::GetTextLineHeight()),
            SetColorAlpha(accentColor, 100)
        );
    }
    return open;
}
```

Declare in `imgui_panels.h`:
```cpp
bool TreeNodeAccented(const char* label, ImU32 accentColor);
```

Replace TreeNode calls in effects sections where accent would improve hierarchy:
- Kaleidoscope: "Focal Offset", "Warp"
- Wave Ripple: "Origin"
- Mobius: "Fixed Points", "Point Motion"
- Voronoi: "Iso Settings"
- Glitch: "CRT", "Analog", "Digital", "VHS"
- Toon: "Brush Stroke"

**Done when**: Labels use secondary text color, listbox has alternating rows with enabled indicators, TreeNodes show accent bars when expanded.

---

## Phase 4: Verification

**Goal**: Confirm visual hierarchy and functionality.

**Build**:

Visual hierarchy check:
- Group headers (FEEDBACK, OUTPUT, SIMULATIONS, TRANSFORMS) - full-width glow underline
- Subcategory headers (Symmetry, Warp, Motion, Style) - L-bracket with tint
- Section headers (+ Kaleidoscope, + Sine Warp) - collapsible with gradient background
- TreeNodes when expanded show accent bar

Polish check:
- "Techniques" and "Effects" labels use secondary text color
- Effect order listbox has alternating row tint
- Enabled effects show cyan left-edge indicator in listbox
- TreeNode accent bars appear on expand, disappear on collapse

Functional check:
- All section collapse/expand works
- All sliders and controls respond
- Effect order list reordering works
- Preset save/load preserves section state

**Done when**: All 4 header tiers visually distinct, listbox scannable, panel fully functional.
