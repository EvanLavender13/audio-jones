# Bus Mixer UI

Vertical channel strip layout for the modulation bus panel, replacing the horizontal module strip design with a mixer-console aesthetic. 8 strips side by side with tall segmented LED meters as the dominant visual element.

## Classification

- **Category**: General (UI redesign)
- **Pipeline Position**: N/A - purely UI, no engine changes
- **Prerequisite**: Modulation buses fully implemented with horizontal module strip UI

## References

- DAW mixer consoles (Ableton, Logic, FL Studio) - vertical channel strip layout with meter bridge
- Eurorack modular synths - narrow vertical modules with LED meters, dense but readable

## Design Direction

Pro-audio meter bridge. The panel's visual identity comes from 8 vertical strips of glowing, pulsing light - cyan, magenta, orange cycling. When you glance at the panel, you read signal levels instantly across all buses, the same way you'd scan a mixing desk. The meters are the architecture; everything else is controls tucked above them.

## Layout

Bottom-dock recommended (full viewport width). 8 strips side by side with 2px gaps. Each strip is a `BeginChild` filling the panel height.

```
+-----------------------------------------------------------------------------------------------------+
|  Buses                                                                                          _ x  |
| +----------++----------++----------++----------++----------++----------++----------++----------+      |
| | 1 *      || 2 *      || 3 *      || 4 *      || 5 o      || 6 o      || 7 o      || 8 o      |    |
| |gated bass||smooth bea||beat pulse||smth treb ||          ||          ||          ||          |      |
| |[Gate   v]||[EnvFol v]||[EnvTrg v]||[SlwEx  v]||[Mult   v]||[Mult   v]||[Mult   v]||[Mult   v]|    |
| |A[Bass  v]|| [Beat  v]|| [Beat  v]|| [Treb  v]||A[Bass  v]||A[Bass  v]||A[Bass  v]||A[Bass  v]|    |
| |B[LFO1  v]||          ||          ||          ||B[LFO1  v]||B[LFO1  v]||B[LFO1  v]||B[LFO1  v]|    |
| |          ||-Envelope-||-Envelope-||--Slew---||          ||          ||          ||          |      |
| |          ||Atk  0.010||Atk  0.010||Lag  0.20||          ||          ||          ||          |      |
| |          ||Rel  0.300||Rel  0.300||          ||          ||          ||          ||          |      |
| |          ||          ||Hld  0.000||          ||          ||          ||          ||          |      |
| |          ||          ||Thr  0.300||          ||          ||          ||          ||          |      |
| |+--------+||+--------+||+--------+||+--------+||+--------+||+--------+||+--------+||+--------+|    |
| || ====== ||| ====== |||        ||| ====== |||        |||        |||        |||        ||    |
| || ====== ||| ====== |||        ||| ====== |||        |||        |||        |||        ||    |
| || ====== ||| ====== |||        ||| ====== |||        |||        |||        |||        ||    |
| ||        ||| ====== |||   ==   ||| ====== |||        |||        |||        |||        ||    |
| || -  -  -||| ====== |||   ==   ||| ====== ||| -  -  -||| -  -  -||| -  -  -||| -  -  -||    |
| ||        ||| ====== |||   ==   ||| ====== |||        |||        |||        |||        ||    |
| || ====== ||| ====== |||        |||        |||        |||        |||        |||        ||    |
| || ====== ||| ====== |||        |||        |||        |||        |||        |||        ||    |
| |+--------+||+--------+||+--------+||+--------+||+--------+||+--------+||+--------+||+--------+|    |
| |  -0.32   ||   0.71   ||   0.04   ||   0.85   ||   0.00   ||   0.00   ||   0.00   ||   0.00   |    |
| +----------++----------++----------++----------++----------++----------++----------++----------+      |
+-----------------------------------------------------------------------------------------------------+
   <- CYAN ->  <-MAGENTA->  <- ORANGE->  <- CYAN ->  <-MAGENTA->  <- ORANGE->  <- CYAN ->  <-MAGENTA->
```

## Strip Anatomy

Each strip has two zones: controls (top, variable height) and meter (bottom, fills remaining space).

### Header

```
 1 *  gated bass
 ^  ^  ^
 |  |  +-- Name: TEXT_SECONDARY color, InputText with hint "Bus N"
 |  +----- Enable dot: 4px radius, filled=on (accent), outline=off (dim)
 +-------- Bus number: accent color, drawn with AddText
```

When disabled: number uses TEXT_DISABLED color. Name hidden. Dot outline only.

