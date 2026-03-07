# Curl Advection Generator

Refactor Curl Advection from a compute-shader simulation (`src/simulation/`) to a fragment-shader generator (`src/effects/`) using `REGISTER_GENERATOR_FULL`. The visual output is unchanged: flowing vein-like patterns that organically evolve and branch. The compute shader is per-texel with no agents, no scatter writes, no shared memory — it maps directly to two fragment shaders (state update + color output) with ping-pong render textures.

**Research**: `docs/research/curl-advection-generator.md`

## Design

### Types

**CurlAdvectionConfig** (unchanged from current — preset compatibility):

```cpp
typedef struct CurlAdvectionConfig {
  bool enabled = false;
  int steps = 40;                   // Advection iterations (10-80)
  float advectionCurl = 0.2f;       // How much paths spiral (0.0-1.0)
  float curlScale = -2.0f;          // Vortex rotation strength (-4.0-4.0)
  float laplacianScale = 0.05f;     // Diffusion/smoothing (0.0-0.2)
  float pressureScale = -2.0f;      // Compression waves (-4.0-4.0)
  float divergenceScale = -0.4f;    // Source/sink strength (-1.0-1.0)
  float divergenceUpdate = -0.03f;  // Divergence feedback rate (-0.1-0.1)
  float divergenceSmoothing = 0.3f; // Divergence smoothing (0.0-0.5)
  float selfAmp = 1.0f;             // Self-amplification (0.5-2.0)
  float updateSmoothing = 0.4f;     // Temporal stability (0.25-0.9)
  float injectionIntensity = 0.0f;  // Energy injection (0.0-1.0)
  float injectionThreshold = 0.1f;  // Accum brightness cutoff (0.0-1.0)
  float decayHalfLife = 0.5f;       // Trail decay half-life (0.1-5.0 s)
  int diffusionScale = 0;           // Trail diffusion tap spacing (0-4)
  float boostIntensity = 1.0f;      // Trail boost strength (0.0-5.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  ColorConfig color;
  bool debugOverlay = false;
} CurlAdvectionConfig;

#define CURL_ADVECTION_CONFIG_FIELDS                                           \
  enabled, steps, advectionCurl, curlScale, laplacianScale, pressureScale,     \
      divergenceScale, divergenceUpdate, divergenceSmoothing, selfAmp,         \
      updateSmoothing, injectionIntensity, injectionThreshold, decayHalfLife,  \
      diffusionScale, boostIntensity, blendMode, color, debugOverlay
```

<!-- Intentional deviation: RGBA32F textures instead of research's RGBA16F. RenderUtilsInitTextureHDR creates RGBA32F and is the standard pattern for all generators. Using custom GL for RGBA16F would break the codebase pattern. -->
<!-- Intentional deviation: Category is Field (section 13) per user choice, not Simulation or Texture as research suggested. -->

**CurlAdvectionEffect** (replaces heap-allocated `CurlAdvection*`):

```cpp
typedef struct CurlAdvectionEffect {
  Shader stateShader;                 // State update fragment shader
  Shader shader;                      // Color output fragment shader (named 'shader' for macro compat)
  ColorLUT *colorLUT;
  RenderTexture2D statePingPong[2];   // RGBA32F velocity state (xy=velocity, z=divergence)
  RenderTexture2D pingPong[2];        // RGBA32F visual output with decay
  int stateReadIdx;
  int readIdx;
  Texture2D currentAccumTexture;      // Cached per-frame for render pass

  // State shader uniform locations
  int stateResolutionLoc;
  int stateStepsLoc;
  int stateAdvectionCurlLoc;
  int stateCurlScaleLoc;
  int stateLaplacianScaleLoc;
  int statePressureScaleLoc;
  int stateDivergenceScaleLoc;
  int stateDivergenceUpdateLoc;
  int stateDivergenceSmoothingLoc;
  int stateSelfAmpLoc;
  int stateUpdateSmoothingLoc;
  int stateInjectionIntensityLoc;
  int stateInjectionThresholdLoc;
  int statePreviousStateLoc;
  int stateAccumTextureLoc;

  // Color shader uniform locations
  int colorStateTextureLoc;
  int colorPreviousFrameLoc;
  int colorDecayFactorLoc;
  int colorLUTLoc;
  int colorValueLoc;
  int colorResolutionLoc;
  int colorDiffusionScaleLoc;
} CurlAdvectionEffect;
```

