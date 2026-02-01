# Style Subcategories

Split the monolithic Style category into 4 focused subcategories for better organization.

## Specification

### New Categories

| Category | Badge | Section Index | Effects |
|----------|-------|---------------|---------|
| Artistic | ART | 4 | Oil Paint, Watercolor, Impressionist, Ink Wash, Pencil Sketch, Cross-Hatching |
| Graphic | GFX | 5 | Toon, Neon Glow, Kuwahara, Halftone (moved from Color) |
| Retro | RET | 6 | Pixelation, Glitch, ASCII Art, Matrix Rain, LEGO Bricks |
| Optical | OPT | 7 | Bloom, Bokeh, Heightfield Relief |

### Transform Category Mapping

Update `GetTransformCategory()` in `imgui_effects.cpp`:

```cpp
// Artistic - section 4
case TRANSFORM_OIL_PAINT:
case TRANSFORM_WATERCOLOR:
case TRANSFORM_IMPRESSIONIST:
case TRANSFORM_INK_WASH:
case TRANSFORM_PENCIL_SKETCH:
case TRANSFORM_CROSS_HATCHING:
  return {"ART", 4};

// Graphic - section 5
case TRANSFORM_TOON:
case TRANSFORM_NEON_GLOW:
case TRANSFORM_KUWAHARA:
case TRANSFORM_HALFTONE:
  return {"GFX", 5};

// Retro - section 6
case TRANSFORM_PIXELATION:
case TRANSFORM_GLITCH:
case TRANSFORM_ASCII_ART:
case TRANSFORM_MATRIX_RAIN:
case TRANSFORM_LEGO_BRICKS:
  return {"RET", 6};

// Optical - section 7
case TRANSFORM_BLOOM:
case TRANSFORM_BOKEH:
case TRANSFORM_HEIGHTFIELD_RELIEF:
  return {"OPT", 7};
```

### Updated Color Category

Color category (now section 8) loses Halftone:
- Color Grade
- False Color
- Palette Quantization

---

## Tasks

### Wave 1: Create New UI Files

These tasks create new files with no dependencies on each other.

#### Task 1.1: Create imgui_effects_artistic.cpp

**Files**: `src/ui/imgui_effects_artistic.cpp`, `src/ui/imgui_effects_artistic.h`

**Build**:
1. Create header with `void DrawArtisticCategory(EffectConfig *e, const ModSources *modSources);`
2. Create cpp with section booleans for: Oil Paint, Watercolor, Impressionist, Ink Wash, Pencil Sketch, Cross-Hatching
3. Move `DrawStyleOilPaint`, `DrawStyleWatercolor`, `DrawStyleImpressionist`, `DrawStyleInkWash`, `DrawStylePencilSketch`, `DrawStyleCrossHatching` from `imgui_effects_style.cpp`
4. Rename functions to `DrawArtistic*` (e.g., `DrawArtisticOilPaint`)
5. Create `DrawArtisticCategory()` calling `GetSectionGlow(4)` and each effect function

**Verify**: File compiles when included.

#### Task 1.2: Create imgui_effects_graphic.cpp

**Files**: `src/ui/imgui_effects_graphic.cpp`, `src/ui/imgui_effects_graphic.h`

**Build**:
1. Create header with `void DrawGraphicCategory(EffectConfig *e, const ModSources *modSources);`
2. Create cpp with section booleans for: Toon, Neon Glow, Kuwahara, Halftone
3. Move `DrawStyleToon`, `DrawStyleNeonGlow`, `DrawStyleKuwahara` from `imgui_effects_style.cpp`
4. Move `DrawColorHalftone` from `imgui_effects_color.cpp`
5. Rename functions to `DrawGraphic*`
6. Create `DrawGraphicCategory()` calling `GetSectionGlow(5)` and each effect function

**Verify**: File compiles when included.

#### Task 1.3: Create imgui_effects_retro.cpp

**Files**: `src/ui/imgui_effects_retro.cpp`, `src/ui/imgui_effects_retro.h`

