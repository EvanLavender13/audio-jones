# Spin Cage

A spinning wireframe platonic solid whose fast rotation causes overlapping projected edges to create emergent geometric interference patterns. Five selectable solids (tetrahedron through icosahedron) control pattern density. All rotation, projection, and vertex computation happen CPU-side in Setup; the shader is a minimal distance-field loop with per-edge glow, gradient color, and FFT reactivity.

**Research**: `docs/research/spin-cage.md`

## Design

### Types

**SpinCageConfig** (in `spin_cage.h`):
```
bool enabled = false;

// Shape
int shape = 1;              // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa
int colorMode = 0;          // 0=Edge Index, 1=Depth

// Rotation
float speedX = 0.33f;       // X-axis rotation speed rad/s (-PI..+PI)
float speedY = 0.33f;       // Y-axis rotation speed rad/s (-PI..+PI)
float speedZ = 0.33f;       // Z-axis rotation speed rad/s (-PI..+PI)
float speedMult = 1.0f;     // Global speed multiplier (0.1-100.0)

// Projection
float perspective = 4.0f;   // Camera distance / projection depth (1.0-20.0)
float scale = 1.0f;         // Wireframe size multiplier (0.1-5.0)

// Glow
float lineWidth = 2.0f;     // Glow falloff width in pixel units (0.5-10.0)
float glowIntensity = 1.0f; // Glow brightness (0.1-5.0)

// Audio
float baseFreq = 55.0f;     // Low end FFT freq spread Hz (27.5-440.0)
float maxFreq = 14000.0f;   // High end FFT freq spread Hz (1000-16000)
float gain = 2.0f;          // FFT magnitude amplification (0.1-10.0)
float curve = 1.5f;         // FFT response curve / contrast (0.1-3.0)
float baseBright = 0.0f;    // Base edge visibility without audio (0.0-1.0)

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

// Blend
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
float blendIntensity = 1.0f;
```

**SpinCageEffect** (in `spin_cage.h`):
```
Shader shader;
ColorLUT *gradientLUT;
float angleX, angleY, angleZ;   // Accumulated rotation angles
int resolutionLoc;
int edgesLoc;          // uniform vec4 edges[30]
int edgeTLoc;          // uniform float edgeT[30]
int edgeCountLoc;      // uniform int edgeCount
int lineWidthLoc;
int glowIntensityLoc;
int fftTextureLoc;
int sampleRateLoc;
int baseFreqLoc;
int maxFreqLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
int gradientLUTLoc;
```

Init signature: `bool SpinCageEffectInit(SpinCageEffect *e, const SpinCageConfig *cfg)` — needs cfg for ColorLUT init. Use `REGISTER_GENERATOR`.

### Algorithm

#### CPU-side (SpinCageEffectSetup)

**Platonic solid vertex/edge data**: Static const arrays at file scope in `spin_cage.cpp`. All vertex sets are normalized to unit sphere.

Vertex coordinates (all on unit sphere, distance 1.0 from origin):

- **Tetrahedron** (4V, 6E): `(0, 0, 1)`, `(√(8/9), 0, -1/3)`, `(-√(2/9), √(2/3), -1/3)`, `(-√(2/9), -√(2/3), -1/3)`. Edges: all pairs `(0,1)(0,2)(0,3)(1,2)(1,3)(2,3)`.

- **Cube** (8V, 12E): All `(±a, ±a, ±a)` where `a = 1/√3 ≈ 0.57735`. Edges: 4 front face `(0,1)(1,3)(3,2)(2,0)`, 4 back face `(4,5)(5,7)(7,6)(6,4)`, 4 connecting `(0,4)(1,5)(2,6)(3,7)`. Vertex ordering: 0=(-,-,-), 1=(+,-,-), 2=(-,+,-), 3=(+,+,-), 4=(-,-,+), 5=(+,-,+), 6=(-,+,+), 7=(+,+,+).

