# Fractal Tree

An endlessly zooming self-similar tree built from a Kaleidoscopic Iterated Function System (KIFS). Golden-ratio branching creates exact self-similarity at every scale, so the camera flies inward (or outward) forever through recursive forks. Branches are colored by depth via gradient LUT, and each depth level reacts to a different frequency band -- bass lights the trunk while treble sparkles in the canopy.

**Research**: `docs/research/fractal_tree.md`

## Design

### Types

**FractalTreeConfig** (`src/effects/fractal_tree.h`):

```c
struct FractalTreeConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;    // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f;  // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;         // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;        // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f;  // Minimum brightness floor (0.0-1.0)

  // Geometry
  float thickness = 1.0f;    // Branch thickness multiplier (0.5-4.0)
  int maxIterations = 16;    // Base KIFS iteration count (8-32)

  // Animation
  float zoomSpeed = 0.5f;    // Zoom animation rate (0.1-3.0)
  bool zoomOut = false;       // Zoom direction (false=inward, true=outward)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

Field-list macro:
```c
#define FRACTAL_TREE_CONFIG_FIELDS                                            \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, thickness,            \
      maxIterations, zoomSpeed, zoomOut, gradient, blendMode, blendIntensity
```

**FractalTreeEffect** (`src/effects/fractal_tree.h`):

```c
typedef struct FractalTreeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float zoomAccum;       // CPU-accumulated: zoomAccum += zoomSpeed * deltaTime
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int thicknessLoc;
  int maxIterLoc;
  int zoomAccumLoc;
  int zoomOutLoc;
  int gradientLUTLoc;
} FractalTreeEffect;
```

### Algorithm

The shader implements the KIFS fractal tree from the reference code with depth tracking, gradient LUT color, and FFT audio reactivity. The `samp()` function is called 5 times (center + 4 cardinal offsets at 0.5px) and averaged for anti-aliasing.

#### samp(vec2 p, float zoom, float ct) -> vec4

All other values are read from uniforms directly.

```glsl
vec4 samp(vec2 p, float zoom, float ct) {
    // Center on limiting point and zoom
    p = (2.0 * p - resolution) / resolution.y;
    p *= 0.5 * zoom;
    p += vec2(0.70062927, 0.9045085);

    // KIFS core -- golden-ratio constants are locked
    float s = zoom;
    p.x = abs(p.x);
    int depth = 0;
    int iMax = maxIter + int(ct);
    for (int i = 0; i < iMax; i++) {
        if (p.y - 1.0 > -0.577 * p.x) {
            p.y -= 1.0;
            p *= mat2(0.809015, -1.401257, 1.401257, 0.809015);
            p.x = abs(p.x);
            s *= 1.618033;
            depth = i + 1;
        } else {
            break;
        }
    }

    // Branch test
    float inBranch = min(step(p.x, s * thickness * 0.5 / resolution.y),
                         step(abs(p.y - 0.5), 0.5));
    if (inBranch < 0.5) {
        return vec4(0.0);
    }

    // Depth-indexed color and audio
    float t = float(depth) / float(iMax);

    vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    return vec4(col * brightness, 1.0);
}
```

#### main()

```glsl
void main() {
    float ct = 6.0 * fract(zoomAccum);
    if (zoomOut) { ct = 6.0 - ct; }
    float zoom = pow(0.618, ct);

    vec2 pos = fragTexCoord * resolution;

    // 5-sample supersampling (center + 4 cardinal at 0.5px)
    vec4 col = samp(pos, zoom, ct);
    col += samp(pos - vec2(0.5, 0.0), zoom, ct);
    col += samp(pos + vec2(0.5, 0.0), zoom, ct);
    col += samp(pos - vec2(0.0, 0.5), zoom, ct);
    col += samp(pos + vec2(0.0, 0.5), zoom, ct);
    col *= 0.2;

    finalColor = col;
}
```

Key constraints:
- Limit point `vec2(0.70062927, 0.9045085)`, scale `1.618033`, zoom base `0.618`, rotation matrix `mat2(0.809015, -1.401257, 1.401257, 0.809015)`, branch condition `p.y - 1.0 > -0.577 * p.x` are ALL locked golden-ratio constants. Never parameterize.
- `zoomAccum` is CPU-accumulated: `zoomAccum += zoomSpeed * deltaTime`. The shader receives it as a uniform. <!-- Intentional deviation: research calls this 'time' but zoomAccum is more descriptive -->
- The 5-sample pattern is the reference's anti-aliasing. Keep verbatim.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |
| thickness | float | 0.5-4.0 | 1.0 | yes | Thickness |
| maxIterations | int | 8-32 | 16 | no | Max Iterations |
| zoomSpeed | float | 0.1-3.0 | 0.5 | yes | Zoom Speed |
| zoomOut | bool | - | false | no | Zoom Out |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_FRACTAL_TREE_BLEND`
- Display name: `"Fractal Tree"`
- Category section: 10 (Geometric)
- Config field name: `fractalTree`
- Effect field name: `fractalTree`
- Shader file: `shaders/fractal_tree.fs`

