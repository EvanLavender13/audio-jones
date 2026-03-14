# UI Design Language: Signal Stack

A unified visual grammar for all AudioJones panels and effect UIs.

## Classification

- **Category**: General (UI architecture / standardization)
- **Pipeline Position**: N/A — cross-cutting concern affecting all panels and 112+ effects

## The Problem

### Panel-Level

Five different header mechanisms used inconsistently:

| Panel | Current Headers |
|-------|----------------|
| Presets | `SeparatorText` |
| Playlist | Custom transport strip |
| Drawables | `TextColored` (redundant header, type name) |
| Analysis | Mixed: `TextColored` (3 sections) + `DrawSectionBegin` (2 sections) |
| LFOs | `DrawGroupHeader` + `DrawSectionBegin` per LFO |
| Effects | `DrawGroupHeader` + `DrawCategoryHeader` + `DrawSectionBegin` + `SeparatorText` |

### Effect-Level (The Main Problem)

Across 112+ effects:

| Pattern | Count | Issue |
|---------|-------|-------|
| **Clean** (`SeparatorText` sections) | ~80 | But section ordering and naming varies |
| **Flat** (no sections) | ~20-30 | No grouping even with 8+ params |
| **Outdated** (`TreeNodeAccented`) | 3 | crt, glitch, toon — retired pattern |
| **Mixed** (manual `Separator`/`Spacing`) | ~5 | Ad-hoc visual breaks |

Section ordering inconsistency:
- Audio first in 64% of effects, middle in 28%, last in 8%
- Same concept has multiple names: "Geometry" vs "Shape" vs "Grid", "Animation" vs "Motion" vs "Timing", "Glow" vs "Visual" vs "Rendering"

---

## Design Principles

### Collapsible Only Where Density Demands It

**Collapsible (`DrawSectionBegin`)**: Only in the Effects window for individual effects. 112 effects competing for scroll space — collapsing is essential for navigation.

**Never collapsible**: LFOs, Analysis, Drawables, and subsections within an expanded effect. If you navigated to a panel or opened an effect, you're there to see it. Collapsing defeats the purpose.

### Three Visual Divider Patterns

Instead of one collapsible widget used everywhere, three distinct patterns for three different contexts:

**1. `SeparatorText("Label")`** — The universal section divider within panels and effects.
Lightweight inline text divider with theme border. No box, no accent bar, no interaction. Used for organizing params into groups (Audio, Geometry, Animation, etc.) inside any panel or effect. This is the default choice.

**2. Module Strip** — For compact repeating items (LFOs, drawables in list).
A new pattern: thin horizontal band with a left accent edge (2-3px colored bar), optional subtle background tint, and content flowing inline. No collapse arrow. No click interaction on the header itself. Feels like a rack-mounted module in a synthesizer — always visible, always compact. Distinguished from its neighbors by the accent color cycling (Cyan → Magenta → Orange).

**3. `DrawSectionBegin/End()`** — For collapsible effect entries in the Effects window ONLY.
Gradient box + left accent bar + ±arrow. Used when 112 effects need density management. The only place collapsibility exists in the UI.

### Remove What's Dead

| Widget | Status |
|--------|--------|
| `TreeNodeAccented()` / `TreeNodeAccentedPop()` | **Remove** — 3 remaining call sites (crt, glitch, toon) convert to `SeparatorText` |
| `SliderFloatWithTooltip()` | **Remove** — zero call sites, superseded by `ModulatableSlider` |
| `TextColored()` as headers | **Stop using** — no affordance, no structure. Replace with `SeparatorText` or Module Strip |
| `DrawGroupHeader()` in LFOs | **Remove** — redundant with window title |

---

## New Widget: Module Strip

A compact, always-visible container for repeating items. Designed for the LFO panel and potentially the drawable list's selected-item header.

### Visual Design

