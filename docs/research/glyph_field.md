# Glyph Field

Layered planes of scrolling character grids where text dissolves into abstract visual texture. Multiple flat grids at different scales and speeds overlap with transparency, creating parallax depth. Characters lose legibility — becoming pattern, rhythm, and color fields that breathe, flicker, and break apart like a corrupted transmission viewed through stacked screens.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage alongside Plasma, Interference, Constellation
- **Chosen Approach**: Full — the layered plane architecture, multiple motion modes, and three scroll directions are core to the concept. Cutting any would lose the "abstract texture + typographic chaos" identity.

## References

- [Hash Functions for GPU Rendering (JCGT 2020)](https://jcgt.org/published/0009/03/02/) — PCG3D hash: Pareto-optimal for per-cell randomization quality and speed
- [Polar Coordinates — Ronja's Tutorials](https://www.ronja-tutorials.com/post/053-polar-coordinates/) — Cartesian-to-polar conversion, angular/radial tiling, seam handling
- [Necromurlok (Shadertoy ssf3zX)](https://www.shadertoy.com/view/ssf3zX) — Primary visual reference: per-row scrolling text grid with PCG3D hash coloring and step-based band distortion
- [XorDev "The Typist" (Shadertoy sd3czM)](https://www.shadertoy.com/view/sd3czM) — LCD sub-pixel RGB stripe technique via `sin(p.x + vec4(0,2,4,6)) * sin(p.y)`

## Algorithm

### Font Atlas

Ship a 256x256 PNG bitmap containing a 16x16 grid of ASCII characters (256 glyphs). Each cell occupies 16x16 pixels. Sample a character by index:

```glsl
// charIdx: integer 0..255 mapped to (col, row) in 16x16 grid
vec2 cellOrigin = vec2(charIdx % 16, charIdx / 16) / 16.0;
vec2 charUV = cellOrigin + localUV / 16.0;
float glyph = texture(fontAtlas, charUV).r;
```

### PCG3D Hash

Stateless 3D hash from the reference shader. Three multiply-XOR rounds produce uncorrelated per-cell random vectors without any state texture:

```glsl
uvec3 pcg3d(uvec3 p) {
    p = p * 1234567u + 1234567890u;
    p.x *= p.y + p.z; p.y *= p.x + p.z; p.z *= p.y + p.x;
    p ^= p >> 16;
    p.x *= p.y + p.z; p.y *= p.x + p.z; p.z *= p.y + p.x;
    return p;
}
vec3 pcg3df(vec3 p) {
    return vec3(pcg3d(floatBitsToUint(p))) / float(0xFFFFFFFFu);
}
```

Each cell hashes its grid coordinate to get: character index, color LUT position, scroll speed offset, flutter phase, inversion state.

### Grid Layout

For each layer, compute cell coordinates from UV:

```
cellCoord = floor(uv * gridSize)
localUV   = fract(uv * gridSize)
```

`gridSize` controls character density. Each layer uses a different `gridSize` scaled by `layerScaleSpread`.

### Scroll Direction Modes

**Horizontal (per-row):** Hash the row index → each row scrolls left or right at a random speed.
```
uv.x += time * (hash(row).x - 0.5) * scrollSpeed
```

**Vertical (per-column):** Hash the column index → each column scrolls up or down.
```
uv.y += time * (hash(col).x - 0.5) * scrollSpeed
```

**Radial (per-ring):** Convert to polar coordinates, then treat angle as the "row" axis and radius as the "column" axis. Each concentric ring scrolls angularly at its own speed.
```
float r     = length(uv - center);
float theta = atan(uv.y - center.y, uv.x - center.x);
// Normalize theta to [0, 1], r to desired range
polarUV = vec2(theta / TAU, r * gridSize);
// Per-ring angular scroll
polarUV.x += time * (hash(ringIndex).x - 0.5) * scrollSpeed;
// Then grid as usual: cellCoord = floor(polarUV * gridSize), etc.
```

Seam at theta=0 handled by `fract()` wrapping. Inner rings have fewer cells naturally (circumference shrinks), maintaining roughly square character aspect.

### Motion Modes (Mixable)

All four modes blend via per-mode amplitude parameters (0 = off, 1 = full):

**Column/Row Rain** — The base scroll described above. `rainAmount` scales the scroll offset.

**Cell Flutter** — Each cell independently cycles its character index on its own timer. `flutterSpeed` controls cycle rate, `flutterAmount` scales the index offset added per frame:
```
charOffset = floor(time * flutterSpeed + hash(cell).y * 100.0)
charIdx += charOffset * flutterAmount
```

**Wave Distortion** — Sine-based UV displacement applied to the grid before cell quantization:
```
uv += sin(uv.yx * waveFreq + time * waveSpeed) * waveAmplitude
```
Characters stretch, compress, and ripple. Affects cell boundaries, not just content.

**Drift and Scatter** — Per-cell position offset that slowly wanders using time-seeded hash:
```
vec2 drift = (hash(cell + floor(time * driftSpeed)).xy - 0.5) * driftAmount
uv += drift
```
Occasionally cells burst apart when the hash crosses a threshold, then reform.

### Band Distortion

Step-based UV scaling from the reference shader creates uneven row/column heights. Multiple `step()` thresholds multiply UV at different distances from center:
```
uv *= 1.0 + step(threshold1, abs(uv.y)) * bandStrength;
uv *= 1.0 + step(threshold2, abs(uv.y)) * bandStrength;
```
Rows near center render at normal size; rows toward edges compress. Creates an anamorphic/fisheye feel without actual perspective math.

### Inversion Flicker

Animated percentage of cells swap foreground/background:
```
float flickerHash = fract(hash(cell).y + time * inversionSpeed);
if (flickerHash < inversionRate)
    glyph = (1.0 - glyph);
```
`inversionRate` 0.0 = no inversion, 0.5 = half the cells inverted at any moment, 1.0 = all inverted.

### Layer Compositing

Loop over `layerCount` layers (1–4). Each layer:
- Scales UV by `baseGridSize * pow(layerScaleSpread, layerIndex)`
- Offsets scroll speed by `baseSpeed * pow(layerSpeedSpread, layerIndex)`
- Seeds hash with `layerIndex` to decorrelate patterns
- Multiplies result by per-layer opacity falloff

Layers composite additively — overlapping characters create brighter interference zones where grids align.

### Coloring

Per-cell hash maps to gradient LUT position:
```glsl
float t = hash(cellCoord + layerSeed).x;
vec3 color = textureLod(gradientLUT, vec2(t, 0.5), 0.0).rgb;
finalColor = glyph * color * cellBrightness;
```

Standard `ColorConfig` / `ColorLUT` integration (same pattern as Plasma, Interference). User controls full palette via solid/rainbow/gradient modes.

### Optional: LCD Sub-pixel Mode

XorDev's technique overlays micro RGB stripes per pixel:
```glsl
vec3 lcd = max(sin(pixelCoord.x * lcdFreq + vec3(0, 2, 4)) * sin(pixelCoord.y * lcdFreq), 0.0);
finalColor *= lcd;
```
Toggle-able as a style option. Creates the look of characters displayed on a physical LCD panel.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| gridSize | float | 8–64 | 24 | Character density — cells per screen height |
| layerCount | int | 1–4 | 2 | Number of overlapping grid planes |
| layerScaleSpread | float | 0.5–2.0 | 1.4 | Scale ratio between successive layers |
| layerSpeedSpread | float | 0.5–2.0 | 1.3 | Speed ratio between successive layers |
| layerOpacity | float | 0.1–1.0 | 0.6 | Opacity falloff per layer (multiplied per depth) |
| scrollDirection | enum | H/V/Radial | Horizontal | Axis of primary character movement |
| scrollSpeed | float | 0.0–2.0 | 0.4 | Base scroll velocity |
| rainAmount | float | 0.0–1.0 | 1.0 | Row/column rain motion intensity |
| flutterAmount | float | 0.0–1.0 | 0.3 | Per-cell character cycling intensity |
| flutterSpeed | float | 0.1–10.0 | 2.0 | Character cycling rate |
| waveAmplitude | float | 0.0–0.5 | 0.05 | Sine distortion strength |
| waveFreq | float | 1.0–20.0 | 6.0 | Sine distortion spatial frequency |
| waveSpeed | float | 0.0–5.0 | 1.0 | Sine distortion animation speed |
| driftAmount | float | 0.0–0.5 | 0.0 | Per-cell position wander magnitude |
| driftSpeed | float | 0.1–5.0 | 0.5 | Position wander rate |
| bandDistortion | float | 0.0–1.0 | 0.3 | Step-based row height variation |
| inversionRate | float | 0.0–1.0 | 0.1 | Fraction of cells with inverted glyphs |
| inversionSpeed | float | 0.0–2.0 | 0.1 | How fast inversion states rotate |
| lcdMode | bool | on/off | off | LCD sub-pixel RGB stripe overlay |
| lcdFreq | float | 100–2000 | 800 | LCD stripe spatial frequency |
| gradient | ColorConfig | — | cyan→magenta | Color palette via gradient LUT |

## Modulation Candidates

- **scrollSpeed**: overall movement intensity
- **gridSize**: character density pulsing
- **waveAmplitude**: distortion intensity
- **waveFreq**: distortion pattern tightness
- **inversionRate**: flicker chaos amount
- **flutterSpeed**: character cycling frenzy
- **driftAmount**: scatter/explosion intensity
- **bandDistortion**: anamorphic squeeze
- **layerOpacity**: depth visibility

## Asset Requirements

- **Font atlas PNG** (256x256, 16x16 character grid, single-channel or RGBA with glyphs in red channel). Source options: export from a bitmap font editor, render from a TTF at init, or use a CC0 bitmap font sheet. The atlas ships in a new `assets/` or `fonts/` directory.

## Notes

- **Performance**: 4 layers × full-screen fragment shader with 1 texture fetch (atlas) + 1 LUT fetch per layer per pixel. PCG3D is ALU-only (no texture). Should run well under 1ms at 1080p.
- **Matrix Rain overlap**: Matrix Rain is a retro *transform* that overlays procedural runes on existing content. Glyph Field is a *generator* that creates standalone abstract texture. Different pipeline position, different purpose, no font atlas.
- **Radial seam**: `fract()` wrapping at theta=0 may show a visible join if characters span the seam. Mitigate by ensuring `gridSize` produces integer angular divisions, or accept the artifact as part of the aesthetic.
- **Band distortion in radial mode**: Step thresholds apply to radius instead of y-coordinate, creating uneven ring widths.