**Build**:
1. Create header with `void DrawRetroCategory(EffectConfig *e, const ModSources *modSources);`
2. Create cpp with section booleans for: Pixelation, Glitch, ASCII Art, Matrix Rain, LEGO Bricks
3. Move `DrawStylePixelation`, `DrawStyleGlitch`, `DrawStyleAsciiArt`, `DrawStyleMatrixRain`, `DrawStyleLegoBricks` from `imgui_effects_style.cpp`
4. Rename functions to `DrawRetro*`
5. Create `DrawRetroCategory()` calling `GetSectionGlow(6)` and each effect function

**Verify**: File compiles when included.

#### Task 1.4: Create imgui_effects_optical.cpp

**Files**: `src/ui/imgui_effects_optical.cpp`, `src/ui/imgui_effects_optical.h`

**Build**:
1. Create header with `void DrawOpticalCategory(EffectConfig *e, const ModSources *modSources);`
2. Create cpp with section booleans for: Bloom, Bokeh, Heightfield Relief
3. Move `DrawStyleBokeh`, `DrawStyleBloom`, `DrawStyleHeightfieldRelief` from `imgui_effects_style.cpp`
4. Rename functions to `DrawOptical*`
5. Create `DrawOpticalCategory()` calling `GetSectionGlow(7)` and each effect function

**Verify**: File compiles when included.

---

### Wave 2: Create New Shader Setup Files

These tasks create new files with no dependencies on each other.

#### Task 2.1: Create shader_setup_artistic.cpp

**Files**: `src/render/shader_setup_artistic.cpp`, `src/render/shader_setup_artistic.h`

**Build**:
1. Create header declaring: `SetupOilPaint`, `SetupWatercolor`, `SetupImpressionist`, `SetupInkWash`, `SetupPencilSketch`, `SetupCrossHatching`
2. Move those functions from `shader_setup_style.cpp`
3. Include `post_effect.h` and relevant config headers

**Verify**: File compiles when included.

#### Task 2.2: Create shader_setup_graphic.cpp

**Files**: `src/render/shader_setup_graphic.cpp`, `src/render/shader_setup_graphic.h`

**Build**:
1. Create header declaring: `SetupToon`, `SetupNeonGlow`, `SetupKuwahara`, `SetupHalftone`
2. Move `SetupToon`, `SetupNeonGlow`, `SetupKuwahara` from `shader_setup_style.cpp`
3. Move `SetupHalftone` from `shader_setup_color.cpp`
4. Include `post_effect.h` and relevant config headers

**Verify**: File compiles when included.

#### Task 2.3: Create shader_setup_retro.cpp

**Files**: `src/render/shader_setup_retro.cpp`, `src/render/shader_setup_retro.h`

**Build**:
1. Create header declaring: `SetupPixelation`, `SetupGlitch`, `SetupAsciiArt`, `SetupMatrixRain`, `SetupLegoBricks`
2. Move those functions from `shader_setup_style.cpp`
3. Include `post_effect.h` and relevant config headers

**Verify**: File compiles when included.

#### Task 2.4: Create shader_setup_optical.cpp

**Files**: `src/render/shader_setup_optical.cpp`, `src/render/shader_setup_optical.h`

**Build**:
1. Create header declaring: `SetupBloom`, `SetupBokeh`, `SetupHeightfieldRelief`
2. Move those functions from `shader_setup_style.cpp`
3. Include `post_effect.h` and relevant config headers

**Verify**: File compiles when included.

---

### Wave 3: Update Existing Files

These tasks modify existing files. Must run after Wave 1-2.