```
┌─ 3px accent bar                    ┐
│                                     │  ← subtle bg tint (accent color @ 8% alpha)
│  [Enable dot]  LFO 1    Rate ══════│  ← top rim-light (accent @ 24% alpha, 1px)
│                                     │
│  ▿△□~⌇≈  ╭────────────────╮  ║    │  ← waveform icons, history preview, meter
│           ╰────────────────╯  ║    │
└─────────────────────────────────────┘
   (repeat for LFO 2, 3, ...)
```

Key visual elements:
- **Left accent bar**: 3px vertical, full height of the module. Color cycles per item index.
- **Background tint**: Accent color at ~8% alpha (same as `DrawCategoryHeader` uses). Provides subtle grouping without heavy boxes.
- **Top rim-light**: 1px horizontal line at top edge, accent color at ~24% alpha. Defines the top boundary crisply without a full border.
- **No bottom border**: The next module's rim-light or the panel edge provides closure. Avoids double-line clutter between adjacent modules.
- **No collapse arrow**: Content is always visible.
- **Enable indicator**: Small filled circle (4px radius) in accent color when enabled, dim border-only circle when disabled. Replaces a checkbox — cleaner in a compact layout.

### Implementation Sketch

```c
// In imgui_panels.h
void DrawModuleStripBegin(const char *label, ImU32 accentColor, bool enabled);
void DrawModuleStripEnd(void);
```

`DrawModuleStripBegin`:
1. Get cursor pos + content width
2. Draw background tint rect (accent @ 8% alpha)
3. Draw left accent bar (3px × module height — deferred to End since height unknown; or use a fixed height)
4. Draw top rim-light (1px line, accent @ 24% alpha)
5. Draw enable indicator circle
6. Draw label text
7. `ImGui::Indent(12.0f)` for content

`DrawModuleStripEnd`:
1. `ImGui::Unindent(12.0f)`
2. Draw left accent bar now that we know the height (use `GetCursorScreenPos().y - startY`)
3. `ImGui::Spacing()`

**Deferred height approach**: Store `startY` in a static, compute height in End, draw the accent bar retroactively. This is the same technique `TreeNodeAccentedPop` uses.

---

## Panel Redesigns

### LFO Panel

**Current**: `DrawGroupHeader("LFOS")` + `DrawSectionBegin` per LFO (collapsible).
**Problem**: Collapsing LFOs creates a window full of closed headers. The group header is redundant with the window title. Each LFO section, when collapsed, hides the waveform preview and controls — exactly the things you came to this panel to see.

**Redesign**: Module Strip per LFO, always visible.

```
┌─────────────────────────────────────────┐
│ LFOs (window title)                     │
│                                         │
│ ● LFO 1        Rate [═══|════]  0.50 Hz│  ← Module Strip, cyan accent
│   ▿△□~⌇≈  ╭─────────────────╮  ║      │
│            ╰─────────────────╯  ║      │
│                                         │
│ ● LFO 2        Rate [══|═════]  1.20 Hz│  ← Module Strip, magenta accent
│   ▿△□~⌇≈  ╭─────────────────╮  ║      │
│            ╰─────────────────╯  ║      │
│                                         │
│ ○ LFO 3        Rate [═|══════]  0.25 Hz│  ← Module Strip, orange, disabled (dimmed)
│   ▿△□~⌇≈  ╭─────────────────╮  ║      │
│            ╰─────────────────╯  ║      │
│                                         │
│ ...                                     │
└─────────────────────────────────────────┘
```

Changes:
1. Remove `DrawGroupHeader("LFOS")` — window title is sufficient
2. Replace `DrawSectionBegin` per LFO with `DrawModuleStripBegin` — always visible, compact
3. Enable indicator: accent-colored filled circle (enabled) or dim outline circle (disabled) replaces hidden checkbox
4. Keep waveform icons, history preview, and output meter layout — these are already well-designed
5. Dim the entire module when disabled (push `TEXT_DISABLED` color)

### Analysis Panel

**Current**: Mixed `TextColored` (Beat Detection, Band Energy, Profiler) and `DrawSectionBegin` (Audio Features, Zone Timing).
**Problem**: Inconsistent — some collapsible, some not. The user is monitoring; they don't collapse metrics.

