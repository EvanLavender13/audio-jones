# Infinite Zoom Parallax Fix

Fixes the parallax offset in infinite zoom to be depth-proportional instead of index-based. Currently each layer shifts by `offset * layerIndex`, which has no relationship to the layer's current zoom depth. The fix ties the offset to `1/scale`, so near layers (small scale) shift more and far layers (large scale) shift less — matching how real parallax works. A new `parallaxStrength` multiplier controls the depth exaggeration.

**Research**: `docs/research/infinite_zoom_parallax_fix.md`

## Design

### Types

**InfiniteZoomConfig** — add one field after `offsetLissajous`:
```c
float parallaxStrength = 1.0f; // Depth-proportional parallax multiplier (0.0-5.0)
```

**InfiniteZoomEffect** — add one field after `blendModeLoc`:
```c
int parallaxStrengthLoc;
```

**INFINITE_ZOOM_CONFIG_FIELDS** — append `parallaxStrength` after `offsetLissajous`.

### Algorithm

The shader loop body is reordered so that `phase`, `scale`, and `alpha` are computed **before** `layerCenter`. This lets the parallax formula use `scale` as a depth proxy.

Current loop top (lines 58–74):
```glsl
vec2 layerCenter = center + offset * float(i);
vec2 uv = fragTexCoord - layerCenter;
float phase = fract((float(i) - time) / L);
float scale = exp2(phase * zoomDepth);
float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5;
if (alpha < 0.001) continue;
```

Fixed loop top:
```glsl
float phase = fract((float(i) - time) / L);
float scale = exp2(phase * zoomDepth);
float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5;
if (alpha < 0.001) continue;

vec2 layerCenter = center + offset * (parallaxStrength / scale);
vec2 uv = fragTexCoord - layerCenter;
```

The `1/scale` relationship means:
- `scale = 1` (nearest layer, phase ≈ 0): full offset magnitude
- `scale = exp2(zoomDepth)` (farthest layer): offset shrinks exponentially
- With `zoomDepth = 3.0` (default), far layers get 1/8th the offset of near layers

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `parallaxStrength` | float | 0.0-5.0 | 1.0 | Yes | `"Parallax Strength##infzoom"` |

### Constants

No new enums or constants. This modifies the existing `TRANSFORM_INFINITE_ZOOM` effect.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Add parallaxStrength to config and effect structs

**Files**: `src/effects/infinite_zoom.h`
**Creates**: Config field and effect loc field that Wave 2 tasks reference

**Do**:
1. Add `float parallaxStrength = 1.0f; // Depth-proportional parallax multiplier (0.0-5.0)` to `InfiniteZoomConfig` after `offsetLissajous`
2. Add `int parallaxStrengthLoc;` to `InfiniteZoomEffect` after `blendModeLoc`
3. Add `parallaxStrength` to `INFINITE_ZOOM_CONFIG_FIELDS` macro after `offsetLissajous`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader + CPU (parallel)

#### Task 2.1: Fix shader parallax formula

**Files**: `shaders/infinite_zoom.fs`
**Depends on**: Wave 1 complete

**Do**:
1. Add `uniform float parallaxStrength;` after the existing `blendMode` uniform declaration
2. Reorder the loop body: move `phase`, `scale`, `alpha` computation and the `alpha < 0.001` early-out to **before** the `layerCenter` / `uv` calculation. Implement the Algorithm section above.
3. Replace `center + offset * float(i)` with `center + offset * (parallaxStrength / scale)`

All code after the `uv = fragTexCoord - layerCenter` line remains unchanged.

**Verify**: File parses (no GLSL syntax errors at runtime).

#### Task 2.2: Wire uniform, register param, add UI slider

**Files**: `src/effects/infinite_zoom.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. **Init**: Add `e->parallaxStrengthLoc = GetShaderLocation(e->shader, "parallaxStrength");` after the `blendModeLoc` line
2. **Setup**: Add `SetShaderValue(e->shader, e->parallaxStrengthLoc, &cfg->parallaxStrength, SHADER_UNIFORM_FLOAT);` — place it after the offset block (after line 79)
3. **RegisterParams**: Add `ModEngineRegisterParam("infiniteZoom.parallaxStrength", &cfg->parallaxStrength, 0.0f, 5.0f);` — place it before the offsetX registration
4. **UI**: In `DrawInfiniteZoomParams`, in the Parallax section (after the `SeparatorText("Parallax")` call), add `ModulatableSlider("Parallax Strength##infzoom", &e->infiniteZoom.parallaxStrength, "infiniteZoom.parallaxStrength", "%.2f", ms);` as the first widget before the Offset X slider

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Parallax offset visibly shifts near layers more than far layers when offsetX/offsetY are non-zero
- [ ] `parallaxStrength = 0` disables parallax entirely (all layers share the same center)
- [ ] `parallaxStrength = 1` gives natural depth separation
- [ ] Lissajous modulation on offset still works (feeds into the depth-proportional formula)
- [ ] Preset save/load round-trips the new `parallaxStrength` field
