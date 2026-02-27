# Glyph Field Rework

Clean up the glyph field generator: remove warp and LCD features, fix band distortion to match the reference's step-based UV scaling, rename flutter→char, and reorganize UI sections.

**Research**: `docs/research/glyph-field-rework.md`

## Design

### Types

#### GlyphFieldConfig (modified)

Remove these fields entirely:
- `waveAmplitude`, `waveFreq`, `waveSpeed` (wave distortion)
- `lcdMode`, `lcdFreq` (LCD sub-pixel)
- `bandDistortion` (replaced by `bandStrength`)
- `flutterAmount`, `flutterSpeed` (renamed to char*)

Add/rename:
- `bandStrength` float, default `0.3f`, range 0.0–1.0, comment `// Step-based edge compression (0.0-1.0)`
- `charAmount` float, default `0.3f`, range 0.0–1.0, comment `// Per-cell character cycling intensity (0.0-1.0)`
- `charSpeed` float, default `2.0f`, range 0.1–10.0, comment `// Character cycling rate (0.1-10.0)`

Field order in struct (by UI section):
1. `enabled`
2. Audio: `baseFreq`, `maxFreq`, `freqBins`, `gain`, `curve`, `baseBright`
3. Grid: `gridSize`, `layerCount`, `layerScaleSpread`, `layerSpeedSpread`, `layerOpacity`, `bandStrength`
4. Scroll: `scrollDirection`, `scrollSpeed`
5. Stutter: `stutterAmount`, `stutterSpeed`, `stutterDiscrete`
6. Character: `charAmount`, `charSpeed`, `inversionRate`, `inversionSpeed`
7. Drift: `driftAmount`, `driftSpeed`
8. Color: `gradient`
9. Blend: `blendMode`, `blendIntensity`

#### GlyphFieldEffect (modified)

Remove:
- `waveTime` accumulator
- `waveAmplitudeLoc`, `waveFreqLoc`, `waveTimeLoc`
- `lcdModeLoc`, `lcdFreqLoc`
- `bandDistortionLoc`
- `flutterAmountLoc`, `flutterTimeLoc`

Rename:
- `flutterTime` → `charTime`

Add:
- `bandStrengthLoc`
- `charAmountLoc`, `charTimeLoc`

#### GLYPH_FIELD_CONFIG_FIELDS macro

Updated to match new struct fields (remove wave/lcd/bandDistortion/flutter, add bandStrength/charAmount/charSpeed).

### Algorithm

#### Shader: Band Strength (replaces bandDistortion)

Remove the continuous linear scaling:
```glsl
// DELETE:
float distFromCenter = abs(wUV.y);
float bandScale = 1.0 + distFromCenter * bandDistortion * 2.0;
vec2 gridUV = wUV;
gridUV.y = wUV.y * bandScale;
```

Replace with step-based UV scaling from the reference, applied before grid quantization. This goes right after the layer's UV is set up (after the `wUV` assignment), replacing the wave displacement AND band distortion blocks:

```glsl
// Step-based band compression (reference: 3 threshold multipliers)
vec2 gridUV = uv;
gridUV *= 1.0 + bandStrength * step(0.5, abs(gridUV.y));
gridUV *= 1.0 + bandStrength * step(0.375, abs(gridUV.y));
gridUV *= 1.0 + bandStrength * step(0.125, abs(gridUV.y));
```

Note: `gridUV` starts from `uv` (centered coords). Layer scale is applied through `gs = gridSize * scale` during grid quantization — same as the current code. The `gridUV` variable replaces both the wave-displaced `wUV` and the old `bandScale` computation.

#### Shader: Rename flutter → char

All occurrences:
- `uniform float flutterAmount` → `uniform float charAmount`
- `uniform float flutterTime` → `uniform float charTime`
- `float flutterStep = floor(flutterTime + ...)` → `float charStep = floor(charTime + ...)`
- `... * flutterAmount` → `... * charAmount`

#### Shader: Remove wave displacement

Delete:
```glsl
// Wave
uniform float waveAmplitude;
uniform float waveFreq;
uniform float waveTime;
```

Delete the wave displacement block inside the layer loop:
```glsl
vec2 wUV = uv;
wUV.x += sin(uv.y * waveFreq + waveTime + layerF * 1.7) * waveAmplitude;
wUV.y += cos(uv.x * waveFreq + waveTime + layerF * 2.3) * waveAmplitude;
```

#### Shader: Remove LCD mode

Delete:
```glsl
uniform int lcdMode;
uniform float lcdFreq;
```

