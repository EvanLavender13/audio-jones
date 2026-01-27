# Glitch Enhancements

Extend the existing Glitch effect with 6 new techniques: variable resolution pixelation (Datamosh), row/column displacement (Slice), diagonal UV banding, radial temporal jitter, and block color masking.

## Current State

- `src/config/glitch_config.h` - 14 parameters across CRT, Analog, Digital, VHS, Overlay
- `shaders/glitch.fs:1-184` - Staged pipeline: CRT+VHS UV distort → Analog+Digital chromatic → Overlay
- `src/ui/imgui_effects_style.cpp:48-99` - TreeNodeAccented subcategories
- `src/render/post_effect.h:193-208` - 16 uniform locations
- `src/automation/param_registry.cpp:122-125,382-385` - 4 modulatable params

## Technical Implementation

### Hash Functions

Add to `glitch.fs` after existing `hash33()`:

```glsl
// Single float hash (for seeds)
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

// Integer hash (for block indices)
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
```

### Technique 1: Datamosh (Variable Resolution Pixelation)

**Stage**: First, before all other UV distortions

```glsl
uniform bool datamoshEnabled;
uniform float datamoshMin;      // 4-32, default 6.0
uniform float datamoshMax;      // 16-128, default 64.0
uniform float datamoshSpeed;    // 1-30, default 6.0
uniform float datamoshBands;    // 1-32, default 8.0

if (datamoshEnabled) {
    uint offset = uint(float(frame) / datamoshSpeed) + uint((uv.x + uv.y) * datamoshBands);
    float res = mix(datamoshMin, datamoshMax, hash11(float(offset)));
    uv = floor(uv * res) / res;
}
```

**Key insight**: `(uv.x + uv.y) * datamoshBands` creates diagonal bands—pixels along the same diagonal share the same random seed.

### Technique 2: Row Slice (Horizontal Displacement)

**Stage**: After Datamosh, before CRT

```glsl
uniform bool rowSliceEnabled;
uniform float rowSliceIntensity;  // 0-0.5, default 0.1
uniform float rowSliceBurstFreq;  // 0.5-20, default 4.0
uniform float rowSliceBurstPower; // 1-15, default 7.0
uniform float rowSliceColumns;    // 8-128, default 32.0

if (rowSliceEnabled) {
    uint seed = uint(gl_FragCoord.x + time * 60.0) / uint(rowSliceColumns);
    float gate = step(hash11(float(seed)), pow(abs(sin(time * rowSliceBurstFreq)), rowSliceBurstPower));
    float offset = (hash11(float(seed + 1u)) * 2.0 - 1.0) * gate * rowSliceIntensity;
    uv.x += offset;
}
```

**Key insight**: `pow(sin(time), 7.0)` creates narrow burst windows—displacement only triggers ~14% of the time.

### Technique 3: Column Slice (Vertical Displacement)

**Stage**: After Row Slice, before CRT

```glsl
uniform bool colSliceEnabled;
uniform float colSliceIntensity;  // 0-0.5, default 0.1
uniform float colSliceBurstFreq;  // 0.5-20, default 4.0
uniform float colSliceBurstPower; // 1-15, default 7.0
uniform float colSliceRows;       // 8-128, default 32.0

if (colSliceEnabled) {
    uint seed = uint(gl_FragCoord.y + time * 60.0) / uint(colSliceRows);
    float gate = step(hash11(float(seed)), pow(abs(sin(time * colSliceBurstFreq)), colSliceBurstPower));
    float offset = (hash11(float(seed + 1u)) * 2.0 - 1.0) * gate * colSliceIntensity;
    uv.y += offset;
}
```

### Technique 4: Diagonal Bands

**Stage**: After Datamosh, before Slice

```glsl
uniform bool diagonalBandsEnabled;
uniform float diagonalBandCount;    // 2-32, default 8.0
uniform float diagonalBandDisplace; // 0-0.1, default 0.02
uniform float diagonalBandSpeed;    // 0-10, default 1.0

if (diagonalBandsEnabled) {
    float diagonal = uv.x + uv.y;
    float band = floor(diagonal * diagonalBandCount);
    float bandOffset = hash11(band + floor(time * diagonalBandSpeed)) * 2.0 - 1.0;
    uv += bandOffset * diagonalBandDisplace;
}
```

**Key insight**: `x + y = c` defines a diagonal line. All pixels on that line share the same `c` value.

### Technique 5: Block Mask (Color Tinting)

**Stage**: After Digital block displacement, before Overlay

