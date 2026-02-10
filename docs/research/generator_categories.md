# Generator Sub-Categories

Generators have grown to 15 effects lumped under a single "Generator" heading. This document defines visual-character-based sub-categories matching how transforms already organize (Symmetry, Warp, Cellular, etc.).

## Grouping Axis

**Visual character** — what it looks like on screen. Matches the transform convention where categories describe the visual technique (not audio mapping or generation method).

## Categories

### Geometric
Structured shapes, arcs, outlines — defined forms with clear edges.

| Effect | Description |
|--------|-------------|
| Signal Frames | Nested rotating rectangle/triangle outlines |
| Arc Strobe | Lissajous figure with sweeping strobe arc |
| Pitch Spiral | Coiled spiral where octave rings glow with active notes |
| Spectral Arcs | Concentric ring arcs per semitone |

### Filament
Thin trails, scattered marks, wispy strands — sparse elements in space.

| Effect | Description |
|--------|-------------|
| Muons | Thin filaments spiraling like particle trails |
| Filaments | Tangled glowing line segments radiating from center |
| Constellation | Drifting star points connected by fading lines |
| Slashes | Scattered diagonal bars in staccato bursts |

### Texture
Space-filling patterns, interference, grids — covers the viewport with repeating visual structure.

| Effect | Description |
|--------|-------------|
| Plasma | Glowing vertical streaks like aurora curtains |
| Interference | Concentric ripples overlapping into moire fringes |
| Moire Generator | Rotating stripe/ring layers creating interference |
| Scan Bars | Colored bars crowding toward a focal point |
| Glyph Field | Scrolling character grids at different depths |

### Atmosphere
Volumetric environment, fills — backgrounds and ambient layers.

| Effect | Description |
|--------|-------------|
| Nebula | Drifting fractal gas clouds with twinkling stars |
| Solid Color | Flat color fill |

## Scope

Implementation touches:
- `src/ui/imgui_effects_generators.cpp` — collapsible sub-category headers
- `src/config/effect_config.h` — generator enum values may need sub-category tagging
- `docs/effects.md` — inventory table gains sub-headings under GENERATORS

## Notes

- New generators slot into whichever sub-category matches their visual character
- Atmosphere is intentionally small — it holds environmental fills, not every dark effect
- Glyph Field lands in Texture despite its character-based content because it fills space with a repeating grid pattern
