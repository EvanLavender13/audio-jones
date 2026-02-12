# Glyph Field Stutter Mask

Adds per-lane freeze/resume toggling and discrete cell-snapping to the existing Glyph Field generator. Creates a nervous data-stream stutter inspired by Ryoji Ikeda-style cascading text displays.

**Research**: `docs/research/glyph_field_stutter.md`

## Design

### Types

Extend `GlyphFieldConfig` with three new fields (after `scrollSpeed`):

```
float stutterAmount = 0.0f;   // Fraction of lanes frozen (0.0-1.0)
float stutterSpeed = 1.0f;    // Freeze/unfreeze toggle rate (0.1-5.0)
float stutterDiscrete = 0.0f; // Smooth-to-cell-snap blend (0.0-1.0)
```

Extend `GlyphFieldEffect` with:

```
float stutterTime;       // CPU-accumulated: stutterSpeed * dt
int stutterAmountLoc;
int stutterTimeLoc;
int stutterDiscreteLoc;
```

### Algorithm

#### Stutter Mask (GLSL)

For each scroll lane (row, column, or ring depending on direction):

1. Hash lane index to get per-lane toggle rate: `laneHash = pcg3df(vec3(laneIdx, layerF, 91.3)).x`
2. Discretize time by that rate: `step = floor(stutterTime * (0.5 + laneHash))`
3. Binary gate: `gate = step(0.5, sin(step))`
4. Blend with amount: `mask = mix(1.0, gate, stutterAmount)`

Multiply the scroll offset by `mask`. When `mask = 0`, the lane freezes.

#### Discrete Quantization (GLSL)

After computing scroll offset for a lane:

1. Compute quantized offset: `qOffset = floor(offset * gs) / gs`
2. Blend: `finalOffset = mix(offset, qOffset, stutterDiscrete)`

Replace the raw offset with `finalOffset` before modifying `cellCoord`/`localUV`.

#### Integration Point

Both operations insert into the existing scroll direction branches (`glyph_field.fs` lines 119-144). Each branch already computes a scroll offset per lane — the stutter mask multiplies that offset, and the discrete blend wraps it before it modifies `cellCoord`/`localUV`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| stutterAmount | float | 0.0-1.0 | 0.0 | Yes | Stutter |
| stutterSpeed | float | 0.1-5.0 | 1.0 | Yes | Stutter Speed |
| stutterDiscrete | float | 0.0-1.0 | 0.0 | Yes | Discrete |

### UI Layout

New "Stutter" section after "Scroll", before "Motion":

```
ImGui::SeparatorText("Stutter");
ModulatableSlider("Stutter##glyphfield", ...)       // stutterAmount
ModulatableSlider("Stutter Speed##glyphfield", ...)  // stutterSpeed
ModulatableSlider("Discrete##glyphfield", ...)       // stutterDiscrete
```

---

## Tasks

### Wave 1: Config + Shader (no file overlap)

#### Task 1.1: Extend config and effect structs

**Files**: `src/effects/glyph_field.h`, `src/effects/glyph_field.cpp`, `src/config/preset.cpp`

**Do**:
- Add the 3 config fields to `GlyphFieldConfig` after `scrollSpeed`, with defaults and range comments matching existing style
- Add `stutterTime` accumulator and 3 uniform location ints to `GlyphFieldEffect`
- In `CacheLocations()`: cache the 3 new uniform locations
- In `GlyphFieldEffectInit()`: zero-initialize `stutterTime`
- In `GlyphFieldEffectSetup()`: accumulate `stutterTime += cfg->stutterSpeed * deltaTime` alongside the other accumulators
- In `BindUniforms()`: bind `stutterAmount`, `stutterTime`, `stutterDiscrete`
- In `GlyphFieldRegisterParams()`: register all 3 params with ranges from the Parameters table
- In `preset.cpp`: add `stutterAmount`, `stutterSpeed`, `stutterDiscrete` to the `GlyphFieldConfig` NLOHMANN macro

**Verify**: `cmake.exe --build build` compiles (shader uniforms not yet used — that's fine).

#### Task 1.2: Shader stutter logic

**Files**: `shaders/glyph_field.fs`

**Do**:
- Add 3 uniforms: `uniform float stutterAmount;`, `uniform float stutterTime;`, `uniform float stutterDiscrete;`
- In each of the 3 scroll direction branches (horizontal, vertical, radial), implement the Algorithm section:
  - Compute stutter mask from the lane index (`cellCoord.y` for horizontal, `cellCoord.x` for vertical, `ringIdx` for radial)
  - Multiply `scrollOffset` by `mask`
  - After computing the displaced coordinate (e.g., `offsetX`), apply discrete quantization: blend between smooth and `floor(val * gs) / gs`
- Follow existing code style — use `pcg3df` with a unique seed constant (e.g., 91.3) distinct from other hash calls

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: UI (depends on Wave 1 for config fields)

#### Task 2.1: Stutter UI controls

**Files**: `src/ui/imgui_effects_gen_texture.cpp`

**Do**:
- In `DrawGeneratorsGlyphField()`, add a "Stutter" section after the "Scroll" section (after line 368) and before "Motion" (line 371)
- Add 3 `ModulatableSlider` calls matching the Parameters table labels and formats
- Use `"%.2f"` format for all 3 sliders
- Follow the existing `##glyphfield` suffix convention for ImGui IDs

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `stutterAmount = 0.0` produces identical output to current glyph field (backward compatible)
- [ ] All 3 scroll directions (horizontal, vertical, radial) exhibit stutter behavior
- [ ] `stutterDiscrete = 1.0` produces visible cell-snapping
- [ ] Presets serialize and deserialize the 3 new fields
