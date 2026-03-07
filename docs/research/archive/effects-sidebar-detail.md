# Effects Sidebar + Detail Panel

The Effects window is restructured from a single scrolling panel into a two-pane master-detail layout. A left sidebar tree shows all 5 groups (Feedback, Output, Simulations, Generators, Transforms) with collapsible categories and individual effects. Clicking an effect shows its full parameter panel on the right. The transform pipeline reorder strip sits at the top of the detail panel.

## Classification

- **Category**: General (UI architecture redesign)
- **Scope**: `src/ui/imgui_effects.cpp` + `src/ui/imgui_effects_dispatch.cpp` + supporting widget additions

## References

- [ImGui Splitter (Issue #319)](https://github.com/ocornut/imgui/issues/319) - Resizable split pane pattern using invisible button + mouse delta
- [ImGui TreeNodeEx flags](https://pyimgui.readthedocs.io/en/latest/guide/treenode-flags.html) - Leaf, Selected, OpenOnArrow flags for proper tree behavior
- [ImGui BeginChild layout](https://github.com/ocornut/imgui/issues/3228) - Side-by-side child windows via SameLine()

## Reference Code

### Splitter (from Issue #319)

```cpp
// Resizable divider between two BeginChild panels
void DrawSplitter(int split_vertically, float thickness, float* size0,
                  float* size1, float min_size0, float min_size1)
{
    ImVec2 backup_pos = ImGui::GetCursorPos();
    if (split_vertically)
        ImGui::SetCursorPosY(backup_pos.y + *size0);
    else
        ImGui::SetCursorPosX(backup_pos.x + *size0);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f,0.6f,0.6f,0.10f));
    ImGui::Button("##Splitter", ImVec2(!split_vertically ? thickness : -1.0f,
                                        split_vertically ? thickness : -1.0f));
    ImGui::PopStyleColor(3);
    ImGui::SetItemAllowOverlap();

    if (ImGui::IsItemActive()) {
        float mouse_delta = split_vertically ? ImGui::GetIO().MouseDelta.y
                                             : ImGui::GetIO().MouseDelta.x;
        if (mouse_delta < min_size0 - *size0)
            mouse_delta = min_size0 - *size0;
        if (mouse_delta > *size1 - min_size1)
            mouse_delta = *size1 - min_size1;
        *size0 += mouse_delta;
        *size1 -= mouse_delta;
    }
    ImGui::SetCursorPos(backup_pos);
}
```

### Two-panel layout (standard ImGui pattern)

```cpp
// Sidebar (fixed width, fills height)
ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), true);
// ... tree content ...
ImGui::EndChild();

ImGui::SameLine();

// Detail panel (fills remaining width and height)
ImGui::BeginChild("Detail", ImVec2(0, 0), true);
// ... selected effect params ...
ImGui::EndChild();
```

### Tree node with selection (standard ImGui pattern)

```cpp
ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
                             | ImGuiTreeNodeFlags_SpanAvailWidth;
if (isSelected)
    nodeFlags |= ImGuiTreeNodeFlags_Selected;
if (isLeaf)
    nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

bool nodeOpen = ImGui::TreeNodeEx(label, nodeFlags);
if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    selectedEffect = thisEffect;  // Click selects, arrow toggles
```

## Layout

```
+======================== Effects ========================+
|                                                         |
| +-- SIDEBAR --+  +---------- DETAIL PANEL -----------+ |
| |             |  |                                    | |
| | [Search...] |  |  PIPELINE ORDER                   | |
| |             |  |  [Kaleido] [Bloom] [CRT] [::drag] | |
| | FEEDBACK    |  |  --------------------------------- | |
| |   Blur      |  |                                    | |
| |   Motion    |  |  Kaleidoscope              [SYM]   | |
| |   Half-life |  |  [x] Enabled                       | |
| |   Desat     |  |                                    | |
| |   Flow Fld  |  |  Geometry                          | |
| |             |  |  Segments  [====|=======]          | |
| | OUTPUT      |  |  Zoom      [========|==]           | |
| |   Gamma     |  |  Rotation  [===|======]   deg/s    | |
| |   Clarity   |  |  Reflect   [=====|====]   deg      | |
| |             |  |                                    | |
| | SIMULATIONS |  |  Audio                             | |
| | > Physarum  |  |  Base Freq [==|=======]   Hz       | |
| | > Curl Flow |  |  Max Freq  [========|==]   Hz      | |
| |   ...       |  |  Gain      [====|=====]            | |
| |             |  |  Contrast  [======|===]            | |
| | GENERATORS  |  |                                    | |
| | v Geometric |  |                                    | |
| |   Sig Frm   |  |                                    | |
| |   Arc Strb  |  |                                    | |
| | > Filament  |  |                                    | |
| | > Texture   |  |                                    | |
| | > Atmosph   |  |                                    | |
| |             |  |                                    | |
| | TRANSFORMS  |  |                                    | |
| | v Symmetry  |  |                                    | |
| |  *Kaleido   |  |                                    | |
| |   KIFS      |  |                                    | |
| |   Poincare  |  |                                    | |
| | v Warp      |  |                                    | |
| |   Sine Warp |  |                                    | |
| |  *Tex Warp  |  |                                    | |
| |   ...       |  |                                    | |
| | > Cellular  |  |                                    | |
| | > Motion    |  |                                    | |
| | > Painterly |  |                                    | |
| +-------------+  +-----------------------------------+ |
+---------------------------------------------------------+

LEGEND:
  v / >    = expanded / collapsed category
  *        = enabled effect (accent-colored dot or text)
  [====|=] = modulatable slider
  [SYM]    = category badge
  [::drag] = drag handle in pipeline strip
```

## Algorithm

### Structural Change

Replace the single vertical flow in `imgui_effects.cpp` with:

1. **Outer frame**: `ImGui::BeginChild("Sidebar")` + `ImGui::SameLine()` + `ImGui::BeginChild("Detail")`
2. **Optional resizable splitter**: Invisible button between the two children tracks mouse drag to resize `sidebarWidth` (persisted in static float, default ~220px)

### Sidebar Content

- **Search bar** at top: `ImGui::InputTextWithHint("##search", "Search...", ...)` filters the tree by substring match on effect display name
- **Group headers**: `TreeNodeEx("FEEDBACK", DefaultOpen)` etc. — reuse existing `DrawGroupHeader()` styling or adapt for tree context
- **Category nodes**: `TreeNodeEx("Symmetry", OpenOnArrow | SpanAvailWidth)` — shows `>` / `v` toggle
- **Effect leaves**: `TreeNodeEx("Kaleidoscope", Leaf | NoTreePushOnOpen | SpanAvailWidth)` — click to select, no expand
- **Enabled indicator**: Draw a small colored dot (accent color) left of the label for enabled effects, or tint the text. Use `EffectDescriptor.enabledOffset` to read enabled state from `EffectConfig`
- **Active count badge**: Category nodes show `(3)` suffix or similar to indicate how many effects are enabled within

### Detail Panel Content

**Top: Pipeline strip** (only when transforms are enabled)
- Horizontal row of compact chips showing enabled transforms in pipeline order
- Each chip: effect name + badge color, drag handle for reorder
- Reuses existing drag-drop logic from `imgui_effects.cpp` lines 149-251

**Below: Selected effect params**
- Header: effect display name + category badge (right-aligned)
- Enable checkbox
- Call the effect's `drawParams` callback (same function pointer from `EffectDescriptor`)
- Call `drawOutput` callback if present (generators)
- When effect is disabled: same content but push `ImGui::BeginDisabled()` / `ImGui::EndDisabled()` around the params (ImGui built-in dimming)

### Feedback/Output Special Cases

Feedback and Output groups have no per-effect detail — they are small fixed sets of sliders. Two options:
- **Inline in detail panel**: Selecting "Blur" or "Flow Field" in the sidebar shows those sliders in the detail panel, same as any effect. Flow Field's 20+ params benefit from the detail panel space.
- The sidebar leaf items for Feedback/Output map to hardcoded detail-panel draw functions (not dispatched through `EffectDescriptor`).

### State

- `static float sidebarWidth = 220.0f` — persisted splitter position
- `static int selectedGroup` — which group (0=Feedback..4=Transforms)
- `static int selectedCategory` — which category within group
- `static int selectedEffect` — index or enum of selected effect (for detail panel dispatch)
- `static char searchBuf[64]` — search filter text
- Existing `g_effectSectionOpen[]` array may be repurposed or removed (sidebar tree handles collapse state via ImGui's internal tree state)

### Migration Path

- `imgui_effects.cpp` is the only file that changes layout logic
- `imgui_effects_dispatch.cpp`'s `DrawEffectCategory()` is still used — the detail panel calls it for the single selected effect (or a new single-effect variant)
- All colocated `drawParams` / `drawOutput` callbacks remain untouched
- All `EffectDescriptor` data remains untouched
- Pipeline reorder logic moves from the middle of the scroll to the top of the detail panel

## Notes

- **ImGui docking**: AudioJones already uses ImGui docking branch. The Effects window could alternatively be split into two dockable windows (Sidebar + Detail) that the user can rearrange. Simpler approach: single window with internal BeginChild split.
- **Performance**: Tree traversal iterates `EFFECT_DESCRIPTORS[]` (97 entries) per frame for the sidebar. This is trivially fast for ImGui.
- **Search**: Substring match on `EffectDescriptor.name` is sufficient. No fuzzy matching needed for 130 items.
- **Scroll preservation**: ImGui's `BeginChild` provides independent scroll regions — sidebar and detail panel scroll independently. No more losing position in a giant single scroll.
- **Keyboard navigation**: ImGui tree nodes support arrow key navigation natively. Tab between sidebar and detail panel would require manual focus management.
- **Width persistence**: `sidebarWidth` could be saved to the preset/settings JSON, or just use a session-static default.
