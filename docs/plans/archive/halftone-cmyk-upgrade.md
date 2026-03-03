# Halftone CMYK Upgrade

Upgrade the existing halftone from a single luminance-based dot grid to a four-color CMYK halftone. Each ink channel (Cyan, Magenta, Yellow, Key/Black) gets its own dot grid at a different screen angle, composited subtractively on white paper. The result looks like magnified newsprint.

**Research**: `docs/research/halftone-cmyk-upgrade.md`

## Design

### Types

**HalftoneConfig** (unchanged fields and ranges):
```
bool enabled = false;
float dotScale = 8.0f;      // Grid cell size in pixels (2.0-20.0)
float dotSize = 1.0f;        // Ink density multiplier (0.5-2.0)
float rotationSpeed = 0.0f;  // Rotation rate in radians/second
float rotationAngle = 0.0f;  // Static rotation offset in radians
```

No struct field changes. `CONFIG_FIELDS` macro unchanged.

**HalftoneEffect** (unchanged):
```
Shader shader;
int resolutionLoc;
int dotScaleLoc;
int dotSizeLoc;
int rotationLoc;
float rotation;
```

No new uniform locations needed — `dotScale`, `dotSize`, and `rotation` are the same uniforms with the same semantics. The shader uses them differently internally but the C++ interface is identical.

### Algorithm

The shader replaces the single-grid luminance approach with four overlapping CMYK dot grids composited subtractively.

#### Rotation helper (keep existing)

```glsl
mat2 rotm(float r) {
    float c = cos(r), s = sin(r);
    return mat2(c, -s, s, c);
}
```

#### CMYK screen angle constants

```glsl
const float ANGLE_C = 0.2618;  // 15 degrees
const float ANGLE_M = 1.3090;  // 75 degrees
const float ANGLE_Y = 0.0;     // 0 degrees
const float ANGLE_K = 0.7854;  // 45 degrees
```

#### Dot grid function

```glsl
float cmykDot(vec2 fc, float angle, float cellSize) {
    mat2 m = rotm(angle);
    vec2 rotated = m * fc;
    vec2 cellLocal = fract(rotated / cellSize) - 0.5;
    return 1.7 - length(cellLocal) * 4.0;
}
```

Takes pixel coordinates, rotates by the channel's screen angle (plus the global `rotation` uniform), tiles into cells, returns a falloff value centered at 1.7 (cell center) falling toward negative (cell edge). The `1.7` and `4.0` constants match the reference code's falloff curve.

#### Cell-center sampling (for texture lookup)

Sample once using the K-channel grid (45° — the dominant visual channel):

```glsl
mat2 mk = rotm(rotation + ANGLE_K);
vec2 rotK = mk * fc;
vec2 cellK = floor(rotK / dotScale) * dotScale + dotScale * 0.5;
vec2 centerK = transpose(mk) * cellK;
vec3 texColor = texture(texture0, centerK / resolution).rgb;
```

This reuses the existing halftone's cell-center sampling pattern (rotate → snap → unrotate). One sample for all four channels avoids expense and misalignment.

#### RGB → CMYK conversion

```glsl
float K = 1.0 - max(texColor.r, max(texColor.g, texColor.b));
float invK = 1.0 - K;
float C = invK > 0.001 ? (invK - texColor.r) / invK : 0.0;
float M = invK > 0.001 ? (invK - texColor.g) / invK : 0.0;
float Y = invK > 0.001 ? (invK - texColor.b) / invK : 0.0;
```

Division-by-zero guard: when K ≈ 1 (pure black), CMY = 0.

#### Ink presence test (per channel)

Hard threshold — sharp binary dots like newsprint:

```glsl
float cInk = step(0.5, C * dotSize + cmykDot(fc, rotation + ANGLE_C, dotScale));
float mInk = step(0.5, M * dotSize + cmykDot(fc, rotation + ANGLE_M, dotScale));
float yInk = step(0.5, Y * dotSize + cmykDot(fc, rotation + ANGLE_Y, dotScale));
float kInk = step(0.5, K * dotSize + cmykDot(fc, rotation + ANGLE_K, dotScale));
```