**Redesign**: `SeparatorText` for all section labels. Everything always visible.

```
┌─────────────────────────────────────┐
│ Analysis (window title)             │
│                                     │
│ ─── Beat Detection ─────────────── │  ← SeparatorText
│ ┌─────────────────────────────────┐ │
│ │ ▌▌▌▌▌▌▌▌▌▌▌▌▌▌▌█████▌▌▌▌▌▌▌▌ │ │  ← beat graph (unchanged)
│ └─────────────────────────────────┘ │
│                                     │
│ ─── Band Energy ────────────────── │  ← SeparatorText
│ ┌─────────────────────────────────┐ │
│ │ BASS ████████████               │ │  ← 3-bar meter (unchanged)
│ │ MID  ██████████████████         │ │
│ │ TREB ████████                   │ │
│ └─────────────────────────────────┘ │
│                                     │
│ ─── Audio Features ─────────────── │  ← SeparatorText (was DrawSectionBegin)
│ ┌─────────────────────────────────┐ │
│ │ Cent ████████  Flat ██████      │ │  ← 2-col meter grid (unchanged)
│ │ Sprd ████      Roll ████████    │ │
│ │ Flux ██████    Crst ██████████  │ │
│ └─────────────────────────────────┘ │
│                                     │
│ ─── Profiler ───────────────────── │  ← SeparatorText (was TextColored)
│ ┌─────────────────────────────────┐ │
│ │ 47%  ████████████     60fps     │ │  ← budget bar (unchanged)
│ └─────────────────────────────────┘ │
│ ┌─────────────────────────────────┐ │
│ │ Feedback ███ Sim ██ Draw █ Out █│ │  ← flame bar (unchanged)
│ └─────────────────────────────────┘ │
│                                     │
│ ─── Zone Timing ────────────────── │  ← SeparatorText (was DrawSectionBegin)
│ Feedback  ▁▂▃▂▁▂▃▄▃▂  0.42      │
│ Sim       ▁▁▂▁▁▂▃▂▁▁  0.28      │  ← sparklines (unchanged)
│ Drawables ▁▁▁▁▁▁▁▂▁▁  0.05      │
│ Output    ▁▂▁▂▃▂▁▂▃▂  0.31      │
└─────────────────────────────────────┘
```

Changes:
1. Replace `TextColored("Beat Detection")` etc. with `SeparatorText("Beat Detection")`
2. Replace `DrawSectionBegin("Audio Features")` with `SeparatorText("Audio Features")` — remove collapsibility
3. Replace `DrawSectionBegin("Zone Timing")` with `SeparatorText("Zone Timing")` — remove collapsibility
4. All graphs/meters unchanged — they already look great

### Drawables Panel

**Current**: `TextColored("Drawable List")` header + `BeginListBox` + `TextColored("Waveform Settings")` + collapsible `DrawSectionBegin` sections (Geometry, Animation, Color).
**Problem**: Redundant headers, plain ListBox, collapsible sections within a detail view.

**Redesign**: Rich list (custom-drawn like playlist) + `SeparatorText` sections (non-collapsible).

```
┌──────────────────────────────────────────┐
│ Drawables (window title)                 │
│                                          │
│ [+ Waveform] [+ Spectrum] [+ Shape] ... │  ← action bar (unchanged)
│                                          │
│ ┌──────────────────────────────────────┐ │
│ │ ■ 1  W  Waveform 1                  │ │  ← rich list rows
│ │ ■ 2  S  Spectrum 1              x   │ │     (swatch + index + badge + name)
│ │ ■ 3  W  Waveform 2                  │ │     (x on hover, drag-to-reorder)
│ └──────────────────────────────────────┘ │
│                                          │
│ [✓ Enabled]   Path [Circular ▾]          │  ← selected item controls
│                                          │
│ ─── Geometry ──────────────────────────  │  ← SeparatorText (was DrawSectionBegin)
│ X        [════|═══════]                  │
│ Y        [══════|═════]                  │
│ Radius   [═══|════════]                  │
│ Height   [═════|══════]                  │
│ Thickness[══|═════════]                  │
│                                          │
│ ─── Animation ─────────────────────────  │  ← SeparatorText (was DrawSectionBegin)
│ Spin     [════════|═══]                  │
│ Angle    [══|═════════]                  │
│ Opacity  [═══════════|]                  │
│                                          │
│ ─── Color ─────────────────────────────  │  ← SeparatorText (was DrawSectionBegin)
│ Mode     [Solid ▾]                       │
│ Color    [■ picker]                      │
└──────────────────────────────────────────┘
```

