---
name: write-effect-description
description: Use when adding an entry to docs/effects.md for a new visual effect
---

# Writing Effect Descriptions

One sentence, **20 words max**, describing what the viewer SEES — not what the shader does.

## Rules

1. **One dominant visual impression.** Pick the single most striking thing a viewer would notice. Do not list modes, features, or parameters.
2. **20 words max.** If it reads like a feature list, cut everything after the first comma or dash.
3. **No implementation language.** Ban: "configurable", "optional", "switchable", "with modes for", "adjustable", "multiple".
4. **Concrete analogy > abstract noun.** "like a toy kaleidoscope" beats "symmetrical radial pattern".

## Examples

| Bad (too long / listy) | Good (single impression) |
|------------------------|--------------------------|
| Overlapping planes of scrolling characters dissolving into abstract texture — rows drift at different speeds, letters flicker and swap, and an optional LCD stripe grid turns everything into a glowing monitor closeup | Scrolling character grids layered at different depths, dissolving into flickering abstract texture |
| Colored bars scrolling and crowding together toward a focal point, switchable between venetian blinds, a spinning spoke fan, and concentric rings — with a snap mode that makes them lurch like a stuck reel | Colored bars crowding toward a focal point like a stuck projector reel lurching forward |
| Ray-sphere intersection with spherical UV tiling and facet normal perturbation | Spinning mirror ball throwing dancing light spots across the walls |
| Golden-angle Vogel disc blur with brightness weighting for bright pixel emphasis | Dreamy out-of-focus blur where bright spots become soft glowing circles |
| Computes voronoi geometry with multiple blendable cell-based effects | Organic cell and bubble patterns with crisp shaded edges |
| Edge detection with directional strokes and paper texture overlay | Hand-drawn graphite shading on rough paper |
