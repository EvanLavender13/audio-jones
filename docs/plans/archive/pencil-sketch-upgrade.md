# Pencil Sketch Upgrade

Full-fidelity port of flockaroo's "notebook drawings" shader, replacing the current simplified pencil sketch with gradient-aligned hatching strokes, colored tinting via halftone threshold, noise-modulated pencil pressure, cubic contrast curve, and paper texture. Introduces a shared noise texture utility for the noise sampling the algorithm requires.

**Research**: `docs/research/pencil-sketch-upgrade.md`

## Design

### Types

**PencilSketchConfig** (modify `src/effects/pencil_sketch.h`):

```cpp
struct PencilSketchConfig {
  bool enabled = false;
  int angleCount = 3;            // Number of hatching directions (2-6)
  int sampleCount = 16;          // Samples per direction/stroke length (8-24)
  float gradientEps = 0.4f;      // Edge sensitivity epsilon (0.2-1.0)
  float desaturation = 1.8f;     // Color muting toward gray (0.0-3.0)
  float toneCap = 0.7f;          // Max brightness clamp (0.3-1.0)
  float noiseInfluence = 0.8f;   // Noise modulation of stroke intensity (0.0-1.0)
  float colorStrength = 1.0f;    // Monochrome (0) to tinted strokes (1) (0.0-1.0)
  float paperStrength = 1.0f;    // Paper texture visibility (0.0-1.0)
  float vignetteStrength = 1.0f; // Edge darkening (0.0-1.0)
  float wobbleSpeed = 1.0f;      // Animation rate, 0 = static (0.0-2.0)
  float wobbleAmount = 4.0f;     // Pixel displacement magnitude (0.0-8.0)
};
```

Removed: `strokeFalloff` (reference uses fixed linear falloff).

```cpp
#define PENCIL_SKETCH_CONFIG_FIELDS                                            \
  enabled, angleCount, sampleCount, gradientEps, desaturation, toneCap,        \
      noiseInfluence, colorStrength, paperStrength, vignetteStrength,           \
      wobbleSpeed, wobbleAmount
```

**PencilSketchEffect** (modify `src/effects/pencil_sketch.h`):

```cpp
typedef struct PencilSketchEffect {
  Shader shader;
  int resolutionLoc;
  int angleCountLoc;
  int sampleCountLoc;
  int gradientEpsLoc;
  int desaturationLoc;
  int toneCapLoc;
  int noiseInfluenceLoc;
  int colorStrengthLoc;
  int paperStrengthLoc;
  int vignetteStrengthLoc;
  int wobbleTimeLoc;
  int wobbleAmountLoc;
  int noiseTexLoc;
  float wobbleTime;
} PencilSketchEffect;
```

Removed: `strokeFalloffLoc`. Added: `desaturationLoc`, `toneCapLoc`, `noiseInfluenceLoc`, `colorStrengthLoc`, `noiseTexLoc`.

**Noise texture API** (new `src/render/noise_texture.h`):

```cpp
// Generate and upload noise texture (call once at startup)
void NoiseTextureInit(void);

// Return the shared noise Texture2D
Texture2D NoiseTextureGet(void);

// Unload at shutdown
void NoiseTextureUninit(void);
```

No struct — module-internal static `Texture2D`. The texture is 1024x1024 RGBA, bilinear + wrap-repeat.

### Algorithm

#### Noise Texture Generation (`noise_texture.cpp`)

PCG hash for deterministic high-quality noise:

```c
static uint32_t pcg_hash(uint32_t input) {
    uint32_t state = input * 747796405u + 2891336453u;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}
```

Generate 1024 * 1024 pixels. Each pixel `(x, y)` seeded with `(y * 1024 + x)`. Hash 4 times with different seed offsets for independent R, G, B, A channels. Each channel: `(pcg_hash(seed + channelOffset) >> 24)` to get a `uint8_t` value (0-255). Upload as `RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8`. Set bilinear filter, wrap-repeat.

#### Pencil Sketch Shader (`pencil_sketch.fs`)

Full GLSL — this is the single source of truth for the shader implementation:

