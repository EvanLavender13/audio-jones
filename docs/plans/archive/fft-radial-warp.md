# FFT Radial Warp

Radial UV displacement driven by real-time FFT magnitude data. Bass frequencies warp the center, treble warps the edges. Bidirectional push/pull via angular segments creates flower-like interference patterns that breathe with the music. Includes FFT texture throttling infrastructure to reduce jitter from per-frame updates.

## Specification

### Infrastructure: FFT Texture Throttling

Move FFT texture update from per-frame (`render_pipeline.cpp:267`) into the 20hz-gated path. The drawable waveform processing already runs at 20hz via `UpdateVisuals()` in `main.cpp:137-144`.

**Implementation**: Add `fftTexture` update call inside `UpdateVisuals()` alongside existing drawable processing. Requires passing the PostEffect pointer and FFT magnitude array to `UpdateVisuals()`.

### Types

#### FftRadialWarpConfig (`src/config/fft_radial_warp_config.h`)

```cpp
#ifndef FFT_RADIAL_WARP_CONFIG_H
#define FFT_RADIAL_WARP_CONFIG_H

// FFT Radial Warp: audio-reactive radial displacement
// Maps FFT bins to screen radius - bass at center, treble at edges.
// Angular segments create bidirectional push/pull patterns.
struct FftRadialWarpConfig {
  bool enabled = false;
  float intensity = 0.1f;      // Displacement strength (0.0 - 0.5)
  float freqStart = 0.0f;      // FFT bin at center, 0 = bass (0.0 - 1.0)
  float freqEnd = 0.5f;        // FFT bin at maxRadius, 0.5 = mids (0.0 - 1.0)
  float maxRadius = 0.7f;      // Screen radius mapping to freqEnd (0.1 - 1.0)
  int segments = 4;            // Angular divisions for push/pull (1 - 16)
  float pushPullPhase = 0.0f;  // Rotates push/pull pattern (radians)
};

#endif // FFT_RADIAL_WARP_CONFIG_H
```

### Algorithm (Shader)

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D fftTexture;  // 1D FFT magnitudes (FFT_BIN_COUNT x 1)

uniform float intensity;
uniform float freqStart;
uniform float freqEnd;
uniform float maxRadius;
uniform int segments;
uniform float pushPullPhase;

const float TAU = 6.283185307;