Changes:
1. Remove `TextColored("Drawable List")` — window title is sufficient
2. Convert list from `BeginListBox` to Rich List pattern:
   - Custom-drawn rows in `BeginChild` (like playlist)
   - Color swatch + index number + type badge (`W`/`S`/`P`/`T`) + name
   - Active row: accent tint background
   - Hovered row: subtle tint + `x` remove button
   - Drag-to-reorder (replaces Up/Down buttons)
3. Remove `TextColored("Waveform Settings")` etc. — list highlight shows which item is selected
4. Replace all `DrawSectionBegin` in `drawable_type_controls.cpp` with `SeparatorText` — Geometry, Animation, Color, Texture, Path, Shape, Gate, Random Walk all become non-collapsible
5. Enabled checkbox and Path combo sit directly below the list (no section wrapper)

---

## Effect UI Standard

This is the core of the standardization effort. Every effect's `Draw<Name>Params()` function follows these rules. These params are shown inside a `DrawSectionBegin` block (the per-effect collapsible section in the Effects window), so everything below uses `SeparatorText` for organization — never collapsible.

### Section Ordering

Canonical order. Skip sections that don't apply. When present, they appear in this order:

```
1. Audio          ← FFT parameters (baseFreq, maxFreq, gain, contrast, baseBright)
2. Geometry       ← Spatial structure (scale, cellSize, layers, sides, radius)
3. [Domain]       ← Effect-specific sections (Wave, Field, Raymarching, etc.)
4. Animation      ← Time-based motion (speed, phase, frequency, spin)
5. Glow           ← Brightness/emission (threshold, intensity, radius, softness)
6. Color          ← Color mapping (palette, gradient, tint, saturation)
```

**Rules:**
- **Audio always comes first** when present. It defines the input signal; everything else reacts to it.
- **Geometry comes second** — the spatial foundation.
- **Domain-specific sections** go in the middle (0-4 of these, names unique to the effect).
- **Animation comes after domain** — it modulates what's already defined.
- **Glow and Color come last** — output-stage concerns.

### Section Name Vocabulary

| Standard Name | Replaces | When to Use |
|---------------|----------|-------------|
| `"Audio"` | — | FFT: baseFreq, maxFreq, gain, contrast, baseBright |
| `"Geometry"` | "Shape", "Grid", "Layout", "Ring Layout", "Structure" | Spatial: scale, size, layers, radius, count, offset |
| `"Animation"` | "Motion", "Timing", "Dynamics", "Movement" | Time: speed, phase, frequency, spin, drift |
| `"Glow"` | "Visual", "Rendering", "Appearance", "Bloom" | Brightness: threshold, intensity, radius, softness |
| `"Color"` | "Tonemap", "Palette" | Color: tint, saturation, hue shift |
| `"Trail"` | "Decay", "History", "Persistence" | Feedback persistence: decay, fade, accumulation |

Domain-specific names are fine when no standard name fits (e.g., "Flocking", "Pressure", "Wave", "Raymarching", "Curvature").

### Audio Section Convention

When present, always this order:

```
SeparatorText("Audio")
ModulatableSlider("Base Freq (Hz)", &cfg->baseFreq, ...)    // 27.5-440, default 55
ModulatableSlider("Max Freq (Hz)",  &cfg->maxFreq, ...)     // 1000-16000, default 14000
ModulatableSlider("Gain",           &cfg->gain, ...)         // 0.1-10, default 2.0
ModulatableSlider("Contrast",       &cfg->curve, ...)        // 0.1-3, default 1.5
ModulatableSlider("Base Bright",    &cfg->baseBright, ...)   // 0-1, default 0.15
```

