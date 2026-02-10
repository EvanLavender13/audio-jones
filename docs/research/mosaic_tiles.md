# Mosaic Tiles

Dual-offset rounded-box grid that tiles the source image into glowing mosaic cells. Two identical grids offset by half a cell composite over a colored background. Each tile shows a portion of the input texture blended with a cycling random palette tint, with per-cell brightness pulsing at independent phases. The overlapping layers create a layered LED panel / light board aesthetic.

## Classification

- **Category**: TRANSFORMS > Cellular
- **Pipeline Position**: Transform pass (user-ordered with other transforms)
- **Chosen Approach**: Balanced — dual-grid with per-cell pulsing and palette/source blend captures the original shader's layered mosaic character without overcomplicating with per-corner radii or hex grids

## References

- [IQ 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) - sdRoundedBox SDF formula, the standard rounded rectangle distance field
- [Dave Hoskins integer hash](https://www.shadertoy.com/view/4djSRW) - fast per-cell pseudo-random via integer arithmetic (used in original shader)
- Original Shadertoy shader (pasted by user) - dual-offset grid structure, compositing logic, palette cycling approach

## Algorithm

### Grid Construction

Two grids share the same `scale` but offset by half a cell:

```
gridA_uv = inputUV * scale
gridB_uv = inputUV * scale - 0.5
```

Each grid produces cell coordinates (`floor`) and fractional position within cell (`fract`).

### Per-Cell SDF Mask

Each cell renders a rounded box centered at (0.5, 0.5) in cell-local coordinates:

```
d = sdRoundedBox(fract_uv - 0.5, vec2(0.5 - gap), vec4(cornerRadius))
mask = smoothstep(edgeWidth, 0.0, d)
```

`gap` controls spacing between tiles. `cornerRadius` rounds the corners (0 = sharp, approaches circle at high values). `edgeWidth` provides antialiased edges.

### Texture Sampling

Sample source texture at each cell's center position (one sample per cell, constant across the tile):

```
cellCenter = (floor(grid_uv) + 0.5) / scale
texColor = texture(inputTex, cellCenter)
```

This gives each tile a single flat color from the source image, creating the mosaic pixelation.

### Per-Cell Color Tinting

A 3-color palette (configurable) cycles per cell using integer hash:

```
paletteIndex = int(3.0 * hash(floor(grid_uv) + seed + time * colorSpeed))
tileColor = mix(texColor, palette[paletteIndex], tintBlend)
```

`tintBlend` at 0 shows pure source color; at 1 shows pure palette cycling.

### Per-Cell Brightness Pulsing

Each cell pulses independently using its hash as a phase offset:

```
cellHash = hash(floor(grid_uv) + seed)
pulse = 0.75 + pulseDepth * 0.25 * sin(time * pulseSpeed + cellHash * 588.0)
tileColor *= pulse
```

### Dual-Grid Compositing

Grid B composites over Grid A at adjustable opacity, then background fills gaps:

```
color = mix(black, gridA_color, gridA_mask)
color = mix(color, gridB_color, layerOpacity * gridB_mask)
color = mix(color, backgroundColor, 1.0 - clamp(gridA_mask + layerOpacity * gridB_mask, 0, 1))
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| scale | float | 4.0-60.0 | 16.0 | Grid density (cells across screen height) |
| cornerRadius | float | 0.0-0.45 | 0.12 | Tile corner rounding (0 = sharp square) |
| gap | float | 0.0-0.3 | 0.1 | Spacing between tiles |
| tintBlend | float | 0.0-1.0 | 0.3 | Palette vs source color (0 = pure source, 1 = pure palette) |
| pulseDepth | float | 0.0-1.0 | 0.5 | Amplitude of per-cell brightness oscillation |
| pulseSpeed | float | 0.1-4.0 | 1.0 | Rate of brightness oscillation |
| colorSpeed | float | 0.0-2.0 | 0.4 | Rate of palette color cycling |
| layerOpacity | float | 0.0-1.0 | 0.5 | Second grid layer opacity (0 = single grid) |
| brightness | float | 0.5-4.0 | 1.5 | Overall output brightness multiplier |
| bgColor | vec3 | color | (0.36, 0.10, 0.89) | Background color in tile gaps (default purple) |

## Modulation Candidates

- **scale**: zooms the grid in/out, dramatic density shifts
- **cornerRadius**: morphs tiles from sharp rectangles to rounded pills
- **gap**: opens/closes spacing, tiles breathe apart and together
- **tintBlend**: shifts between faithful source reproduction and vivid palette mosaic
- **pulseDepth**: controls how much individual cells flicker vs stay steady
- **layerOpacity**: fades the second grid layer in/out, shifting between single and dual mosaic
- **brightness**: overall intensity pump

### Interaction Patterns

- **Competing Forces**: `tintBlend` vs source image brightness — high tintBlend makes all cells equally vivid (palette-driven), low tintBlend lets the source image's dark/bright regions create natural contrast variation across the grid. Modulating tintBlend shifts between "LED wall showing content" and "abstract color mosaic."
- **Cascading Threshold**: `gap` gates `layerOpacity` visibility — when gap is small, both grid layers overlap almost completely and the second layer blends into the first. As gap increases, the offset tiles separate into distinct floating elements where the second layer becomes visually distinct from the first.

## Notes

- Hash function uses Dave Hoskins integer hash (same as original shader) for GPU-friendly per-cell randomness without texture lookups
- Sampling at cell center means each tile is a flat color — no sub-tile detail. This is the mosaic/pixelation aesthetic. Scale parameter controls how much detail survives.
- At very low scale values (4-8), tiles are large and the effect looks like chunky pixel art. At high values (40+), tiles are tiny and the source image is mostly preserved with a subtle grid overlay.
- The 3-color palette defaults to the original shader's cyan/yellow/red but could be extended to use the codebase's gradient system if desired.