### Algorithm

Two-pass fragment shader pipeline per frame:

**Pass 1 — State Update** (`curl_advection_state.fs`):

Reads `stateTexture` (ping-pong read buffer), writes new velocity/divergence state. Direct port of the compute shader with coordinate changes.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D stateTexture;   // ping-pong read
uniform sampler2D accumTexture;   // for energy injection
uniform vec2 resolution;
uniform int steps;
uniform float advectionCurl;
uniform float curlScale;
uniform float laplacianScale;
uniform float pressureScale;
uniform float divergenceScale;
uniform float divergenceUpdate;
uniform float divergenceSmoothing;
uniform float selfAmp;
uniform float updateSmoothing;
uniform float injectionIntensity;
uniform float injectionThreshold;

#define _D 0.6
#define _K0 (-20.0 / 6.0)
#define _K1 (4.0 / 6.0)
#define _K2 (1.0 / 6.0)
#define _G0 0.25
#define _G1 0.125
#define _G2 0.0625

vec2 rotate2d(vec2 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

vec3 sampleState(vec2 uv) {
    return texture(stateTexture, uv).xyz;
}

void computeCurlAndBlur(vec2 uv, vec2 texel, out float curl, out vec3 blur) {
    vec3 c      = sampleState(uv);
    vec3 uv_n   = sampleState(uv + vec2(0, texel.y));
    vec3 uv_s   = sampleState(uv + vec2(0, -texel.y));
    vec3 uv_e   = sampleState(uv + vec2(texel.x, 0));
    vec3 uv_w   = sampleState(uv + vec2(-texel.x, 0));
    vec3 uv_ne  = sampleState(uv + vec2(texel.x, texel.y));
    vec3 uv_nw  = sampleState(uv + vec2(-texel.x, texel.y));
    vec3 uv_se  = sampleState(uv + vec2(texel.x, -texel.y));
    vec3 uv_sw  = sampleState(uv + vec2(-texel.x, -texel.y));

    curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y
         + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y
               + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);

    blur = _G0 * c
         + _G1 * (uv_n + uv_e + uv_w + uv_s)
         + _G2 * (uv_nw + uv_sw + uv_ne + uv_se);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    // Sample 9 neighbors
    vec3 center = sampleState(uv);
    vec3 uv_n  = sampleState(uv + vec2(0, texel.y));
    vec3 uv_s  = sampleState(uv + vec2(0, -texel.y));
    vec3 uv_e  = sampleState(uv + vec2(texel.x, 0));
    vec3 uv_w  = sampleState(uv + vec2(-texel.x, 0));
    vec3 uv_ne = sampleState(uv + vec2(texel.x, texel.y));
    vec3 uv_nw = sampleState(uv + vec2(-texel.x, texel.y));
    vec3 uv_se = sampleState(uv + vec2(texel.x, -texel.y));
    vec3 uv_sw = sampleState(uv + vec2(-texel.x, -texel.y));

    // Curl (rotation)
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y
        + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y
              + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);

    // Divergence (expansion)
    float div = uv_s.y - uv_n.y - uv_e.x + uv_w.x
        + _D * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y
              + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);

    // Laplacian (diffusion)
    vec3 lapl = _K0 * center
              + _K1 * (uv_n + uv_e + uv_w + uv_s)
              + _K2 * (uv_nw + uv_sw + uv_ne + uv_se);

    // Multi-step self-advection
    vec2 off = center.xy;
    vec2 offd = off;
    vec3 ab = vec3(0);

    for (int i = 0; i < steps; i++) {
        vec2 sampleUV = uv - off * texel;
        float localCurl;
        vec3 localBlur;
        computeCurlAndBlur(sampleUV, texel, localCurl, localBlur);
        offd = rotate2d(offd, advectionCurl * localCurl);
        off += offd;
        ab += localBlur / float(steps);
    }

    // Field update equation
    float sp = pressureScale * lapl.z;
    float sc = curlScale * curl;
    float sd = center.z + divergenceUpdate * div + divergenceSmoothing * lapl.z;

    vec2 norm = length(center.xy) > 0.0 ? normalize(center.xy) : vec2(0);

    vec2 tab = selfAmp * ab.xy
             + laplacianScale * lapl.xy
             + norm * sp
             + center.xy * divergenceScale * sd;

    vec2 rab = rotate2d(tab, sc);

    vec3 result = mix(vec3(rab, sd), center.xyz, updateSmoothing);

    // Accumulation-based energy injection
    if (injectionIntensity > 0.0) {
        vec4 accumSample = texture(accumTexture, uv);
        float brightness = dot(accumSample.rgb, vec3(0.299, 0.587, 0.114));
        float contribution = max(brightness - injectionThreshold, 0.0);

        vec2 injDir = length(result.xy) > 0.001
            ? normalize(result.xy) : vec2(1.0, 0.0);
        result.xy += injectionIntensity * contribution * injDir;
    }

    // Clamp to valid range
    result.z = clamp(result.z, -1.0, 1.0);
    if (length(result.xy) > 1.0) {
        result.xy = normalize(result.xy);
    }

    finalColor = vec4(result, 0.0);
}
```

**Pass 2 — Color Output with Diffusion** (`curl_advection_color.fs`):

Reads freshly-written state, maps velocity angle through colorLUT, blends with previous visual frame for trail persistence. When `diffusionScale > 0`, samples `previousFrame` with a 5-tap cross kernel (H + V) instead of a single tap, producing iterative Gaussian-like diffusion over many frames.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D stateTexture;    // fresh state from pass 1
uniform sampler2D previousFrame;   // visual ping-pong read (for decay)
uniform sampler2D colorLUT;        // velocity angle -> color
uniform vec2 resolution;
uniform float decayFactor;         // exp(-0.693147 * dt / halfLife)
uniform float value;               // brightness from color config
uniform int diffusionScale;        // tap spacing (0 = no blur)

const float PI = 3.14159265359;

// 5-tap Gaussian weights matching TrailMap's trail_diffusion.glsl
const float W0 = 0.0625;
const float W1 = 0.25;
const float W2 = 0.375;

// Sample previousFrame with cross-kernel diffusion blur
vec3 samplePrevious(vec2 uv, vec2 texel, float ds) {
    // Horizontal 5-tap
    vec3 h = texture(previousFrame, uv + vec2(-2.0*ds*texel.x, 0)).rgb * W0
           + texture(previousFrame, uv + vec2(-ds*texel.x, 0)).rgb * W1
           + texture(previousFrame, uv).rgb * W2
           + texture(previousFrame, uv + vec2(ds*texel.x, 0)).rgb * W1
           + texture(previousFrame, uv + vec2(2.0*ds*texel.x, 0)).rgb * W0;

    // Vertical 5-tap
    vec3 v = texture(previousFrame, uv + vec2(0, -2.0*ds*texel.y)).rgb * W0
           + texture(previousFrame, uv + vec2(0, -ds*texel.y)).rgb * W1
           + texture(previousFrame, uv).rgb * W2
           + texture(previousFrame, uv + vec2(0, ds*texel.y)).rgb * W1
           + texture(previousFrame, uv + vec2(0, 2.0*ds*texel.y)).rgb * W0;

    // Average H and V, subtract double-counted center
    return h + v - texture(previousFrame, uv).rgb * W2;
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 state = texture(stateTexture, uv).xyz;

    // Color: velocity angle -> LUT
    float angle = atan(state.y, state.x);
    float t = (angle + PI) / (2.0 * PI);
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
    float brightness = length(state.xy);

    vec3 newColor = color * brightness * value;

    // Previous frame with optional diffusion
    vec3 previous;
    if (diffusionScale > 0) {
        vec2 texel = 1.0 / resolution;
        previous = samplePrevious(uv, texel, float(diffusionScale));
    } else {
        previous = texture(previousFrame, uv).rgb;
    }

    vec3 result = max(newColor, previous * decayFactor);

    finalColor = vec4(result, 1.0);
}
```