- **Octahedron** (6V, 12E): `(±1,0,0)`, `(0,±1,0)`, `(0,0,±1)`. Each vertex connects to 4 neighbors (all except its opposite).

- **Dodecahedron** (20V, 30E): Golden ratio `φ = (1+√5)/2`. Raw vertices: 8 at `(±1,±1,±1)`, 4 at `(0,±1/φ,±φ)`, 4 at `(±1/φ,±φ,0)`, 4 at `(±φ,0,±1/φ)`. All normalized to unit sphere. Edges derived from face connectivity (each vertex degree 3).

- **Icosahedron** (12V, 30E): Cyclic permutations of `(0,±1,±φ)` normalized to unit sphere. Each vertex degree 5.

Store vertex arrays and edge index arrays as `static const` data. Use a shape descriptor table: `{ vertexCount, edgeCount, vertices[], edges[] }`.

**Per-frame Setup**:

1. Accumulate rotation angles:
   ```
   angleX += speedX * speedMult * deltaTime
   angleY += speedY * speedMult * deltaTime
   angleZ += speedZ * speedMult * deltaTime
   ```

2. Build 3x3 rotation matrix `R = Rx(angleX) * Ry(angleY) * Rz(angleZ)` using standard Euler rotation matrices (same construction as reference shader).

3. For each vertex of the selected shape:
   - Rotate: `rotated = R * vertex`
   - Store `rotatedZ[v] = rotated.z` (needed for Depth color mode)
   - Project: `projected.xy = (rotated.xy / (rotated.z + perspective)) * scale`
   Scale is applied **after** the perspective divide — it's a pure 2D size multiplier that doesn't interact with depth foreshortening.

4. For each edge `(i, j)`:
   - Pack endpoints: `edges[e] = vec4(projected[i].x, projected[i].y, projected[j].x, projected[j].y)`
   - Compute `edgeT[e]` based on colorMode:
     - **Edge Index** (colorMode 0): `t = float(e) / float(edgeCount - 1)` if edgeCount > 1, else 0.5 <!-- Intentional deviation from research (which uses /edgeCount) — this maps full gradient range 0..1 -->
     - **Depth** (colorMode 1): `t = (avgZ + 1.0) / 2.0` where `avgZ = (rotatedZ[i] + rotatedZ[j]) / 2` — normalized 0..1 from back to front. Vertices are unit sphere so rotated Z range is [-1, 1].

5. Upload uniforms: `edges[]` via `SetShaderValueV` (SHADER_UNIFORM_VEC4, edgeCount), `edgeT[]` via `SetShaderValueV` (SHADER_UNIFORM_FLOAT, edgeCount), `edgeCount` as int.

#### GPU-side (spin_cage.fs)

```glsl
#version 330

// Based on "cube!" by catson
// https://www.shadertoy.com/view/3XcBDl
// License: CC BY-NC-SA 3.0 Unported
// Modified: All 5 platonic solids, CPU-side projection, FFT reactivity, gradient color

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec4 edges[30];      // Projected edge endpoints (a.x, a.y, b.x, b.y)
uniform float edgeT[30];     // Per-edge t value (0-1) for color + FFT
uniform int edgeCount;

uniform float lineWidth;
uniform float glowIntensity;

uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Line segment distance (verbatim from catson reference)
float lineDist(vec2 p, vec2 a, vec2 b) {
    vec2 ba = b - a;
    vec2 pa = p - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba);
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;

    vec3 result = vec3(0.0);
    float pixelWidth = lineWidth / resolution.y;

    for (int i = 0; i < edgeCount; i++) {
        vec2 a = edges[i].xy;
        vec2 b = edges[i].zw;
        float dist = lineDist(uv, a, b);

        // Glow falloff (Filaments-style inverse distance)
        float glow = pixelWidth / (pixelWidth + dist);

        // Color from gradient LUT
        float t = edgeT[i];
        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // FFT brightness: log-space frequency from t
        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float fftMag = 0.0;
        if (bin <= 1.0) {
            fftMag = texture(fftTexture, vec2(bin, 0.5)).r;
        }
        float brightness = baseBright + pow(clamp(fftMag * gain, 0.0, 1.0), curve);

        result += color * glow * glowIntensity * brightness;
    }

    result = tanh(result);  // Soft clamp to prevent blowout
    finalColor = vec4(result, 1.0);
}
```