```glsl
uniform bool blockMaskEnabled;
uniform float blockMaskIntensity; // 0-1, default 0.5
uniform int blockMaskMinSize;     // 1-10, default 1
uniform int blockMaskMaxSize;     // 5-20, default 10
uniform vec3 blockMaskTint;       // default (1.0, 0.0, 0.5)

if (blockMaskEnabled) {
    int gridSize = (frame % (blockMaskMaxSize - blockMaskMinSize + 1)) + blockMaskMinSize;
    ivec2 blockCoord = ivec2(gl_FragCoord.xy) / gridSize;
    int index = blockCoord.y * int(resolution.x / float(gridSize)) + blockCoord.x;

    uint blockHash = hash(uint(index + frame / 6));
    int pattern = int(blockHash % 7u) + 2;
    float mask = (index % pattern == 0) ? 1.0 : 0.0;

    col = mix(col, col * blockMaskTint, mask * blockMaskIntensity);
}
```

### Technique 6: Radial Temporal Jitter

**Stage**: Last, after Block Mask

```glsl
uniform bool temporalJitterEnabled;
uniform float temporalJitterAmount; // 0-0.1, default 0.02
uniform float temporalJitterGate;   // 0-1, default 0.3

if (temporalJitterEnabled) {
    vec2 coord = gl_FragCoord.xy;
    float radial = (coord.x * coord.y) / (resolution.x * resolution.y * 0.25);
    float jitterSeed = hash11(radial + floor(time * 10.0));
    float gate = step(jitterSeed, temporalJitterGate);

    vec2 jitterOffset = vec2(hash11(radial), hash11(radial + 1.0)) * 2.0 - 1.0;
    uv += jitterOffset * gate * temporalJitterAmount;
}
```

**Key insight**: `coord.x * coord.y` equals zero along axes and grows toward corners, creating radial zones.

**Note**: True temporal jitter requires frame history. This approximation creates spatial jitter that visually resembles temporal discontinuity.

### Shader Execution Order

```
1. Datamosh        (resolution reduction first)
2. Diagonal Bands  (UV displacement)
3. Row Slice       (horizontal UV displacement)
4. Column Slice    (vertical UV displacement)
5. CRT             (existing barrel distortion)
6. VHS             (existing tracking bars)
7. Analog          (existing noise + chromatic)
8. Digital         (existing block displacement)
9. Block Mask      (color modification)
10. Temporal Jitter (spatial jitter)
11. Overlay        (existing scanlines + noise)
```

---

## Phase 1: Config + Hash Functions

**Goal**: Extend GlitchConfig with all new parameters and add hash functions to shader.
**Depends on**: —
**Files**: `src/config/glitch_config.h`, `shaders/glitch.fs`

**Build**:
- Add 25 new fields to `GlitchConfig`: Datamosh (4), Row Slice (4), Column Slice (4), Diagonal Bands (4), Block Mask (5), Temporal Jitter (3), plus bool enables
- Add `hash11()` and `hash()` functions to `glitch.fs` after `hash33()`

**Verify**: `cmake.exe --build build` compiles without errors.

**Done when**: Config struct has all fields with defaults, shader has both hash functions.

---

## Phase 2: Uniform Locations