#### Task 3.1: Update imgui_effects.cpp

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add includes for new headers: `imgui_effects_artistic.h`, `imgui_effects_graphic.h`, `imgui_effects_retro.h`, `imgui_effects_optical.h`
2. Update `GetTransformCategory()` switch cases per specification above
3. Update Color category switch to section index 8 (was 5)
4. Update Simulation category switch to section index 9 (was 6)
5. Replace `DrawStyleCategory(e, modSources)` call with:
   ```cpp
   DrawArtisticCategory(e, modSources);
   ImGui::Spacing();
   DrawGraphicCategory(e, modSources);
   ImGui::Spacing();
   DrawRetroCategory(e, modSources);
   ImGui::Spacing();
   DrawOpticalCategory(e, modSources);
   ```

**Verify**: Compiles.

#### Task 3.2: Update imgui_effects_color.cpp

**Files**: `src/ui/imgui_effects_color.cpp`
**Depends on**: Task 1.2 complete

**Build**:
1. Remove `sectionHalftone` static bool
2. Remove `DrawColorHalftone` function
3. Remove Halftone call from `DrawColorCategory`
4. Update `GetSectionGlow(5)` to `GetSectionGlow(8)` in `DrawColorCategory`

**Verify**: Compiles.

#### Task 3.3: Delete imgui_effects_style.cpp

**Files**: `src/ui/imgui_effects_style.cpp`
**Depends on**: Wave 1 complete, Task 3.1 complete

**Build**:
1. Delete the file (all functions moved to new category files)

**Verify**: Project compiles without it.

#### Task 3.4: Update shader_setup.cpp

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Replace `#include "shader_setup_style.h"` with includes for new headers
2. Remove `#include "shader_setup_color.h"` Halftone dispatch (if present)
3. Update dispatch calls to use new headers (no functional change, just includes)

**Verify**: Compiles.

#### Task 3.5: Update shader_setup_color.cpp

**Files**: `src/render/shader_setup_color.cpp`
**Depends on**: Task 2.2 complete

**Build**:
1. Remove `SetupHalftone` function (moved to shader_setup_graphic.cpp)

**Verify**: Compiles.

#### Task 3.6: Delete shader_setup_style.cpp

**Files**: `src/render/shader_setup_style.cpp`, `src/render/shader_setup_style.h`
**Depends on**: Wave 2 complete, Task 3.4 complete

**Build**:
1. Delete both files (all functions moved to new category files)

**Verify**: Project compiles without them.

#### Task 3.7: Update CMakeLists.txt

**Files**: `CMakeLists.txt`
**Depends on**: Task 3.3, Task 3.6 complete

**Build**:
1. In IMGUI_UI_SOURCES: Remove `imgui_effects_style.cpp`, add `imgui_effects_artistic.cpp`, `imgui_effects_graphic.cpp`, `imgui_effects_retro.cpp`, `imgui_effects_optical.cpp`
2. In RENDER_SOURCES: Remove `shader_setup_style.cpp`, add `shader_setup_artistic.cpp`, `shader_setup_graphic.cpp`, `shader_setup_retro.cpp`, `shader_setup_optical.cpp`

**Verify**: `cmake.exe -G Ninja -B build -S .` succeeds.

---

### Wave 4: Documentation

#### Task 4.1: Update docs/effects.md

**Files**: `docs/effects.md`
**Depends on**: Wave 3 complete

**Build**:
1. Replace `### Style` section with 4 new subsections under TRANSFORMS:
   - `### Artistic` (Oil Paint, Watercolor, Impressionist, Ink Wash, Pencil Sketch, Cross-Hatching)
   - `### Graphic` (Toon, Neon Glow, Kuwahara, Halftone)
   - `### Retro` (Pixelation, Glitch, ASCII Art, Matrix Rain, LEGO Bricks)
   - `### Optical` (Bloom, Bokeh, Heightfield Relief)
2. Remove Halftone from `### Color` section

**Verify**: Documentation reflects new organization.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S .` succeeds
- [ ] `cmake.exe --build build` compiles with no errors
- [ ] Effects panel shows 4 new category headers instead of "Style"
- [ ] Pipeline list shows correct badges (ART/GFX/RET/OPT/COL)
- [ ] Halftone appears under Graphic, not Color
- [ ] All 17 former Style effects are still functional