When ink density is 0, only the very center of dots passes threshold (tiny dots). When density is 1, most of the cell fills with ink (large dots). `dotSize` scales the density contribution.

#### Subtractive compositing

```glsl
float r = (1.0 - cInk) * (1.0 - kInk);   // Cyan absorbs red
float g = (1.0 - mInk) * (1.0 - kInk);   // Magenta absorbs green
float b = (1.0 - yInk) * (1.0 - kInk);   // Yellow absorbs blue
finalColor = vec4(r, g, b, 1.0);
```

White paper (1,1,1) minus absorbed ink channels.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| dotScale | float | 2.0-20.0 | 8.0 | Yes (existing) | Dot Scale |
| dotSize | float | 0.5-2.0 | 1.0 | Yes (add) | Dot Size |
| rotationSpeed | float | -PI..PI | 0.0 | Yes (existing) | Spin |
| rotationAngle | float | -PI..PI | 0.0 | Yes (existing) | Angle |

### Constants

- Enum: `TRANSFORM_HALFTONE` (existing, unchanged)
- Display name: `"Halftone"` (unchanged)
- Category: `"PRT"`, section 5 (unchanged)

---

## Tasks

### Wave 1: [Header + Shader + Implementation — No File Overlap]

Since the `.h`, `.fs`, and `.cpp` have no file overlap and the types are unchanged, all three can run in parallel.

#### Task 1.1: Update header comment

**Files**: `src/effects/halftone.h`

**Do**:
1. Change the top-of-file comment from `"Luminance-based dot pattern simulating print halftoning"` to `"Four-color CMYK dot pattern simulating print halftoning"`.
2. Update `dotSize` field comment from `// Dot radius multiplier within cell (0.5-2.0)` to `// Ink density multiplier (0.5-2.0)` — the parameter is repurposed from radius scaling to density scaling in the CMYK shader.

No struct layout, field name, or API changes.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: Rewrite shader to CMYK

**Files**: `shaders/halftone.fs`

**Do**: Replace the shader body with the CMYK algorithm from the Design section above. Keep the existing `#version 330`, `in`/`out`/`uniform` declarations, and the `rotm()` helper. Replace `main()` entirely. Add the four `ANGLE_*` constants and the `cmykDot()` function. Follow the Design section's Algorithm exactly — it contains all formulas and GLSL.

The header comment should read: `// Halftone: four-color CMYK dot pattern simulating print halftoning`

No attribution comment needed (reference code has no attribution requirement per research doc).

**Verify**: File has valid GLSL syntax. Uniforms `resolution`, `dotScale`, `dotSize`, `rotation` are all still referenced.

---

#### Task 1.3: Update .cpp — register dotSize for modulation + update UI label

**Files**: `src/effects/halftone.cpp`

**Do**:
1. In `HalftoneRegisterParams()`: add `ModEngineRegisterParam("halftone.dotSize", &cfg->dotSize, 0.5f, 2.0f);`
2. In `DrawHalftoneParams()`: change the `dotSize` slider from `ImGui::SliderFloat` to `ModulatableSlider("Dot Size##halftone", &ht->dotSize, "halftone.dotSize", "%.2f", ms);`
3. Update top-of-file comment from `"Halftone effect module implementation"` to `"Halftone (CMYK) effect module implementation"`

No other changes — Setup, Init, Uninit, bridge function, registration macro all stay the same.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Halftone effect still appears in Print category with "PRT" badge
- [ ] Enabling halftone shows colored CMYK dots instead of grayscale dots
- [ ] `dotScale` changes grid density
- [ ] `dotSize` changes ink coverage (modulatable)
- [ ] `rotationSpeed` / `rotationAngle` rotate the entire dot grid ensemble
- [ ] Grayscale input produces mostly K-channel dots (similar to old behavior)
- [ ] Preset save/load still works (same config fields)