### Render Callback Flow (per frame)

1. **State pass**: `BeginTextureMode(statePingPong[writeIdx])` -> draw fullscreen quad with state shader reading `statePingPong[readIdx]` + `accumTexture` -> `EndTextureMode()`
2. **Color pass**: `BeginTextureMode(pingPong[writeIdx])` -> draw fullscreen quad with color shader reading `statePingPong[writeIdx]` (fresh state) + `pingPong[readIdx]` (previous visual, for decay + diffusion) -> `EndTextureMode()`
3. Flip both `stateReadIdx` and `readIdx`
4. Blend compositor composites `pingPong[readIdx]` onto the scene

### State Initialization

On init and reset, upload random noise to both state ping-pong textures via `UpdateTexture`:
- xy: random velocity in [-0.1, 0.1]
- z: divergence = 0
- w: 0

### Parameters

All parameters identical to existing `CurlAdvectionConfig`. No changes to ranges, defaults, or modulation registrations.

### Constants

- Enum: `TRANSFORM_CURL_ADVECTION` (reuse existing — no rename needed, enum value stays at same position)
- Display name: `"Curl Advection"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR_FULL`)
- Section: 13 (Field)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Effect header

**Files**: `src/effects/curl_advection.h` (create)
**Creates**: `CurlAdvectionConfig`, `CurlAdvectionEffect`, function declarations