```glsl
// Based on "notebook drawings" by flockaroo (Florian Berger)
// https://www.shadertoy.com/view/XtVGD1
// License: CC BY-NC-SA 3.0 Unported
// Modified: Adapted for AudioJones pipeline with configurable parameters

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Input scene
uniform sampler2D texture1;   // Shared noise texture (1024x1024 RGBA)
uniform vec2 resolution;
uniform int angleCount;
uniform int sampleCount;
uniform float gradientEps;
uniform float desaturation;
uniform float toneCap;
uniform float noiseInfluence;
uniform float colorStrength;
uniform float paperStrength;
uniform float vignetteStrength;
uniform float wobbleTime;
uniform float wobbleAmount;

#define PI2 6.28318530718

// Sample noise texture with resolution-scaled UV
vec4 getRand(vec2 pos) {
    vec2 noiseRes = vec2(textureSize(texture1, 0));
    return textureLod(texture1, pos / noiseRes / resolution.y * 1080.0, 0.0);
}

// Sample input with desaturation and tonal clamping
vec4 getCol(vec2 pos) {
    vec2 uv = pos / resolution;
    vec4 c1 = texture(texture0, uv);
    // Edge fade to white (prevents wobble from sampling outside bounds)
    vec4 e = smoothstep(vec4(-0.05), vec4(0.0), vec4(uv, vec2(1.0) - uv));
    c1 = mix(vec4(1.0, 1.0, 1.0, 0.0), c1, e.x * e.y * e.z * e.w);
    // Desaturate: extract green-dominant brightness, mix toward gray
    float d = clamp(dot(c1.xyz, vec3(-0.5, 1.0, -0.5)), 0.0, 1.0);
    vec4 c2 = vec4(0.7);
    return min(mix(c1, c2, desaturation * d), toneCap);
}

// Halftone threshold: smoothstep around noise-dithered brightness
vec4 getColHT(vec2 pos) {
    return smoothstep(0.95, 1.05, getCol(pos) * 0.8 + 0.2 + getRand(pos * 0.7));
}

// Luminance from desaturated color
float getVal(vec2 pos) {
    vec4 c = getCol(pos);
    return dot(c.xyz, vec3(0.333));
}

// Central-difference gradient
vec2 getGrad(vec2 pos, float eps) {
    vec2 d = vec2(eps, 0.0);
    return vec2(
        getVal(pos + d.xy) - getVal(pos - d.xy),
        getVal(pos + d.yx) - getVal(pos - d.yx)
    ) / eps / 2.0;
}

void main() {
    float scale = resolution.y / 400.0;
    vec2 pos = fragTexCoord * resolution
             + wobbleAmount * sin(wobbleTime * vec2(1.0, 1.7)) * scale;

    float col = 0.0;        // Darkness accumulator
    vec3 col2 = vec3(0.0);  // Tinted color accumulator
    float sum = 0.0;

    for (int i = 0; i < angleCount; i++) {
        float ang = PI2 / float(angleCount) * (float(i) + 0.8);
        vec2 v = vec2(cos(ang), sin(ang));

        for (int j = 0; j < sampleCount; j++) {
            // Perpendicular offset (stroke width)
            vec2 dpos = v.yx * vec2(1.0, -1.0) * float(j) * scale;
            // Parallel offset (curved stroke path, j^2 taper)
            vec2 dpos2 = v.xy * float(j * j) / float(sampleCount) * 0.5 * scale;

            for (float s = -1.0; s <= 1.0; s += 2.0) {
                vec2 pos2 = pos + s * dpos + dpos2;
                // Color sample at perpendicular offset, 2x distance
                vec2 pos3 = pos + (s * dpos + dpos2).yx * vec2(1.0, -1.0) * 2.0;
                vec2 g = getGrad(pos2, gradientEps);

                // Stroke: gradient aligned with sample direction
                float fact = dot(g, v)
                           - 0.5 * abs(dot(g, v.yx * vec2(1.0, -1.0)));
                // Color direction factor
                float fact2 = dot(normalize(g + vec2(0.0001)),
                                  v.yx * vec2(1.0, -1.0));

                fact = clamp(fact, 0.0, 0.05);
                fact2 = abs(fact2);

                // Fixed linear falloff (reference algorithm)
                fact *= 1.0 - float(j) / float(sampleCount);

                col += fact;
                col2 += fact2 * getColHT(pos3).xyz;
                sum += fact2;
            }
        }
    }

    // Normalize
    col /= float(sampleCount * angleCount) * 0.75 / sqrt(resolution.y);
    col2 /= sum;

    // Noise-modulated stroke intensity
    float noiseVal = getRand(pos * 0.7).x;
    col *= mix(1.0, 0.6 + 0.8 * noiseVal, noiseInfluence);

    // Cubic contrast curve
    col = 1.0 - col;
    col *= col * col;

    // Paper grid texture
    vec2 sp = sin(pos.xy * 0.1 / sqrt(scale));
    vec3 karo = vec3(1.0);
    karo -= paperStrength * 0.5 * vec3(0.25, 0.1, 0.1)
          * dot(exp(-sp * sp * 80.0), vec2(1.0));

    // Vignette
    float r = length(pos - resolution.xy * 0.5) / resolution.x;
    float vign = 1.0 - r * r * r * vignetteStrength;

    // Final: blend monochrome vs tinted strokes
    vec3 result = vec3(col) * mix(vec3(1.0), col2, colorStrength) * karo * vign;

    finalColor = vec4(result, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| angleCount | int | 2-6 | 3 | No | Angle Count |
| sampleCount | int | 8-24 | 16 | No | Sample Count |
| gradientEps | float | 0.2-1.0 | 0.4 | Yes | Gradient Eps |
| desaturation | float | 0.0-3.0 | 1.8 | Yes | Desaturation |
| toneCap | float | 0.3-1.0 | 0.7 | Yes | Tone Cap |
| noiseInfluence | float | 0.0-1.0 | 0.8 | Yes | Noise Influence |
| colorStrength | float | 0.0-1.0 | 1.0 | Yes | Color Strength |
| paperStrength | float | 0.0-1.0 | 1.0 | Yes | Paper Strength |
| vignetteStrength | float | 0.0-1.0 | 1.0 | Yes | Vignette |
| wobbleSpeed | float | 0.0-2.0 | 1.0 | No | Wobble Speed |
| wobbleAmount | float | 0.0-8.0 | 4.0 | Yes | Wobble Amount |

### Constants

- Enum: `TRANSFORM_PENCIL_SKETCH` (existing, unchanged)
- Display name: `"Pencil Sketch"` (unchanged)
- Badge: `"ART"`, section 4 (unchanged)

---

## Tasks

### Wave 1: Headers

#### Task 1.1: Create noise texture header

**Files**: `src/render/noise_texture.h` (CREATE)
**Creates**: Noise texture API that `pencil_sketch.cpp` and `post_effect.cpp` will include

**Do**: Create header with include guard `NOISE_TEXTURE_H`. Include `"raylib.h"`. Declare three functions: `NoiseTextureInit(void)`, `NoiseTextureGet(void)` returning `Texture2D`, `NoiseTextureUninit(void)`. Add one-line comments per function matching the Design section. No struct — the texture is module-internal state.

**Verify**: Header compiles when included.

---

#### Task 1.2: Update pencil sketch header

**Files**: `src/effects/pencil_sketch.h` (MODIFY)
**Creates**: Updated config/effect struct layouts for Wave 2 tasks

**Do**: Replace the `PencilSketchConfig` struct and `PENCIL_SKETCH_CONFIG_FIELDS` macro with the Design section versions. Remove `strokeFalloff`. Add `desaturation`, `toneCap`, `noiseInfluence`, `colorStrength`. Replace the `PencilSketchEffect` struct with the Design section version — remove `strokeFalloffLoc`, add `desaturationLoc`, `toneCapLoc`, `noiseInfluenceLoc`, `colorStrengthLoc`, `noiseTexLoc`. Keep all function declarations unchanged.

**Verify**: `cmake.exe --build build` compiles (will have warnings from .cpp mismatches until Wave 2, but headers parse).

---

### Wave 2: Implementation

#### Task 2.1: Implement noise texture module

**Files**: `src/render/noise_texture.cpp` (CREATE)
**Depends on**: Task 1.1

**Do**: Include `"noise_texture.h"`, `"rlgl.h"`, `<stdint.h>`, `<stdlib.h>`. Implement with a file-static `Texture2D noiseTexture`.

`NoiseTextureInit`: Allocate a `1024 * 1024 * 4` byte buffer. For each pixel `(x, y)`, compute base seed `y * 1024 + x`. Hash with PCG (from Design section) using 4 different seed offsets (e.g., seed, seed + 1, seed + 2, seed + 3) for R, G, B, A. Extract top 8 bits: `(hash >> 24) & 0xFF`. Store as `uint8_t[4]`. Upload with `rlLoadTexture(data, 1024, 1024, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1)`. Build the `Texture2D` struct (width=1024, height=1024, mipmaps=1, format=`RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8`). Set `TEXTURE_FILTER_BILINEAR` and `TEXTURE_WRAP_REPEAT`. Free the pixel buffer. Log creation with `TraceLog(LOG_INFO, ...)`.

`NoiseTextureGet`: Return the static `noiseTexture`.

`NoiseTextureUninit`: Call `UnloadTexture(noiseTexture)`.

**Verify**: Compiles.

---

#### Task 2.2: Rewrite pencil sketch shader

**Files**: `shaders/pencil_sketch.fs` (MODIFY)
**Depends on**: Wave 1 complete

**Do**: Replace the entire shader with the Algorithm section GLSL verbatim. The Algorithm section IS the shader — copy it exactly. This includes the attribution header, all uniforms, all helper functions (`getRand`, `getCol`, `getColHT`, `getVal`, `getGrad`), and `main()`.

**Verify**: Shader file has the correct attribution header citing flockaroo / XtVGD1.

---

#### Task 2.3: Update pencil sketch C++ module

**Files**: `src/effects/pencil_sketch.cpp` (MODIFY)
**Depends on**: Tasks 1.1, 1.2

**Do**: Add `#include "render/noise_texture.h"` to includes.