### Flat Effects Policy

- **5 or fewer params**: flat layout acceptable, no sections needed
- **6+ params**: must have sections following the canonical order

---

## Effects Window Hierarchy (Unchanged)

The Effects window is the only place the full 4-level hierarchy is needed:

```
Level 1: DrawGroupHeader ──── FEEDBACK, OUTPUT, SIMULATIONS, GENERATORS, TRANSFORMS
  Level 2: DrawCategoryHeader ──── Symmetry, Warp, Cellular, Motion, ...
    Level 3: DrawSectionBegin/End ──── Kaleidoscope, Bloom, Voronoi, ... (COLLAPSIBLE)
      Level 4: SeparatorText ──── Audio, Geometry, Animation, Glow, Color (NOT collapsible)
```

This is already correct in structure. Phase 3 fixes the content within Level 4 (section ordering, naming, flat effects).

---

## Effects Refactor Priorities (Phase 3)

### Priority 1: Eliminate Outdated Patterns

| File | Current | Change |
|------|---------|--------|
| `crt.cpp` | `TreeNodeAccented` (5 sections) | → `SeparatorText` |
| `glitch.cpp` | 9+ `TreeNodeAccented` sections | → `SeparatorText` |
| `toon.cpp` | Mixed flat + `TreeNodeAccented` | → `SeparatorText` |

### Priority 2: Organize Flat Effects (6+ Params)

Add `SeparatorText` sections to: `flux_warp` (7), `voronoi` (8), `impressionist` (8), `interference_warp` (8), `domain_warp` (6), and others identified in audit.

### Priority 3: Normalize Section Ordering

Move Audio to first position in: `constellation`, `muons`, `digital_shard`, `faraday`, `fireworks`, `shell`, `vortex`.

### Priority 4: Normalize Section Names

Rename to canonical vocabulary across all effects.

---

## Rollout Strategy

### Phase 1: Establish the Standard (Simple Panels)

Refactor in order:
1. **LFOs** — Replace `DrawGroupHeader` + `DrawSectionBegin` with Module Strip pattern
2. **Analysis** — Replace mixed `TextColored` / `DrawSectionBegin` with uniform `SeparatorText`
3. **Drawables** — Rich list, replace collapsible sections with `SeparatorText`, remove redundant headers

### Phase 2: Document as UI/UX Guide

Extract proven patterns into permanent documentation. This becomes the reference for all future UI work and the authority for Phase 3 reviews.

### Phase 3: Effects Audit & Refactor

Sweep by category using the guide: outdated patterns → flat effects → section ordering → section names.

---

## Helper Changes Summary

### New
| Helper | Purpose |
|--------|---------|
| `DrawModuleStripBegin()` | Compact always-visible container for repeating items (LFOs) |
| `DrawModuleStripEnd()` | Close module strip, draw deferred accent bar |

### Keep
| Helper | Where Used |
|--------|-----------|
| `DrawGroupHeader()` | Effects window groups only |
| `DrawCategoryHeader()` | Effects window transform categories only |
| `DrawSectionBegin/End()` | Effects window per-effect sections only |
| `SeparatorText()` | Everywhere else — the universal section divider |
| `DrawGradientBox()` | Graph/meter backgrounds |
| `DrawGlow()` | Emphasis behind elements |
| `IntensityToggleButton()` | Float 0/1 toggles |

### Remove
| Helper | Reason |
|--------|--------|
| `TreeNodeAccented()` | Superseded — 3 call sites convert to `SeparatorText` |
| `TreeNodeAccentedPop()` | Paired with above |
| `SliderFloatWithTooltip()` | Zero call sites |

---

## Notes

- All Phase 1 changes are purely UI — no config struct, serialization, or shader changes
- Module Strip is a new widget in `imgui_panels.cpp` — ~40 lines of implementation
- Phase 3 effect refactors are also UI-only (adding/renaming `SeparatorText` calls, reordering sliders)
- The standard is enforced by convention and review, not by code