**Do**: Create the generator header. `CurlAdvectionConfig` is identical to the current `src/simulation/curl_advection.h` config struct (copy it verbatim including defaults, ranges, CONFIG_FIELDS macro). `CurlAdvectionEffect` follows the Design section layout. Function declarations follow the generator convention:

- `bool CurlAdvectionEffectInit(CurlAdvectionEffect *e, const CurlAdvectionConfig *cfg, int width, int height);`
- `void CurlAdvectionEffectSetup(CurlAdvectionEffect *e, const CurlAdvectionConfig *cfg, float deltaTime);`
- `void CurlAdvectionEffectRender(CurlAdvectionEffect *e, const CurlAdvectionConfig *cfg, float deltaTime, int screenWidth, int screenHeight);`
- `void CurlAdvectionEffectResize(CurlAdvectionEffect *e, int width, int height);`
- `void CurlAdvectionEffectUninit(CurlAdvectionEffect *e);`
- `CurlAdvectionConfig CurlAdvectionConfigDefault(void);`
- `void CurlAdvectionRegisterParams(CurlAdvectionConfig *cfg);`

Drop the old simulation-specific declarations: `CurlAdvectionSupported`, `CurlAdvectionUpdate`, `CurlAdvectionProcessTrails`, `CurlAdvectionDrawDebug`, `CurlAdvectionReset`, `CurlAdvectionApplyConfig`. Drop forward declarations for `TrailMap`.

Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `ColorLUT`.

**Verify**: Header compiles when included from a .cpp that also includes raylib.

---

### Wave 2: Parallel Implementation

#### Task 2.1: State update shader

**Files**: `shaders/curl_advection_state.fs` (create)

**Do**: Implement the state update fragment shader from the Algorithm section above. This is a direct port of `shaders/curl_advection.glsl` with these changes:
- `#version 330` instead of `#version 430`
- Remove `layout(local_size_x/y)`, all `layout(binding)` declarations, `image2D` outputs, `trailMap`, `colorLUT`
- Use `in vec2 fragTexCoord` / `out vec4 finalColor` instead of `gl_GlobalInvocationID`
- `uv = fragTexCoord` (direct UV, no manual calculation)
- `stateTexture` and `accumTexture` are `uniform sampler2D` (no binding qualifiers)
- Output state via `finalColor = vec4(result, 0.0)` instead of `imageStore`
- Remove the color/trail output section (that moves to the color shader)
- Keep ALL PDE math, advection loop, and injection logic verbatim

**Verify**: Shader has valid GLSL 330 syntax. All uniforms match the `stateShader` locations in the Design section.

---

#### Task 2.2: Color output shader

**Files**: `shaders/curl_advection_color.fs` (create)