Update `PencilSketchEffectInit`: Remove `strokeFalloffLoc` caching. Add `GetShaderLocation` calls for `desaturation`, `toneCap`, `noiseInfluence`, `colorStrength`, and `texture1` (for `noiseTexLoc`).

Update `PencilSketchEffectSetup`: Remove `strokeFalloff` uniform binding. Add `SetShaderValue` calls for the 4 new float uniforms. Add `SetShaderValueTexture(e->shader, e->noiseTexLoc, NoiseTextureGet())` to bind the noise texture each frame.

Update `PencilSketchRegisterParams`: Remove `strokeFalloff` registration. Add registrations for `gradientEps` (0.2-1.0), `desaturation` (0.0-3.0), `toneCap` (0.3-1.0), `noiseInfluence` (0.0-1.0), `colorStrength` (0.0-1.0). Keep existing `paperStrength`, `vignetteStrength`, `wobbleAmount` registrations.

Update `DrawPencilSketchParams` UI function:
- Remove Stroke Falloff slider
- Change Gradient Eps from `ImGui::SliderFloat` to `ModulatableSlider` with ID `"pencilSketch.gradientEps"`
- Add 4 new `ModulatableSlider` calls: Desaturation (`"%.2f"`), Tone Cap (`"%.2f"`), Noise Influence (`"%.2f"`), Color Strength (`"%.2f"`)
- Slider order: Angle Count, Sample Count, Gradient Eps, Desaturation, Tone Cap, Noise Influence, Color Strength, Paper Strength, Vignette, then Animation tree

