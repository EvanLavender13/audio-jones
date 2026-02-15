# Bit Crush

Chunky pixel mosaics that constantly reorganize — an iterative lattice walk snaps pixels to integer grid cells, creating solid color blocks with hard edges that flicker and remap over time. Generates structure from nothing (no input texture). Gradient LUT for hue, FFT energy for brightness.

**Research**: `docs/research/bit-crush.md`

## Design

### Types

**`BitCrushConfig`** (in `src/effects/bit_crush.h`):

```
bool enabled = false

// FFT mapping
float baseFreq = 55.0f      // 27.5-440.0
float maxFreq = 14000.0f    // 1000-16000
float gain = 2.0f           // 0.1-10.0
float curve = 1.0f          // 0.1-3.0
float baseBright = 0.05f    // 0.0-1.0

// Lattice
float scale = 0.3f          // 0.05-1.0
float cellSize = 8.0f       // 2.0-32.0
int iterations = 32         // 4-64
float speed = 1.0f          // 0.1-5.0

// Glow
float glowIntensity = 1.0f  // 0.0-3.0

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}

// Blend compositing
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**`BitCrushEffect`** (in `src/effects/bit_crush.h`):

```
Shader shader
ColorLUT *gradientLUT
float time               // accumulated animation time

// Uniform locations
int resolutionLoc, centerLoc
int fftTextureLoc, sampleRateLoc
int baseFreqLoc, maxFreqLoc, gainLoc, curveLoc, baseBrightLoc
int scaleLoc, cellSizeLoc, iterationsLoc, timeLoc
int glowIntensityLoc, gradientLUTLoc
```

### Algorithm

The shader implements a lattice walk from the reference Shadertoy code. The core algorithm is preserved verbatim from the research doc.

**Hash function:**
```glsl
float r(vec2 p, float time) {
    return cos(time * cos(p.x * p.y));
}
```

**Main body — iterative lattice walk:**
```glsl
// Center-relative, resolution-scaled coordinates
vec2 p = (fragTexCoord - center) * resolution * scale;

// 32-step lattice walk (iterations uniform)
for (int i = 0; i < iterations; i++) {
    vec2 ceilCell = ceil(p / cellSize);
    vec2 floorCell = floor(p / cellSize);
    p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
}
```

**Color via gradient LUT:**
```glsl
float t = fract(dot(p, vec2(0.1, 0.13)));
vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
```

**FFT brightness modulation per cell:**
- Derive `t` from final position (same value used for color)
- Map `t` through `baseFreq → maxFreq` in log space: `freq = baseFreq * pow(maxFreq / baseFreq, t)`
- Convert to FFT bin: `bin = freq / (sampleRate * 0.5)`
- Sample: `energy = texture(fftTexture, vec2(bin, 0.5)).r`
- Apply gain/curve: `energy = pow(clamp(energy * gain, 0.0, 1.0), curve)`
- Final brightness: `brightness = baseBright + energy * glowIntensity`
- Multiply: `finalColor = vec4(color * brightness, 1.0)`

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| scale | float | 0.05–1.0 | 0.3 | Yes | Scale |
| cellSize | float | 2.0–32.0 | 8.0 | Yes | Cell Size |
| iterations | int | 4–64 | 32 | No | Iterations |
| speed | float | 0.1–5.0 | 1.0 | Yes | Speed |
| glowIntensity | float | 0.0–3.0 | 1.0 | Yes | Glow Intensity |
| baseFreq | float | 27.5–440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000–16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1–10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1–3.0 | 1.0 | Yes | Contrast |
| baseBright | float | 0.0–1.0 | 0.05 | Yes | Base Bright |
| blendIntensity | float | 0.0–5.0 | 1.0 | Yes | Blend Intensity |
| blendMode | enum | — | SCREEN | No | Blend Mode |

### Constants

- Enum: `TRANSFORM_BIT_CRUSH_BLEND`
- Display name: `"Bit Crush Blend"`
- Category: `"GEN"`, section 10
- Field name: `bitCrush` (on EffectConfig and PostEffect)
- Macro: `REGISTER_GENERATOR`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create BitCrushConfig and BitCrushEffect types

**Files**: `src/effects/bit_crush.h` (create)
**Creates**: Config struct, Effect struct, function declarations

**Do**: Create header following `motherboard.h` as pattern. Define `BitCrushConfig` with fields from the Design section. Define `BitCrushEffect` with shader, gradientLUT pointer, time accumulator, and all uniform location ints. Define `BIT_CRUSH_CONFIG_FIELDS` macro. Declare Init, Setup, Uninit, ConfigDefault, RegisterParams. Init takes `(BitCrushEffect*, const BitCrushConfig*)`. Setup takes `(BitCrushEffect*, const BitCrushConfig*, float deltaTime, Texture2D fftTexture)`. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`.