**Do**: Implement the color output fragment shader from the Algorithm section above. This shader:
- Reads `stateTexture` (freshly written state from pass 1)
- Maps velocity angle through `colorLUT` (same as the current compute shader's trail output section)
- Reads `previousFrame` (visual ping-pong read buffer)
- When `diffusionScale > 0`, samples `previousFrame` with a 5-tap cross kernel (H + V, same Gaussian weights as TrailMap: 0.0625, 0.25, 0.375, 0.25, 0.0625). When 0, single sample.
- Blends: `result = max(newColor, previous * decayFactor)`
- Outputs to `finalColor`
- Uniforms: `stateTexture`, `previousFrame`, `colorLUT`, `resolution`, `decayFactor`, `value`, `diffusionScale`

**Verify**: Shader has valid GLSL 330 syntax. All uniforms match the `shader` (color shader) locations in the Design section.

---

#### Task 2.3: Effect implementation

**Files**: `src/effects/curl_advection.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the full generator module following `src/effects/muons.cpp` as the pattern. Key sections:

**Includes** (generator convention):
`"curl_advection.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<math.h>`, `<stddef.h>`, `<stdlib.h>`

**Static helpers**:
- `InitPingPong` / `UnloadPingPong`: allocate/free both state and visual ping-pong pairs using `RenderUtilsInitTextureHDR`
- `InitializeStateWithNoise`: allocate float buffer, fill xy with random [-0.1, 0.1], z=0, w=0, upload via `UpdateTexture` to both `statePingPong` textures, free buffer

**CurlAdvectionEffectInit**: Load `shaders/curl_advection_state.fs` as `stateShader`, `shaders/curl_advection_color.fs` as `shader`. Cache all uniform locations for both shaders (prefixed `state*` and `color*` as in Design section). Init `colorLUT` via `ColorLUTInit`. Init ping-pong textures. Initialize state with noise. Clear visual ping-pong. Set indices to 0. Cascade cleanup on failure (same pattern as `muons.cpp`).

**CurlAdvectionEffectSetup**: Update `ColorLUTUpdate`. Bind all state shader uniforms via `SetShaderValue`. Compute `decayFactor = expf(-0.693147f * deltaTime / fmaxf(cfg->decayHalfLife, 0.001f))` and bind to color shader. Compute `value` from color config (same logic as current `CurlAdvectionUpdate`'s value extraction). Bind color shader uniforms. Cache `pe->accumTexture.texture` as `e->currentAccumTexture` for the render pass.

**CurlAdvectionEffectRender**: Execute two-pass rendering per the Render Callback Flow in the Design section. Use `BeginTextureMode`/`EndTextureMode`, `BeginShaderMode`/`EndShaderMode`, `SetShaderValueTexture` for texture bindings, `RenderUtilsDrawFullscreenQuad` for fullscreen quads. Flip both indices after both passes.

**CurlAdvectionEffectResize**: `UnloadPingPong`, `InitPingPong` at new size, `InitializeStateWithNoise`, reset indices.

**CurlAdvectionEffectUninit**: Unload both shaders, `ColorLUTUninit`, `UnloadPingPong`.

**CurlAdvectionConfigDefault**: `return CurlAdvectionConfig{};`

**CurlAdvectionRegisterParams**: Copy verbatim from current `src/simulation/curl_advection.cpp` (all `ModEngineRegisterParam` calls).

**Bridge functions** (non-static):
- `SetupCurlAdvection(PostEffect *pe)`: calls `CurlAdvectionEffectSetup`, passes `pe->accumTexture.texture` to store as `currentAccumTexture`
- `SetupCurlAdvectionBlend(PostEffect *pe)`: calls `BlendCompositorApply` with `pe->curlAdvection.pingPong[pe->curlAdvection.readIdx].texture`, `pe->effects.curlAdvection.boostIntensity`, `pe->effects.curlAdvection.blendMode`
- `RenderCurlAdvection(PostEffect *pe)`: calls `CurlAdvectionEffectRender`

**UI section** (`// === UI ===`): Copy the `DrawCurlAdvectionParams` function verbatim from current `src/simulation/curl_advection.cpp`. It already has the correct signature `(EffectConfig*, const ModSources*, ImU32)`.

**Registration**:
```cpp
// clang-format off
STANDARD_GENERATOR_OUTPUT(curlAdvection)
REGISTER_GENERATOR_FULL(TRANSFORM_CURL_ADVECTION, CurlAdvection, curlAdvection,
                        "Curl Advection", SetupCurlAdvectionBlend,
                        SetupCurlAdvection, RenderCurlAdvection, 13,
                        DrawCurlAdvectionParams, DrawOutput_curlAdvection)
// clang-format on
```

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 2.4: Integration and cleanup

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify), `src/render/render_pipeline.cpp` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete
**Deletes**: `src/simulation/curl_advection.h`, `src/simulation/curl_advection.cpp`, `shaders/curl_advection.glsl`

**Do**:

1. **`src/config/effect_config.h`**: Change `#include "simulation/curl_advection.h"` to `#include "effects/curl_advection.h"`. The `TRANSFORM_CURL_ADVECTION` enum value, `CurlAdvectionConfig curlAdvection` field, and order array entry all remain unchanged.

2. **`src/render/post_effect.h`**: Remove `typedef struct CurlAdvection CurlAdvection;` forward declaration. The `CurlAdvection *curlAdvection;` pointer becomes `CurlAdvectionEffect curlAdvection;` (embedded struct, no pointer). The include for `curl_advection.h` comes from `effect_config.h` (already included).

3. **`src/render/post_effect.cpp`**: Remove `#include "simulation/curl_advection.h"`. Remove `CurlAdvectionInit` call from `PostEffectInit`. Remove `CurlAdvectionUninit` call from `PostEffectUninit`. Remove `CurlAdvectionResize` call from `PostEffectResize`. In `PostEffectClearFeedback`: remove the `CurlAdvectionReset` block; add clearing of the new ping-pong textures (both `curlAdvection.statePingPong[0..1]` and `curlAdvection.pingPong[0..1]`), reset indices, and re-initialize state with noise (call a reset helper or inline the clear + noise upload).

4. **`src/render/render_pipeline.cpp`**: Remove `#include "simulation/curl_advection.h"`. Remove the entire `ApplyCurlAdvectionPass` function. Remove its call from `ApplySimulationPasses`.

5. **`CMakeLists.txt`**: Replace `src/simulation/curl_advection.cpp` with `src/effects/curl_advection.cpp` in the source list.

6. **`src/config/effect_serialization.cpp`**: Change `#include "simulation/curl_advection.h"` to `#include "effects/curl_advection.h"`. The `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CurlAdvectionConfig, CURL_ADVECTION_CONFIG_FIELDS)` line stays unchanged. The `X(curlAdvection)` entry in `EFFECT_CONFIG_FIELDS` stays unchanged.

7. **Delete old files**: `src/simulation/curl_advection.h`, `src/simulation/curl_advection.cpp`, `shaders/curl_advection.glsl`.

**Verify**: `cmake.exe --build build` compiles with no errors. No references to `simulation/curl_advection` remain in the codebase.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Field category (section 13) with "GEN" badge
- [ ] Effect can be enabled, reordered via drag-drop
- [ ] UI controls modify effect in real-time (all PDE sliders responsive)
- [ ] Trail decay works (short decayHalfLife = crisp, long = smeared)
- [ ] Trail diffusion works (diffusionScale > 0 produces visible blur spread)
- [ ] Color modes work (solid/gradient/palette/rainbow via colorLUT)
- [ ] Injection from accumulation texture works when injectionIntensity > 0
- [ ] Blend compositor modes work (screen, additive, etc.)
- [ ] Preset save/load preserves all settings (field name `curlAdvection` unchanged)
- [ ] Modulation routes to all registered parameters
- [ ] No OpenGL 4.3 dependency — works on GL 3.3+ hardware
- [ ] Old simulation files deleted, no stale references

---

## Implementation Notes

1. **`texture0` convention**: Shaders must use `texture0` (not custom names) for the texture auto-bound by `DrawTextureRec`/`RenderUtilsDrawFullscreenQuad`. Additional textures are bound via `SetShaderValueTexture` to named uniforms. The state shader uses `texture0` for previous state, the color shader uses `texture0` for previous visual frame.

2. **`rlDisableColorBlend()` on state pass**: The state shader outputs `alpha=0` (`vec4(result, 0.0)`). Without disabling alpha blending, raylib's draw call blends the output to nothing. Must wrap the state pass in `rlDisableColorBlend()`/`rlEnableColorBlend()`, matching the pattern used by byzantine and other simulation generators.

3. **Zero-padding at edges**: The compute shader's `imageLoad` returned zero for out-of-bounds reads. The fragment shader's `TEXTURE_WRAP_CLAMP` repeats edge values, causing artificial edge activity. Fixed by adding a bounds check in `sampleState()` that returns `vec3(0)` outside `[0,1]`.

4. **Diffusion kernel energy conservation**: The cross-kernel blur (H+V 5-tap) must use `(h + v) * 0.5` to sum to 1.0. The original formula `h + v - center*W2` summed to 1.625, causing blowout to white.

5. **Custom `DrawOutput`**: `STANDARD_GENERATOR_OUTPUT` expects `gradient` and `blendIntensity` fields. CurlAdvection uses `color` and `boostIntensity`, so a manual `DrawOutput_curlAdvection` function replaces the macro.

6. **`CurlAdvectionEffectReset`**: Added public reset function (not in original plan) to re-seed state noise on `PostEffectClearFeedback`. Clearing to black (zero velocity) leaves the simulation dead.
