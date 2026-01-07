# Effects Panel Reorganization

Restructure the Effects panel from a flat list of collapsible sections into pipeline-aligned groups (Feedback, Output, Simulations, Transforms) with distinct visual hierarchy.

## Current State

- `src/ui/imgui_effects.cpp:36-400` - `ImGuiDrawEffectsPanel()` renders flat list of 12 collapsible sections
- `src/ui/imgui_widgets.cpp:90-104` - `DrawSectionBegin()`/`DrawSectionEnd()` for effect headers
- `src/ui/imgui_panels.h` - declares UI functions
- `src/ui/theme.h` - color constants for UI styling

**Current layout:**
1. Effect Order (collapsible) - reorderable transform list
2. "Core Effects" label + 6 inline sliders
3. 11 collapsible sections in arbitrary order

## Target Layout

```
FEEDBACK                          ← Group header (bold, non-collapsible)
├─ Blur, Half-life, Desat         ← Inline sliders (no wrapper)
└─ ▶ Flow Field                   ← Collapsible effect section

OUTPUT                            ← Group header
├─ Chromatic, Gamma, Clarity      ← Inline sliders

SIMULATIONS                       ← Group header
├─ ▶ Physarum
├─ ▶ Curl Flow
└─ ▶ Attractor Flow

TRANSFORMS                        ← Group header
├─ [Effect Order list]            ← Reorder UI (moved here)
├─ ▶ Möbius
├─ ▶ Turbulence
├─ ▶ Kaleidoscope
├─ ▶ Infinite Zoom
├─ ▶ Radial Streak
├─ ▶ Multi-Inversion
└─ ▶ Voronoi
```

---

## Phase 1: Add Group Header Widget

**Goal**: Create `DrawGroupHeader()` with distinct styling from effect section headers.

**Build**:
- Add `DrawGroupHeader(const char* label, ImU32 accentColor)` to `imgui_widgets.cpp`
  - Non-collapsible (returns void)
  - Bolder visual treatment than `DrawSectionHeader`: larger text weight, full-width accent bar, or distinct background
  - Uses existing theme accent colors
- Declare in `imgui_panels.h`
- Optionally add `Theme::GROUP_*` constants to `theme.h` if needed

**Done when**: `DrawGroupHeader("TEST", Theme::GLOW_CYAN)` renders visually distinct from effect headers.

---

## Phase 2: Restructure Effects Panel Layout

**Goal**: Reorganize `ImGuiDrawEffectsPanel()` into four pipeline-aligned groups.

**Build**:
- Modify `imgui_effects.cpp`:
  - Remove `sectionEffectOrder` static (Effect Order moves inside Transforms, not its own section)
  - Add group headers in order: FEEDBACK, OUTPUT, SIMULATIONS, TRANSFORMS
  - Under FEEDBACK: inline sliders (blur, half-life, desat), then Flow Field collapsible
  - Under OUTPUT: inline sliders (chromatic, gamma, clarity)
  - Under SIMULATIONS: Physarum, Curl Flow, Attractor Flow collapsibles
  - Under TRANSFORMS: Effect Order list first, then all transform effect collapsibles

**Done when**: Effects panel displays four distinct groups with correct contents, matching target layout.

---

## Phase 3: Polish and Spacing

**Goal**: Ensure consistent visual spacing and alignment across groups.

**Build**:
- Adjust `ImGui::Spacing()` calls between groups vs within groups
- Verify group headers have consistent margins
- Test with various panel widths to ensure layout holds
- Ensure disabled effect dimming still works within groups

**Done when**: Panel looks clean at typical and narrow widths, spacing is consistent.