Delete the LCD block after the layer loop:
```glsl
if (lcdMode != 0) {
    vec2 pixelCoord = fragTexCoord * resolution;
    total *= max(sin(pixelCoord.x * lcdFreq + vec3(0.0, 2.0, 4.0))
               * sin(pixelCoord.y * lcdFreq), 0.0) * 3.0;
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `bandStrength` | float | 0.0–1.0 | 0.3 | Yes | `"Band Strength##glyphfield"` |
| `charAmount` | float | 0.0–1.0 | 0.3 | Yes | `"Char Cycle##glyphfield"` |
| `charSpeed` | float | 0.1–10.0 | 2.0 | Yes | `"Char Speed##glyphfield"` |

All other parameters unchanged (just reorganized into new UI sections).

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Update GlyphFieldConfig and GlyphFieldEffect structs

**Files**: `src/effects/glyph_field.h`
**Creates**: Updated struct layouts that Wave 2 depends on

**Do**:
1. In `GlyphFieldConfig`:
   - Delete fields: `flutterAmount`, `flutterSpeed`, `waveAmplitude`, `waveFreq`, `waveSpeed`, `lcdMode`, `lcdFreq`, `bandDistortion`
   - Add `bandStrength = 0.3f` after `layerOpacity` with comment `// Step-based edge compression (0.0-1.0)`
   - Add `charAmount = 0.3f` with comment `// Per-cell character cycling intensity (0.0-1.0)`
   - Add `charSpeed = 2.0f` with comment `// Character cycling rate (0.1-10.0)`
   - Place `charAmount`, `charSpeed` before `inversionRate` (Character section groups char + inversion)
   - Reorder fields to match the section layout in the Design section above
   - Update top-of-file comment: remove "LCD sub-pixel overlay"

2. In `GlyphFieldEffect`:
   - Delete: `waveTime`, `waveAmplitudeLoc`, `waveFreqLoc`, `waveTimeLoc`, `lcdModeLoc`, `lcdFreqLoc`, `bandDistortionLoc`, `flutterAmountLoc`, `flutterTimeLoc`
   - Rename `flutterTime` → `charTime`
   - Add: `bandStrengthLoc`, `charAmountLoc`, `charTimeLoc`

3. Update `GLYPH_FIELD_CONFIG_FIELDS` macro:
   - Remove: `flutterAmount`, `flutterSpeed`, `waveAmplitude`, `waveFreq`, `waveSpeed`, `lcdMode`, `lcdFreq`, `bandDistortion`
   - Add: `bandStrength`, `charAmount`, `charSpeed`
   - Field order should match struct field order

**Verify**: `cmake.exe --build build` compiles (will have errors in .cpp and .fs — that's expected, they're fixed in Wave 2).

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Update glyph_field.cpp

