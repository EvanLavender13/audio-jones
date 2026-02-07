# Cosine Palette

Add a fourth color mode (`COLOR_MODE_PALETTE`) to the `ColorConfig` system. Uses the IQ cosine palette formula `color(t) = a + b * cos(2π * (c * t + d))` to generate smooth procedural color from a single float input. Integrates into all existing consumers: drawable vertex coloring, ColorLUT texture baking, and simulation agent hue assignment.

**Research**: `docs/research/cosine-palette.md`

## Design

### Types

**ColorMode enum** — append after `COLOR_MODE_GRADIENT`:

```
COLOR_MODE_PALETTE
```

**ColorConfig struct** — add 12 floats (4 vec3s stored as separate R/G/B components):

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| paletteAR | float | 0.5 | Bias red |
| paletteAG | float | 0.5 | Bias green |
| paletteAB | float | 0.5 | Bias blue |
| paletteBR | float | 0.5 | Amplitude red |
| paletteBG | float | 0.5 | Amplitude green |
| paletteBB | float | 0.5 | Amplitude blue |
| paletteCR | float | 1.0 | Frequency red |
| paletteCG | float | 1.0 | Frequency green |
| paletteCB | float | 1.0 | Frequency blue |
| paletteDR | float | 0.0 | Phase red |
| paletteDG | float | 0.33 | Phase green |
| paletteDB | float | 0.67 | Phase blue |

Stored as individual floats (not arrays/structs) to match existing `ColorConfig` field style and enable flat JSON serialization.

### Algorithm

The cosine palette formula evaluates per-channel:

```
R = clamp(aR + bR * cos(TWO_PI * (cR * t + dR)), 0, 1)
G = clamp(aG + bG * cos(TWO_PI * (cG * t + dG)), 0, 1)
B = clamp(aB + bB * cos(TWO_PI * (cB * t + dB)), 0, 1)
```

Where `t` is the input position (0–1). The clamp handles edge cases where a/b combinations exceed the 0–1 range (e.g., a=0.9 + b=0.8 peaks at 1.7). <!-- Intentional deviation: research notes natural clamping only holds for default a=0.5,b=0.5. Since UI allows arbitrary values, explicit clamp is needed. -->

**For `ColorFromConfig`**: Apply the formula using `interp` (the mirrored t value already computed in the function) to produce an RGB Color.

**For `ColorConfigAgentHue`**: Evaluate the formula at `t = agentIndex / totalAgents`, then convert the resulting RGB to HSV and return the hue component. Same approach as the `COLOR_MODE_GRADIENT` case.

**For `ColorConfigGetSV`**: Evaluate at `t = 0.5` (midpoint), convert to HSV, return S and V. Provides a representative saturation/value for deposit coloring.

### Parameters

| Parameter | Type | Range | Default | UI Label |
|-----------|------|-------|---------|----------|
| paletteA{R,G,B} | float | 0.0–1.0 | 0.5 | Bias R/G/B |
| paletteB{R,G,B} | float | 0.0–1.0 | 0.5 | Amplitude R/G/B |
| paletteC{R,G,B} | float | 0.0–4.0 | 1.0 | Frequency R/G/B |
| paletteD{R,G,B} | float | 0.0–1.0 | 0.0/0.33/0.67 | Phase R/G/B |

No modulation for V1.

### Constants

- Enum value: `COLOR_MODE_PALETTE`
- Display name: `"Palette"` (in the Mode combo dropdown)
- TWO_PI: `6.283185307f` (use `PI * 2` from raylib or define locally)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Add palette fields to ColorConfig

**Files**: `src/render/color_config.h`
**Creates**: `COLOR_MODE_PALETTE` enum value and 12 palette fields on `ColorConfig`

**Do**:
- Add `COLOR_MODE_PALETTE` after `COLOR_MODE_GRADIENT` in the `ColorMode` enum
- Add the 12 float fields to `ColorConfig` with defaults per the Design table
- Place palette fields after the gradient fields block, with a comment grouping them

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (all parallel — no file overlap)

#### Task 2.1: Color evaluation functions

**Files**: `src/render/color_config.cpp`, `src/render/draw_utils.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `draw_utils.cpp` `ColorFromConfig()`: add `else if (color->mode == COLOR_MODE_PALETTE)` case. Evaluate the cosine formula using `interp` as `t`. Clamp each channel to 0–255.
- In `color_config.cpp` `ColorConfigEquals()`: add palette case comparing all 12 floats.
- In `color_config.cpp` `ColorConfigAgentHue()`: add palette case. Evaluate formula at `t`, convert RGB to hue using `ColorConfigRGBToHSV`, return hue. Follow the `COLOR_MODE_GRADIENT` pattern.
- In `color_config.cpp` `ColorConfigGetSV()`: add palette case. Evaluate at `t = 0.5`, convert to HSV, output S and V.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `to_json`: add `case COLOR_MODE_PALETTE` that serializes all 12 float fields.
- In `from_json`: add reads for all 12 palette fields using the existing `j.contains()` / `j[key].get<float>()` pattern.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: UI controls

**Files**: `src/ui/imgui_widgets.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Update the `modes[]` array in `ImGuiDrawColorMode()` to include `"Palette"` (4 entries).
- Update the `ImGui::Combo` count from `3` to `4`.
- Add `else if (color->mode == COLOR_MODE_PALETTE)` block with:
  - 4 groups of 3 sliders each (Bias R/G/B, Amplitude R/G/B, Frequency R/G/B, Phase R/G/B)
  - Use `ImGui::SliderFloat` for each
  - Ranges: a and b 0–1, c 0–4, d 0–1
  - Group with `ImGui::Text("Bias")`, `ImGui::Text("Amplitude")`, etc. as labels

**Verify**: `cmake.exe --build build` compiles. Launch app, select Palette mode on a drawable, confirm sliders appear and color updates.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Palette mode appears in all ColorConfig combo dropdowns (drawables, simulations, generators)
- [ ] Adjusting a/b/c/d sliders updates drawable colors in real time
- [ ] Default values (a=0.5, b=0.5, c=1, d=0/0.33/0.67) produce a rainbow-like palette
- [ ] Saving and loading a preset preserves palette parameters
- [ ] Simulation agents (physarum, boids) display palette-derived hues when palette mode selected
