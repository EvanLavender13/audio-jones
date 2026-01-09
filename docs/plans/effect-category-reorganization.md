# Effect Category Reorganization

Reorganize transform effects in UI by visual outcome, not implementation history. Users think in terms of what they see, not mathematical taxonomy.

## Proposed Categories

| Category | Effects | Visual Result |
|----------|---------|---------------|
| **Symmetry** | Kaleidoscope | Mirrored/folded patterns |
| **Depth** | Infinite Zoom | Tunnel/spiral into center |
| **Warp** | Möbius, Multi-Inversion, Conformal Warp | Bend/distort space |
| **Cellular** | Voronoi | Cell-based patterns |
| **Flow** | Turbulence, Radial Streak | Directional motion |

## Current State

Effects listed flat in UI, ordered by implementation date. No visual grouping. User must understand each effect individually.

## Proposed Change

Group effects under collapsible category headers in `imgui_effects.cpp`. Transform order list remains flat (effects still chain independently), but UI presentation groups by visual outcome.

## Scope

- UI-only change (no shader/config modifications)
- Affects `src/ui/imgui_effects.cpp` layout
- Optional: category icons or color coding

## Dependencies

Complete kaleidoscope cleanup first (extract Droste/Power Map) so categories reflect actual effect purposes.