### UI Layout

```
Audio
  Base Freq (Hz)  -- ModulatableSlider, "%.1f"
  Max Freq (Hz)   -- ModulatableSlider, "%.0f"
  Gain            -- ModulatableSlider, "%.1f"
  Contrast        -- ModulatableSlider, "%.2f"
  Base Bright     -- ModulatableSlider, "%.2f"
Geometry
  Thickness       -- ModulatableSlider, "%.2f"
  Max Iterations  -- ImGui::SliderInt, 8-32
Animation
  Zoom Speed      -- ModulatableSlider, "%.2f"
  Zoom Out        -- ImGui::Checkbox
Color (via STANDARD_GENERATOR_OUTPUT)
  Gradient widget, Blend Intensity slider, Blend Mode combo
```

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create fractal_tree.h

**Files**: `src/effects/fractal_tree.h` (create)
**Creates**: `FractalTreeConfig`, `FractalTreeEffect`, function declarations

**Do**: Create the header following `hex_rush.h` as pattern. Define:
- `FractalTreeConfig` with all fields from the Design section
- `FRACTAL_TREE_CONFIG_FIELDS` macro
- `FractalTreeEffect` struct with shader, gradientLUT, zoomAccum, and all `*Loc` fields from the Design section
- Forward declare `ColorLUT`
- Function declarations: `FractalTreeEffectInit(FractalTreeEffect *e, const FractalTreeConfig *cfg)`, `FractalTreeEffectSetup(... float deltaTime, const Texture2D &fftTexture)`, `FractalTreeEffectUninit(...)`, `FractalTreeRegisterParams(...)`
- Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`

**Verify**: `cmake.exe --build build` compiles (header-only, no link needed yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create fractal_tree.cpp

**Files**: `src/effects/fractal_tree.cpp` (create)
**Depends on**: Wave 1

**Do**: Implement the effect module following `hex_rush.cpp` as pattern:

- **Init**: Load `shaders/fractal_tree.fs`, cache all uniform locations, init `ColorLUT`, init `zoomAccum = 0.0f`. Cascade cleanup on failure (same pattern as hex_rush).
- **Setup**: Accumulate `zoomAccum += cfg->zoomSpeed * deltaTime`. Update ColorLUT. Bind all uniforms including `fftTexture`, `sampleRate` (from `AUDIO_SAMPLE_RATE`), `gradientLUT`, and the `zoomOut` bool (pass as int: `int zo = cfg->zoomOut ? 1 : 0`).
- **Uninit**: Unload shader, free ColorLUT.
- **RegisterParams**: Register all modulatable params from the Parameters table. Register `blendIntensity` too.
- **UI section**: `DrawFractalTreeParams` with Audio, Geometry, Animation sections per the UI Layout. Use `ImGui::Checkbox("Zoom Out##fractaltree", &cfg->zoomOut)` for the bool.
- **Bridge functions**: `SetupFractalTree(PostEffect *pe)` for scratch, `SetupFractalTreeBlend(PostEffect *pe)` for blend compositor.
- **Registration**: `STANDARD_GENERATOR_OUTPUT(fractalTree)` then `REGISTER_GENERATOR(TRANSFORM_FRACTAL_TREE_BLEND, FractalTree, fractalTree, "Fractal Tree", SetupFractalTreeBlend, SetupFractalTree, 10, DrawFractalTreeParams, DrawOutput_fractalTree)`

Include the standard generator `.cpp` includes: own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.2: Create fractal_tree.fs

**Files**: `shaders/fractal_tree.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the Algorithm section. The shader has:

- Attribution header (coffeecake6022, CC BY-NC-SA 3.0, Shadertoy link)
- `#version 330`
- Uniforms: `resolution` (vec2), `fftTexture` (sampler2D), `sampleRate` (float), `baseFreq` (float), `maxFreq` (float), `gain` (float), `curve` (float), `baseBright` (float), `thickness` (float), `maxIter` (int), `zoomAccum` (float), `zoomOut` (int), `gradientLUT` (sampler2D)
- `samp(vec2 p, float zoom, float ct)` function exactly as specified in the Algorithm section. All other values read from uniforms directly.
- `main()` computes `ct`, `zoom`, then 5 supersamples averaged with `* 0.2`
- Background is black (pixels failing branch test return `vec4(0.0)`)

**Verify**: Shader file exists with correct attribution and `#version 330`.

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/fractal_tree.h"` with other effect includes (alphabetical)
2. Add `TRANSFORM_FRACTAL_TREE_BLEND,` to the `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE`
3. Add `FractalTreeConfig fractalTree;` to the `EffectConfig` struct with a one-line comment

**Verify**: Compiles.

---

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/fractal_tree.h"` with other effect includes
2. Add `FractalTreeEffect fractalTree;` member to the `PostEffect` struct

**Verify**: Compiles.

---

#### Task 2.5: Register in effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/fractal_tree.h"` with other includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FractalTreeConfig, FRACTAL_TREE_CONFIG_FIELDS)`
3. Add `X(fractalTree)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

#### Task 2.6: Add to CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/fractal_tree.cpp` to the `EFFECTS_SOURCES` list (alphabetical placement).

**Verify**: Compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [ ] Effect appears in Geometric category in Effects window
- [ ] Enabling shows the fractal tree with continuous zoom animation
- [ ] Gradient LUT colors branches by depth (trunk = one end, twigs = other)
- [ ] FFT makes trunk pulse to bass, canopy sparkle to treble
- [ ] Negative zoom speed reverses zoom direction
- [ ] Thickness slider visibly changes branch width
- [ ] Max Iterations slider changes detail level
- [ ] Rotation Speed spins the tree without breaking the fractal
- [ ] Preset save/load round-trips all settings

## Implementation Notes

- **Alpha must be 1.0 for all pixels**: Generator scratch buffer uses alpha blending. Background pixels must output `vec4(0.0, 0.0, 0.0, 1.0)` (opaque black), not `vec4(0.0)` (transparent). Transparent pixels become no-ops under alpha blending, leaving the previous frame's scratch content visible as feedback trails.
- **zoomOut bool removed**: Replaced with bidirectional `zoomSpeed` range (-3.0 to +3.0). Negative speed naturally reverses the zoom via GLSL `fract()` on the CPU-accumulated value. Simpler interface, same result.
- **rotationSpeed added**: Global rotation applied to centered coordinates before the KIFS limit point offset. Rotates the viewing angle without affecting the golden-ratio constants or the seamless zoom loop. CPU-accumulated as `rotationAccum += rotationSpeed * deltaTime`.