The bridge function `SetupPencilSketch` and registration macro are unchanged.

**Verify**: Compiles.

---

#### Task 2.4: Wire up noise texture lifecycle and build

**Files**: `src/render/post_effect.cpp`, `CMakeLists.txt` (MODIFY)
**Depends on**: Task 1.1

**Do**:

In `post_effect.cpp`:
- Add `#include "noise_texture.h"` to includes
- In `PostEffectInit`: Add `NoiseTextureInit()` call before the effect descriptor loop (before `for (int i = 0; i < TRANSFORM_EFFECT_COUNT; ...)`)
- In `PostEffectUninit`: Add `NoiseTextureUninit()` call before `UnloadTexture(pe->fftTexture)` (after the descriptor uninit loop)

In `CMakeLists.txt`:
- Add `src/render/noise_texture.cpp` to the `RENDER_SOURCES` list

**Verify**: `cmake.exe --build build` compiles with no errors.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Pencil Sketch effect enables and renders colored hatching strokes
- [ ] Noise texture creates (check TraceLog output)
- [ ] New sliders (Desaturation, Tone Cap, Noise Influence, Color Strength) appear in UI
- [ ] Old Stroke Falloff slider is gone
- [ ] Gradient Eps is now modulatable
- [ ] Existing presets with pencilSketch load without errors (strokeFalloff silently dropped)
- [ ] colorStrength=0 produces monochrome, colorStrength=1 produces tinted strokes
- [ ] noiseInfluence=0 produces uniform strokes, noiseInfluence=1 produces noisy pencil pressure