**Key shader notes:**
- `gl_FragCoord.xy` centered by subtracting `0.5 * resolution`, divided by `resolution.y` for aspect-correct UV — origin at screen center
- `pixelWidth = lineWidth / resolution.y` converts pixel-unit lineWidth to UV-space glow radius
- Single FFT bin sample per edge (not BAND_SAMPLES) — with 30 edges, each already covers a narrow band
- `tanh()` soft clamp prevents additive blowout from overlapping edges

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| shape | int (enum) | 0-4 | 1 | No | Shape |
| colorMode | int (enum) | 0-1 | 0 | No | Color Mode |
| speedX | float | -PI..+PI | 0.33 | Yes | Speed X |
| speedY | float | -PI..+PI | 0.33 | Yes | Speed Y |
| speedZ | float | -PI..+PI | 0.33 | Yes | Speed Z |
| speedMult | float | 0.1-100.0 | 1.0 | Yes | Speed Mult |
| perspective | float | 1.0-20.0 | 4.0 | Yes | Perspective |
| scale | float | 0.1-5.0 | 1.0 | Yes | Scale |
| lineWidth | float | 0.5-10.0 | 2.0 | Yes | Line Width |
| glowIntensity | float | 0.1-5.0 | 1.0 | Yes | Glow |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.0 | Yes | Base Bright |

### Constants

- Enum: `TRANSFORM_SPIN_CAGE_BLEND`
- Display name: `"Spin Cage"`
- Field name: `spinCage`
- Category badge: `"GEN"` (auto via `REGISTER_GENERATOR`)
- Section index: `10` (Geometric)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create SpinCage header

**Files**: `src/effects/spin_cage.h` (create)
**Creates**: Config struct, Effect struct, lifecycle declarations

**Do**: Create header following the Filaments header pattern (`filaments.h`). Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Define `SpinCageConfig` and `SpinCageEffect` structs per the Design section. Define `SPIN_CAGE_CONFIG_FIELDS` macro listing all serializable fields. Forward-declare `ColorLUT`. Declare `SpinCageEffectInit`, `SpinCageEffectSetup` (takes `SpinCageConfig*`, `float deltaTime`, `Texture2D fftTexture`), `SpinCageEffectUninit`, `SpinCageConfigDefault`, `SpinCageRegisterParams`.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Implementation (all parallel — no file overlap)

#### Task 2.1: Create SpinCage implementation

**Files**: `src/effects/spin_cage.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create implementation following the Filaments/Interference pattern. Include: own header, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<math.h>`, `<stddef.h>`.

**Static const vertex/edge data**: Define arrays for all 5 platonic solids per the Design section Algorithm. Use a shape descriptor struct: `{ int vertexCount; int edgeCount; const float (*vertices)[3]; const int (*edges)[2]; }`. Build a lookup table indexed by shape enum.

**Init**: Load shader `"shaders/spin_cage.fs"`, cache all uniform locations, init ColorLUT. Init `angleX = angleY = angleZ = 0.0f`.

**Setup**: Implement the CPU-side Algorithm section:
- Accumulate per-axis angles with `speedMult`
- Build Rx * Ry * Rz rotation matrix (3x3 via 3 mat3 multiplies)
- Rotate each vertex, perspective-project to 2D
- Pack edges as vec4 + compute edgeT per colorMode
- Upload uniforms via `SetShaderValue` / `SetShaderValueV`
- Bind fftTexture, gradientLUT, sampleRate

**RegisterParams**: Register all 13 modulatable params (all except shape and colorMode).