**Goal**: Register all new shader uniform locations.
**Depends on**: Phase 1
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/render/shader_setup.cpp`

**Build**:
- Add ~25 `int glitch*Loc` fields to `PostEffect` struct
- Add `GetShaderLocation()` calls in `GetShaderUniformLocations()`
- Extend `SetupGlitch()` to set all new uniforms

**Verify**: `cmake.exe --build build` compiles. Run app, enable Glitch—no shader errors in console.

**Done when**: All uniform locations registered and SetupGlitch binds them.

---

## Phase 3: Datamosh Shader + UI

**Goal**: Implement Datamosh technique end-to-end.
**Depends on**: Phase 2
**Files**: `shaders/glitch.fs`, `src/ui/imgui_effects_style.cpp`

**Build**:
- Add Datamosh shader code at start of `main()`, before CRT
- Add Datamosh `TreeNodeAccented` section after VHS in UI
- Use `ModulatableSlider` for min/max, regular sliders for speed/bands

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Enable Glitch, expand Datamosh, toggle enabled. Screen shows flickering diagonal resolution bands.

**Done when**: Datamosh creates visible blocky diagonal artifacts.

---

## Phase 4: Slice Techniques Shader + UI

**Goal**: Implement Row and Column Slice techniques.
**Depends on**: Phase 2
**Files**: `shaders/glitch.fs`, `src/ui/imgui_effects_style.cpp`

**Build**:
- Add Row Slice shader code after Datamosh/Bands, before CRT
- Add Column Slice shader code after Row Slice
- Add "Slice" `TreeNodeAccented` section with nested Row/Column subsections
- Use `ModulatableSlider` for intensity, regular sliders for burst params

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Enable Slice effects. Sporadic horizontal/vertical displacement bursts appear.

**Done when**: Row and Column Slice create visible displacement bursts.

---

## Phase 5: Diagonal Bands Shader + UI

**Goal**: Implement Diagonal Bands technique.
**Depends on**: Phase 2
**Files**: `shaders/glitch.fs`, `src/ui/imgui_effects_style.cpp`

**Build**:
- Add Diagonal Bands shader code after Datamosh, before Slice
- Add "Diagonal Bands" `TreeNodeAccented` section
- Use `ModulatableSlider` for displace, regular sliders for count/speed

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Enable Diagonal Bands. Visible diagonal stripe displacement pattern.

**Done when**: Diagonal bands create visible 45-degree stripe artifacts.

---

## Phase 6: Block Mask Shader + UI

**Goal**: Implement Block Mask technique.
**Depends on**: Phase 2
**Files**: `shaders/glitch.fs`, `src/ui/imgui_effects_style.cpp`

**Build**:
- Add Block Mask shader code after Digital block displacement, before Overlay
- Add Block Mask controls inside existing "Digital" `TreeNodeAccented` section
- Use `ModulatableSlider` for intensity, `ColorEdit3` for tint

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Enable Digital section, enable Block Mask. Random colored blocks appear.

**Done when**: Block Mask tints random rectangular regions.

---

## Phase 7: Temporal Jitter Shader + UI

**Goal**: Implement Radial Temporal Jitter technique.
**Depends on**: Phase 2
**Files**: `shaders/glitch.fs`, `src/ui/imgui_effects_style.cpp`

**Build**:
- Add Temporal Jitter shader code after Block Mask, before Overlay
- Add "Temporal" `TreeNodeAccented` section at end (before Overlay separator)
- Use `ModulatableSlider` for amount, regular slider for gate

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Enable Temporal. Radial zones show spatial jitter.

**Done when**: Temporal Jitter creates visible radial displacement zones.

---

## Phase 8: Serialization + Param Registration

**Goal**: Enable preset save/load and modulation for key parameters.
**Depends on**: Phase 3, Phase 4, Phase 5, Phase 6, Phase 7
**Files**: `src/config/preset.cpp`, `src/automation/param_registry.cpp`

**Build**:
- Extend `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GlitchConfig, ...)` with all new fields
- Add to `PARAM_TABLE` (8 entries):
  - `glitch.datamoshMin`, `glitch.datamoshMax`
  - `glitch.rowSliceIntensity`, `glitch.colSliceIntensity`
  - `glitch.diagonalBandDisplace`
  - `glitch.blockMaskIntensity`
  - `glitch.temporalJitterAmount`, `glitch.temporalJitterGate`
- Add corresponding pointers to targets array at matching indices

**Verify**:
1. Save preset with new effects enabled, reload—settings persist
2. Route LFO to `glitch.datamoshMin`—value oscillates

**Done when**: Presets preserve settings, modulation routes work.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1 | — |
| 2 | Phase 2 | Wave 1 |
| 3 | Phase 3, Phase 4, Phase 5, Phase 6, Phase 7 | Wave 2 |
| 4 | Phase 8 | Wave 3 |

Phases 3-7 execute as parallel subagents (different shader sections, different UI sections, no file conflicts within shader since each touches distinct code blocks).

---

## Post-Implementation Notes

Changes made after testing that extend beyond the original plan:

### Added: `datamoshIntensity` parameter (2026-01-27)

**Reason**: Original datamosh was all-or-nothing (on/off). User feedback indicated need for smooth blend control between original and pixelated UVs to match reference shader behavior.

**Changes**:
- `src/config/glitch_config.h`: Added `datamoshIntensity` field (0-1, default 1.0)
- `shaders/glitch.fs`: Added uniform, changed `uv = floor(...)` to `uv = mix(uv, pixelatedUv, datamoshIntensity)`
- `src/render/post_effect.h`: Added `glitchDatamoshIntensityLoc`
- `src/render/post_effect.cpp`: Added GetShaderLocation call
- `src/render/shader_setup.cpp`: Added SetShaderValue call
- `src/config/preset.cpp`: Added to serialization macro
- `src/automation/param_registry.cpp`: Added to PARAM_TABLE and targets array
- `src/ui/imgui_effects_style.cpp`: Added ModulatableSlider