**Files**: `src/effects/glyph_field.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. In `CacheLocations()`:
   - Delete: `waveAmplitudeLoc`, `waveFreqLoc`, `waveTimeLoc`, `lcdModeLoc`, `lcdFreqLoc`, `bandDistortionLoc`, `flutterAmountLoc`, `flutterTimeLoc`
   - Add: `bandStrengthLoc` → `"bandStrength"`, `charAmountLoc` → `"charAmount"`, `charTimeLoc` → `"charTime"`

2. In `GlyphFieldEffectInit()`:
   - Delete: `e->waveTime = 0.0f;`
   - Rename: `e->flutterTime` → `e->charTime`

3. In `BindUniforms()`:
   - Delete all SetShaderValue lines for: `waveAmplitude`, `waveFreq`, `waveTime`, `lcdMode` (and `lcdModeInt`), `lcdFreq`, `bandDistortion`, `flutterAmount`, `flutterTime`
   - Add SetShaderValue for: `bandStrength` (float), `charAmount` (float), `charTime` (float)

4. In `GlyphFieldEffectSetup()`:
   - Delete: `e->waveTime += cfg->waveSpeed * deltaTime;`
   - Rename: `e->flutterTime += cfg->flutterSpeed * deltaTime;` → `e->charTime += cfg->charSpeed * deltaTime;`

5. In `GlyphFieldRegisterParams()`:
   - Delete registrations for: `glyphField.flutterAmount`, `glyphField.flutterSpeed`, `glyphField.waveAmplitude`, `glyphField.waveFreq`, `glyphField.waveSpeed`, `glyphField.lcdFreq`, `glyphField.bandDistortion`
   - Add: `"glyphField.bandStrength"` → `&cfg->bandStrength`, 0.0f, 1.0f
   - Add: `"glyphField.charAmount"` → `&cfg->charAmount`, 0.0f, 1.0f
   - Add: `"glyphField.charSpeed"` → `&cfg->charSpeed`, 0.1f, 10.0f

6. In `DrawGlyphFieldParams()` — reorganize UI sections:
   - **Audio** section: unchanged
   - **Grid** section: add `ModulatableSlider("Band Strength##glyphfield", &c->bandStrength, "glyphField.bandStrength", "%.2f", modSources)` after Layer Opacity
   - **Scroll** section: unchanged
   - **Stutter** section: unchanged
   - **Character** section (new `SeparatorText("Character")`):
     - `ModulatableSlider("Char Cycle##glyphfield", &c->charAmount, "glyphField.charAmount", "%.2f", modSources)`
     - `ModulatableSlider("Char Speed##glyphfield", &c->charSpeed, "glyphField.charSpeed", "%.1f", modSources)`
     - `ModulatableSlider("Inversion##glyphfield", &c->inversionRate, "glyphField.inversionRate", "%.2f", modSources)`
     - `ModulatableSlider("Inversion Speed##glyphfield", &c->inversionSpeed, "glyphField.inversionSpeed", "%.2f", modSources)`
   - **Drift** section (new `SeparatorText("Drift")`):
     - `ModulatableSlider("Drift##glyphfield", &c->driftAmount, "glyphField.driftAmount", "%.3f", modSources)`
     - `ModulatableSlider("Drift Speed##glyphfield", &c->driftSpeed, "glyphField.driftSpeed", "%.2f", modSources)`
   - Delete: entire Motion section (Flutter, Wave Amp, Wave Freq, Wave Speed sliders)
   - Delete: entire Distortion section (Band Distort slider — moved to Grid as Band Strength)
   - Delete: entire LCD section (LCD Mode checkbox, LCD Freq slider)

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

#### Task 2.2: Update glyph_field.fs shader

**Files**: `shaders/glyph_field.fs`
**Depends on**: Wave 1 complete

**Do**:
1. Update top-of-file comment: remove "wave/drift/flutter" and "LCD sub-pixel mode" from the description. Keep stutter and drift.

2. Delete uniform declarations:
   - `uniform float flutterAmount;` and `uniform float flutterTime;`
   - `uniform float waveAmplitude;`, `uniform float waveFreq;`, `uniform float waveTime;`
   - `uniform int lcdMode;` and `uniform float lcdFreq;`
   - `uniform float bandDistortion;`

3. Add uniform declarations:
   - `uniform float bandStrength;` (in a "Band compression" comment group)
   - `uniform float charAmount;` and `uniform float charTime;` (rename the Flutter section comment to "Character cycling")

4. Inside the layer loop, replace the wave displacement + band distortion blocks. Delete:
   ```glsl
   // Wave displacement before grid quantization
   vec2 wUV = uv;
   wUV.x += sin(uv.y * waveFreq + waveTime + layerF * 1.7) * waveAmplitude;
   wUV.y += cos(uv.x * waveFreq + waveTime + layerF * 2.3) * waveAmplitude;

   // Band distortion: rows near center normal, edges compress
   float distFromCenter = abs(wUV.y);
   float bandScale = 1.0 + distFromCenter * bandDistortion * 2.0;
   vec2 gridUV = wUV;
   gridUV.y = wUV.y * bandScale;
   ```

   Replace with:
   ```glsl
   // Step-based band compression (reference: 3 threshold multipliers)
   vec2 gridUV = uv;
   gridUV *= 1.0 + bandStrength * step(0.5, abs(gridUV.y));
   gridUV *= 1.0 + bandStrength * step(0.375, abs(gridUV.y));
   gridUV *= 1.0 + bandStrength * step(0.125, abs(gridUV.y));
   ```

   Note: `gridUV` starts from `uv` (centered coords), not `wUV`. The layer scale (`gs = gridSize * scale`) is applied later during grid quantization — same as current code, where `wUV` was just `uv` + wave offset and the `gs` multiplication happens at `floor(gridUV * gs)`.

5. Rename flutter references inside the loop:
   - `float flutterStep = floor(flutterTime + h.y * 100.0);` → `float charStep = floor(charTime + h.y * 100.0);`
   - `int charIdx = int(mod(baseChar + flutterStep * 37.0 * flutterAmount, 256.0));` → `int charIdx = int(mod(baseChar + charStep * 37.0 * charAmount, 256.0));`

6. Delete the LCD block after the layer loop (the `if (lcdMode != 0) { ... }` block).

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Shader loads — effect renders without black screen
- [ ] Band strength at 0 = flat grid, at 1 = visible stepped compression at edges
- [ ] Character cycling works (char cycle slider drives visible glyph changes)
- [ ] No references to removed params (flutter, wave, lcd, bandDistortion) remain in any file
- [ ] Preset loading with old fields gracefully falls back to defaults (no crash)