**Colocated UI** (`// === UI ===` section): `static void DrawSpinCageParams()` with:
- Shape section: `ImGui::Combo("Shape##spinCage", ...)` with `"Tetrahedron\0Cube\0Octahedron\0Dodecahedron\0Icosahedron\0"`, `ImGui::Combo("Color Mode##spinCage", ...)` with `"Edge Index\0Depth\0"`
- Rotation section: `ModulatableSliderSpeedDeg` for speedX/Y/Z, `ModulatableSlider` for speedMult (range 0.1-100, "%.1f")
- Projection section: `ModulatableSlider` for perspective and scale
- Glow section: `ModulatableSlider` for lineWidth and glowIntensity
- Audio section: standard order (Base Freq, Max Freq, Gain, Contrast, Base Bright) per conventions

**Bridge functions**: Non-static `SetupSpinCage(PostEffect* pe)` passing `pe->fftTexture`, and `SetupSpinCageBlend(PostEffect* pe)` calling `BlendCompositorApply`. Then `STANDARD_GENERATOR_OUTPUT(spinCage)` and `REGISTER_GENERATOR(TRANSFORM_SPIN_CAGE_BLEND, SpinCage, spinCage, "Spin Cage", SetupSpinCageBlend, SetupSpinCage, 10, DrawSpinCageParams, DrawOutput_spinCage)`.

**Verify**: Compiles after all Wave 2 tasks complete.

#### Task 2.2: Create SpinCage shader

**Files**: `shaders/spin_cage.fs` (create)
**Depends on**: Wave 1 complete (for understanding uniform names, but no #include dependency)

**Do**: Implement the GPU-side Algorithm section from this plan verbatim. The shader code is fully specified in the Design section — implement it as written. Key points:
- Attribution comment block before `#version 330` referencing catson's "cube!" shader
- Center UV via `gl_FragCoord.xy - 0.5 * resolution`, divide by `resolution.y`
- `pixelWidth = lineWidth / resolution.y` for UV-space glow
- Loop over `edgeCount` edges, compute `lineDist`, inverse-distance glow, gradient color, single FFT bin per edge, accumulate
- `tanh()` soft clamp

**Verify**: File exists, valid GLSL syntax.

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/spin_cage.h"` with other effect includes
2. Add `TRANSFORM_SPIN_CAGE_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add `TRANSFORM_SPIN_CAGE_BLEND` to `TransformOrderConfig::order` initialization array (at end before closing brace)
4. Add `SpinCageConfig spinCage;` member to `EffectConfig` struct

**Verify**: Compiles.

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/spin_cage.h"` with other effect includes
2. Add `SpinCageEffect spinCage;` member to `PostEffect` struct

**Verify**: Compiles.

#### Task 2.5: Build system + serialization

**Files**: `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `src/effects/spin_cage.cpp` to `EFFECTS_SOURCES` in `CMakeLists.txt`
2. In `effect_serialization.cpp`:
   - Add `#include "effects/spin_cage.h"`
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpinCageConfig, SPIN_CAGE_CONFIG_FIELDS)`
   - Add `X(spinCage) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Spin Cage appears in Geometric generator section (section 10)
- [ ] Shape combo switches between all 5 platonic solids
- [ ] Rotation controls spin the wireframe on each axis independently
- [ ] Speed Mult accelerates all axes proportionally
- [ ] Perspective slider changes depth exaggeration
- [ ] Edges glow with gradient LUT colors
- [ ] FFT reactivity makes edges brighten to audio
- [ ] Color Mode toggle switches between Edge Index and Depth mapping
- [ ] Preset save/load round-trips all parameters

---

## Implementation Notes

- `SpinCageEffectSetup` was split into three functions (`TransformVertices`, `UploadUniforms`, `SpinCageEffectSetup`) to stay under the 50-line convention.
- Plan specified `baseBright = 0.0f` — changed to `0.15f` to match the standard generator default (now documented in conventions.md).
- `blendIntensity` registered as a 14th modulatable param, consistent with other generators (plan counted 13).