**Verify**: Header parses (included by later tasks).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect module implementation

**Files**: `src/effects/bit_crush.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement following `motherboard.cpp` as pattern. Include own header plus `audio/audio.h`, `automation/modulation_engine.h`, `config/effect_descriptor.h`, `render/blend_compositor.h`, `render/color_lut.h`, `render/post_effect.h`, `<stddef.h>`.

- `BitCrushEffectInit`: Load `shaders/bit_crush.fs`, cache all uniform locations, init gradientLUT via `ColorLUTInit`, set `time = 0`. On failure cleanup.
- `BitCrushEffectSetup`: Accumulate `time += speed * deltaTime`. Update LUT. Set all uniforms including resolution, center, fftTexture, sampleRate from `AUDIO_SAMPLE_RATE`.
- `BitCrushEffectUninit`: Unload shader, free LUT.
- `BitCrushConfigDefault`: Return `BitCrushConfig{}`.
- `BitCrushRegisterParams`: Register all modulatable params with `"bitCrush."` prefix and ranges from parameter table.
- `SetupBitCrush`: Bridge from `PostEffect*` — call `BitCrushEffectSetup(&pe->bitCrush, &pe->effects.bitCrush, pe->currentDeltaTime, pe->fftTexture)`.
- `SetupBitCrushBlend`: Call `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.bitCrush.blendIntensity, pe->effects.bitCrush.blendMode)`.
- Bottom: `REGISTER_GENERATOR(TRANSFORM_BIT_CRUSH_BLEND, BitCrush, bitCrush, "Bit Crush Blend", SetupBitCrushBlend, SetupBitCrush)`.

**Verify**: Compiles after all Wave 2 tasks complete.

#### Task 2.2: Fragment shader

**Files**: `shaders/bit_crush.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from the Design. Uniforms: `resolution`, `center`, `fftTexture`, `gradientLUT`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `scale`, `cellSize`, `iterations`, `time`, `glowIntensity`. The `r()` hash function, iterative lattice walk, gradient LUT color, and FFT brightness modulation are all specified in the Algorithm section above. Use `#version 330`. Center coordinates relative to `center` uniform.

**Verify**: Shader loads at runtime (no build step needed).

#### Task 2.3: Config registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/bit_crush.h"`. Add `TRANSFORM_BIT_CRUSH_BLEND` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`. Add `TRANSFORM_BIT_CRUSH_BLEND` to `TransformOrderConfig::order` array (at end, before closing brace). Add `BitCrushConfig bitCrush;` to `EffectConfig` struct.

**Verify**: Compiles.

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/bit_crush.h"`. Add `BitCrushEffect bitCrush;` member to `PostEffect` struct.

**Verify**: Compiles.

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/bit_crush.cpp` to `EFFECTS_SOURCES` list.

**Verify**: `cmake.exe --build build` succeeds.

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_gen_texture.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow `DrawGeneratorsMotherboard` as pattern.

- Add `#include "effects/bit_crush.h"` to includes
- Add `static bool sectionBitCrush = false;` with other section bools
- Add `DrawGeneratorsBitCrush` function. UI sections:
  - **Audio**: `SeparatorText("Audio")`. Sliders: Base Freq (Hz) `"%.1f"`, Max Freq (Hz) `"%.0f"`, Gain `"%.1f"`, Contrast `"%.2f"` (maps to `curve`), Base Bright `"%.2f"`. Standard ranges per FFT Audio UI Conventions.
  - **Lattice**: `SeparatorText("Lattice")`. `ModulatableSlider` for Scale `"%.2f"`, Cell Size `"%.1f"`, Speed `"%.2f"`. `ImGui::SliderInt` for Iterations (4-64).
  - **Glow**: `SeparatorText("Glow")`. `ModulatableSlider` for Glow Intensity `"%.2f"`.
  - Color: `ImGuiDrawColorMode(&cfg->gradient)`
  - **Output**: `SeparatorText("Output")`. Blend Intensity + Blend Mode combo.
- Add call in `DrawGeneratorsTexture`: `ImGui::Spacing(); DrawGeneratorsBitCrush(e, modSources, categoryGlow);` after the Motherboard call.

**Verify**: Compiles.

#### Task 2.7: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/bit_crush.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BitCrushConfig, BIT_CRUSH_CONFIG_FIELDS)`. Add `X(bitCrush) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in Generators > Texture panel with correct "GEN" badge
- [ ] Enabling effect shows chunky pixel mosaic pattern
- [ ] Scale, Cell Size, Iterations, Speed controls visibly affect the pattern
- [ ] FFT audio drives cell brightness when music plays
- [ ] Gradient LUT changes cell colors
- [ ] Blend Mode and Intensity work correctly
- [ ] Preset save/load roundtrips all settings