### Controls Zone

**All operators**: Operator combo (full strip width).

**Combinators** (Add, Multiply, Min, Max, Gate, Crossfade, Difference):
- Input A combo (full width, "A" label prefix)
- Input B combo (full width, "B" label prefix)
- No params section

**Envelope Follow**:
- Single input combo (full width, no label prefix)
- SeparatorText("Envelope")
- Attack slider (ModulatableSlider, "%.3f s", 0.001-2.0)
- Release slider (ModulatableSlider, "%.2f s", 0.01-5.0)

**Envelope Trigger**:
- Same as Envelope Follow, plus:
- Hold slider (ModulatableSlider, "%.2f s", 0.0-2.0)
- Threshold slider (ModulatableSlider, "%.2f", 0.0-1.0)

**Slew (Exp/Linear)**:
- Single input combo (full width, no label prefix)
- SeparatorText("Slew")
- Symmetric: Lag Time slider (ModulatableSlider, "%.2f s", 0.01-5.0)
- Asymmetric checkbox swaps Lag Time for Rise Time + Fall Time pair

### Meter Zone

Segmented LED-style bar with glow. Fills all remaining vertical space.

**Geometry:**

| Property | Value |
|----------|-------|
| Segment height | 3px |
| Segment gap | 1px |
| Segment width | strip content width - 8px padding |
| Total segments | meterHeight / 4 (dynamic) |
| Corner radius | 1px per segment |
| Minimum meter height | 80px |

**Bipolar** (combinators): center segment at totalSegments / 2. Positive fills up, negative fills down. Dashed center line at SetColorAlpha(accent, 40).

**Unipolar** (envelope, slew): fills upward from bottom. No center line. Range [0, 1].

**Segment rendering:**

| State | Color |
|-------|-------|
| Lit | SetColorAlpha(accent, 200) |
| Lit glow (drawn first, +2px each side) | SetColorAlpha(accent, 40) |
| Peak hold (single bright segment) | SetColorAlpha(accent, 255) |
| Unlit | SetColorAlpha(accent, 15) |
| Disabled strip | IM_COL32(30, 28, 38, 255) |

**Peak hold**: tracks highest segment reached. Holds 0.5s, then decays at ~8 segments/second. Single bright segment floating above the fill.

**Value readout**: below the meter, centered. Format "%.2f" in SetColorAlpha(accent, 150).

## Strip Background

- Background tint: SetColorAlpha(accent, 6) over default window bg
- Top accent bar: 2px tall, full strip width, SetColorAlpha(accent, 50)

## Disabled Strip

All text TEXT_DISABLED. Outline-only enable dot. No name shown. All meter segments dead dark (no accent tint, no glow). Value readout in TEXT_DISABLED.

## Dimensions

| Element | Value |
|---------|-------|
| Strip width | availableWidth / 8 - gap (target ~130px at 1080p bottom dock) |
| Strip gap | 2px |
| Strip padding | 6px left/right, 4px top |
| Header height | lineHeight + 4px |
| Name height | lineHeight |
| Combo height | lineHeight + framePadding.y * 2 |
| Slider height | lineHeight + framePadding.y * 2 |
| Separator height | lineHeight |
| Meter segment height | 3px |
| Meter segment gap | 1px |
| Meter padding | 4px each side |
| Value readout height | lineHeight |
| Top accent bar | 2px |

## Interaction

| Action | Behavior |
|--------|----------|
| Click enable dot | Toggle bus on/off |
| Click name field | Enter edit mode (InputText), ESC to cancel |
| Change operator | Combo dropdown, strip content reflows |
| Hover meter | Tooltip: bus name + current value + operator |

## Implementation Scope

This is a UI-only change. Replace the contents of `imgui_bus.cpp` (created during mod bus implementation). No changes to:
- Bus processing engine (mod_bus.cpp)
- Serialization (preset.cpp)
- ModSource extension (mod_sources.h/cpp)
- Main loop integration (main.cpp)
- Mod source popup (modulatable_slider.cpp)

The `ImGuiDrawBusPanel` function signature stays the same. Only the rendering internals change from horizontal module strips to vertical mixer strips.

## Notes

- If docked in side panel (626px), strips compress to ~73px each - tight but usable with abbreviated source names in combos
- Bottom dock at 1080p gives ~130px per strip - comfortable for all widgets
- Meter segment count adapts to available height - more space = more segments = smoother visual
- Combinator buses have taller meters than processor buses (fewer controls above) - this is intentional, simpler buses get more visual prominence