void main() {
    vec2 center = vec2(0.5);
    vec2 delta = fragTexCoord - center;
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    // Map radius [0, maxRadius] to FFT bin range [freqStart, freqEnd]
    float freqCoord = clamp(radius / maxRadius, 0.0, 1.0);
    freqCoord = mix(freqStart, freqEnd, freqCoord);
    float magnitude = texture(fftTexture, vec2(freqCoord, 0.5)).r;

    // Bidirectional: angular segments alternate push/pull
    float segmentPhase = angle * float(segments) / TAU + pushPullPhase;
    float pushPull = mix(-1.0, 1.0, step(0.5, fract(segmentPhase)));

    // Compute radial displacement
    vec2 radialDir = (radius > 0.0001) ? normalize(delta) : vec2(1.0, 0.0);
    vec2 displacement = radialDir * magnitude * intensity * pushPull;

    vec2 sampleUV = fragTexCoord + displacement;
    finalColor = texture(texture0, sampleUV);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| intensity | float | 0.0 - 0.5 | 0.1 | Yes | Intensity |
| freqStart | float | 0.0 - 1.0 | 0.0 | Yes | Freq Start |
| freqEnd | float | 0.0 - 1.0 | 0.5 | Yes | Freq End |
| maxRadius | float | 0.1 - 1.0 | 0.7 | Yes | Max Radius |
| segments | int | 1 - 16 | 4 | No | Segments |
| pushPullPhase | float | 0.0 - TAU | 0.0 | Yes | Phase |

### Constants

- Enum name: `TRANSFORM_FFT_RADIAL_WARP`
- Display name: `"FFT Radial Warp"`
- Category: `TRANSFORM_CATEGORY_WARP`

---

## Tasks

### Wave 1: Infrastructure + Config

#### Task 1.1: FFT Texture Throttling

**Files**: `src/main.cpp`
**Creates**: 20hz-gated FFT texture updates

**Build**:
1. Modify `UpdateVisuals` signature to accept `PostEffect* pe` and `const float* fftMagnitude`:
   ```cpp
   static void UpdateVisuals(AppContext *ctx, PostEffect *pe, const float *fftMagnitude);
   ```
2. Inside `UpdateVisuals`, after the existing drawable processing, call:
   ```cpp
   UpdateFFTTexture(pe, fftMagnitude);
   ```
3. Update the call site in main loop (~line 206) to pass the additional arguments:
   ```cpp
   UpdateVisuals(ctx, ctx->postEffect, ctx->analysis.fft.magnitude);
   ```
4. In `RenderPipelineApplyFeedback` (`render_pipeline.cpp:267`), remove the `UpdateFFTTexture(pe, fftMagnitude);` call. This function still handles time accumulators.

**Verify**: Build succeeds. FFT texture now updates at 20hz instead of every frame.

#### Task 1.2: Config Header

**Files**: `src/config/fft_radial_warp_config.h`
**Creates**: `FftRadialWarpConfig` struct

**Build**:
1. Create the file with the struct definition from Specification > Types.

**Verify**: File exists with correct struct.

---

### Wave 2: Effect Registration

#### Task 2.1: Effect Config Integration

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.2 complete

**Build**:
1. Add include after other config headers (~line 67):
   ```cpp
   #include "fft_radial_warp_config.h"
   ```
2. Add enum entry before `TRANSFORM_EFFECT_COUNT` (~line 129):
   ```cpp
   TRANSFORM_FFT_RADIAL_WARP,
   ```
3. Add name in `TRANSFORM_EFFECT_NAMES` array (match enum position):
   ```cpp
   "FFT Radial Warp",        // TRANSFORM_FFT_RADIAL_WARP
   ```
4. Add to `TransformOrderConfig::order` initialization array.
5. Add config member to `EffectConfig` struct:
   ```cpp
   FftRadialWarpConfig fftRadialWarp;
   ```
6. Add case in `IsTransformEnabled()`:
   ```cpp
   case TRANSFORM_FFT_RADIAL_WARP: return e->fftRadialWarp.enabled;
   ```

**Verify**: Build succeeds.

---

### Wave 3: Shader + PostEffect

#### Task 3.1: Fragment Shader

**Files**: `shaders/fft_radial_warp.fs`
**Depends on**: Wave 2 complete

**Build**:
1. Create the shader file with the algorithm from Specification > Algorithm.

**Verify**: File exists with correct GLSL.

#### Task 3.2: PostEffect Shader Members

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 2 complete

**Build in post_effect.h**:
1. Add shader member in `PostEffect` struct (with other shaders):
   ```cpp
   Shader fftRadialWarpShader;
   ```
2. Add uniform location members:
   ```cpp
   int fftRadialWarpResolutionLoc;
   int fftRadialWarpFftTextureLoc;
   int fftRadialWarpIntensityLoc;
   int fftRadialWarpFreqStartLoc;
   int fftRadialWarpFreqEndLoc;
   int fftRadialWarpMaxRadiusLoc;
   int fftRadialWarpSegmentsLoc;
   int fftRadialWarpPushPullPhaseLoc;
   ```

**Build in post_effect.cpp**:
1. In `LoadPostEffectShaders()`, add:
   ```cpp
   pe->fftRadialWarpShader = LoadShader(0, "shaders/fft_radial_warp.fs");
   ```
2. Add to success check:
   ```cpp
   && pe->fftRadialWarpShader.id != 0
   ```
3. In `GetShaderUniformLocations()`, add:
   ```cpp
   pe->fftRadialWarpResolutionLoc = GetShaderLocation(pe->fftRadialWarpShader, "resolution");
   pe->fftRadialWarpFftTextureLoc = GetShaderLocation(pe->fftRadialWarpShader, "fftTexture");
   pe->fftRadialWarpIntensityLoc = GetShaderLocation(pe->fftRadialWarpShader, "intensity");
   pe->fftRadialWarpFreqStartLoc = GetShaderLocation(pe->fftRadialWarpShader, "freqStart");
   pe->fftRadialWarpFreqEndLoc = GetShaderLocation(pe->fftRadialWarpShader, "freqEnd");
   pe->fftRadialWarpMaxRadiusLoc = GetShaderLocation(pe->fftRadialWarpShader, "maxRadius");
   pe->fftRadialWarpSegmentsLoc = GetShaderLocation(pe->fftRadialWarpShader, "segments");
   pe->fftRadialWarpPushPullPhaseLoc = GetShaderLocation(pe->fftRadialWarpShader, "pushPullPhase");
   ```
4. In `SetResolutionUniforms()`, add:
   ```cpp
   SetShaderValue(pe->fftRadialWarpShader, pe->fftRadialWarpResolutionLoc, res, SHADER_UNIFORM_VEC2);
   ```
5. In `PostEffectUninit()`, add:
   ```cpp
   UnloadShader(pe->fftRadialWarpShader);
   ```

**Verify**: Build succeeds.

---

### Wave 4: Shader Setup + Dispatch

#### Task 4.1: Shader Setup Function

**Files**: `src/render/shader_setup_warp.h`, `src/render/shader_setup_warp.cpp`
**Depends on**: Wave 3 complete

**Build in shader_setup_warp.h**:
1. Add declaration:
   ```cpp
   void SetupFftRadialWarp(PostEffect* pe);
   ```

**Build in shader_setup_warp.cpp**:
1. Implement setup function:
   ```cpp
   void SetupFftRadialWarp(PostEffect *pe) {
     const FftRadialWarpConfig *cfg = &pe->effects.fftRadialWarp;
     SetShaderValue(pe->fftRadialWarpShader, pe->fftRadialWarpIntensityLoc,
                    &cfg->intensity, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->fftRadialWarpShader, pe->fftRadialWarpFreqStartLoc,
                    &cfg->freqStart, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->fftRadialWarpShader, pe->fftRadialWarpFreqEndLoc,
                    &cfg->freqEnd, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->fftRadialWarpShader, pe->fftRadialWarpMaxRadiusLoc,
                    &cfg->maxRadius, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->fftRadialWarpShader, pe->fftRadialWarpSegmentsLoc,
                    &cfg->segments, SHADER_UNIFORM_INT);
     SetShaderValue(pe->fftRadialWarpShader, pe->fftRadialWarpPushPullPhaseLoc,
                    &cfg->pushPullPhase, SHADER_UNIFORM_FLOAT);
     SetShaderValueTexture(pe->fftRadialWarpShader, pe->fftRadialWarpFftTextureLoc,
                           pe->fftTexture);
   }
   ```

**Verify**: Build succeeds.

#### Task 4.2: Dispatch Case

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Task 4.1 complete

**Build**:
1. Ensure `#include "shader_setup_warp.h"` is present (should already exist).
2. Add dispatch case in `GetTransformEffect()`:
   ```cpp
   case TRANSFORM_FFT_RADIAL_WARP:
       return { &pe->fftRadialWarpShader, SetupFftRadialWarp, &pe->effects.fftRadialWarp.enabled };
   ```

**Verify**: Build succeeds.

---

### Wave 5: UI + Serialization + Params

#### Task 5.1: UI Panel

**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 4 complete

**Build in imgui_effects.cpp**:
1. Add category mapping in `GetTransformCategory()`:
   ```cpp
   case TRANSFORM_FFT_RADIAL_WARP: return TRANSFORM_CATEGORY_WARP;
   ```

**Build in imgui_effects_warp.cpp**:
1. Add section state at top with others:
   ```cpp
   static bool sectionFftRadialWarp = false;
   ```
2. Add helper function before `DrawWarpCategory()`:
   ```cpp
   static void DrawWarpFftRadialWarp(EffectConfig *e, const ModSources *modSources,
                                      const ImU32 categoryGlow) {
     if (DrawSectionBegin("FFT Radial Warp", categoryGlow, &sectionFftRadialWarp)) {
       const bool wasEnabled = e->fftRadialWarp.enabled;
       ImGui::Checkbox("Enabled##fftradialwarp", &e->fftRadialWarp.enabled);
       if (!wasEnabled && e->fftRadialWarp.enabled) {
         MoveTransformToEnd(&e->transformOrder, TRANSFORM_FFT_RADIAL_WARP);
       }
       if (e->fftRadialWarp.enabled) {
         ModulatableSlider("Intensity##fftradialwarp", &e->fftRadialWarp.intensity,
                           "fftRadialWarp.intensity", "%.3f", modSources);
         ModulatableSlider("Freq Start##fftradialwarp", &e->fftRadialWarp.freqStart,
                           "fftRadialWarp.freqStart", "%.2f", modSources);
         ModulatableSlider("Freq End##fftradialwarp", &e->fftRadialWarp.freqEnd,
                           "fftRadialWarp.freqEnd", "%.2f", modSources);
         ModulatableSlider("Max Radius##fftradialwarp", &e->fftRadialWarp.maxRadius,
                           "fftRadialWarp.maxRadius", "%.2f", modSources);
         ImGui::SliderInt("Segments##fftradialwarp", &e->fftRadialWarp.segments, 1, 16);
         ModulatableSliderAngleDeg("Phase##fftradialwarp", &e->fftRadialWarp.pushPullPhase,
                                   "fftRadialWarp.pushPullPhase", modSources);
       }
       DrawSectionEnd();
     }
   }
   ```
3. Add call in `DrawWarpCategory()` with spacing:
   ```cpp
   ImGui::Spacing();
   DrawWarpFftRadialWarp(e, modSources, categoryGlow);
   ```

**Verify**: Build succeeds.

#### Task 5.2: Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 4 complete

**Build**:
1. Add JSON macro with other config macros:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FftRadialWarpConfig, enabled,
       intensity, freqStart, freqEnd, maxRadius, segments, pushPullPhase)
   ```
2. Add to_json entry in `to_json(json& j, const EffectConfig& e)`:
   ```cpp
   if (e.fftRadialWarp.enabled) { j["fftRadialWarp"] = e.fftRadialWarp; }
   ```
3. Add from_json entry in `from_json(const json& j, EffectConfig& e)`:
   ```cpp
   e.fftRadialWarp = j.value("fftRadialWarp", e.fftRadialWarp);
   ```

**Verify**: Build succeeds.

#### Task 5.3: Parameter Registration

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 4 complete

**Build**:
1. Add entries to `PARAM_TABLE`:
   ```cpp
   {"fftRadialWarp.intensity",
    {0.0f, 0.5f},
    offsetof(EffectConfig, fftRadialWarp.intensity)},
   {"fftRadialWarp.freqStart",
    {0.0f, 1.0f},
    offsetof(EffectConfig, fftRadialWarp.freqStart)},
   {"fftRadialWarp.freqEnd",
    {0.0f, 1.0f},
    offsetof(EffectConfig, fftRadialWarp.freqEnd)},
   {"fftRadialWarp.maxRadius",
    {0.1f, 1.0f},
    offsetof(EffectConfig, fftRadialWarp.maxRadius)},
   {"fftRadialWarp.pushPullPhase",
    {0.0f, ROTATION_OFFSET_MAX},
    offsetof(EffectConfig, fftRadialWarp.pushPullPhase)},
   ```

**Verify**: Build succeeds.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build`
- [ ] Effect appears in Warp category of Effects panel
- [ ] Effect shows "Warp" badge in pipeline list (not "???")
- [ ] Enabling effect adds it to transform pipeline
- [ ] UI sliders modify effect in real-time
- [ ] Audio reactivity visible: waveform warps with music
- [ ] FFT texture updates at 20hz (smoother than before)
- [ ] Preset save/load preserves settings
- [ ] LFO modulation routes to registered parameters
