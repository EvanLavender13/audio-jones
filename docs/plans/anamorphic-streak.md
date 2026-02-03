# Anamorphic Streak

Horizontal light streaks extending from bright areas, simulating the optical artifact from anamorphic cinema lenses. Uses threshold extraction followed by iterative horizontal Kawase blur with configurable sharpness control.

## Specification

### Types

```cpp
// src/config/anamorphic_streak_config.h
#ifndef ANAMORPHIC_STREAK_CONFIG_H
#define ANAMORPHIC_STREAK_CONFIG_H

struct AnamorphicStreakConfig {
  bool enabled = false;
  float threshold = 0.8f;  // Brightness cutoff (0.0-2.0)
  float knee = 0.5f;       // Soft threshold falloff (0.0-1.0)
  float intensity = 0.5f;  // Streak brightness in composite (0.0-2.0)
  float stretch = 8.0f;    // Horizontal extent multiplier (1.0-20.0)
  float sharpness = 0.3f;  // Kernel falloff: soft (0) to hard lines (1) (0.0-1.0)
  int iterations = 4;      // Blur pass count (2-6)
};

#endif // ANAMORPHIC_STREAK_CONFIG_H
```

### Constants

- Enum: `TRANSFORM_ANAMORPHIC_STREAK`
- Display name: `"Anamorphic Streak"`
- Category: `TRANSFORM_CATEGORY_OPTICAL` (section 7, badge "OPT")

### Algorithm

**Prefilter Shader** (`anamorphic_streak_prefilter.fs`):
Reuses Bloom's soft threshold extraction pattern.

<!-- Intentional deviation: Research uses luminance-weighted dot product for brightness,
     but we use max-channel brightness to match existing Bloom implementation. -->

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float threshold;
uniform float knee;

void main() {
    vec3 color = texture(texture0, fragTexCoord).rgb;
    float brightness = max(color.r, max(color.g, color.b));
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);
    finalColor = vec4(color * contribution, 1.0);
}
```

**Horizontal Blur Shader** (`anamorphic_streak_blur.fs`):
Single-pass horizontal Kawase blur with sharpness-controlled weights.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float offset;     // Current iteration offset in pixels
uniform float sharpness;  // 0.0 = soft Kawase, 1.0 = flat weights

void main() {
    vec2 texelSize = 1.0 / resolution;
    float centerWeight = mix(0.5, 0.34, sharpness);
    float sideWeight = (1.0 - centerWeight) * 0.5;

    vec3 sum = texture(texture0, fragTexCoord).rgb * centerWeight;
    sum += texture(texture0, fragTexCoord + vec2(-offset * texelSize.x, 0.0)).rgb * sideWeight;
    sum += texture(texture0, fragTexCoord + vec2( offset * texelSize.x, 0.0)).rgb * sideWeight;

    finalColor = vec4(sum, 1.0);
}
```

**Composite Shader** (`anamorphic_streak_composite.fs`):
Additive blend with intensity control.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // Original image
uniform sampler2D streakTexture; // Blurred streak
uniform float intensity;

void main() {
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 streak = texture(streakTexture, fragTexCoord).rgb;
    finalColor = vec4(original + streak * intensity, 1.0);
}
```

**Render Pass Sequence** (in `ApplyAnamorphicStreakPasses`):

1. Prefilter: source → halfResA (threshold extraction)
2. Blur iterations: ping-pong halfResA ↔ halfResB
   - Pass i: offset = (i + 0.5) * stretch
3. Composite: original + final blur result

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| enabled | bool | - | false | No | Enabled |
| threshold | float | 0.0-2.0 | 0.8 | Yes | Threshold |
| knee | float | 0.0-1.0 | 0.5 | No | Knee |
| intensity | float | 0.0-2.0 | 0.5 | Yes | Intensity |
| stretch | float | 1.0-20.0 | 8.0 | Yes | Stretch |
| sharpness | float | 0.0-1.0 | 0.3 | Yes | Sharpness |
| iterations | int | 2-6 | 4 | No | Iterations |

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create config struct

**Files**: `src/config/anamorphic_streak_config.h`
**Creates**: `AnamorphicStreakConfig` struct

**Build**:
1. Create header with include guard `ANAMORPHIC_STREAK_CONFIG_H`
2. Define `AnamorphicStreakConfig` struct matching specification Types section
3. All fields have default values as specified

**Verify**: File exists with correct struct definition.

---

### Wave 2: Effect Registration & Shaders

#### Task 2.1: Register effect in effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "anamorphic_streak_config.h"` with other config includes
2. Add `TRANSFORM_ANAMORPHIC_STREAK,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add case in `TransformEffectName()`: `case TRANSFORM_ANAMORPHIC_STREAK: return "Anamorphic Streak";`
4. Add `TRANSFORM_ANAMORPHIC_STREAK` to `TransformOrderConfig::order` array (at end before closing brace)
5. Add member to `EffectConfig`: `AnamorphicStreakConfig anamorphicStreak;`
6. Add case in `IsTransformEnabled()`: `case TRANSFORM_ANAMORPHIC_STREAK: return e->anamorphicStreak.enabled;`

**Verify**: Compiles with no errors.

#### Task 2.2: Create prefilter shader

**Files**: `shaders/anamorphic_streak_prefilter.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create shader file with GLSL 330 version
2. Implement soft threshold extraction matching specification Algorithm section
3. Uniforms: `texture0`, `threshold`, `knee`

**Verify**: File exists with correct shader code.

#### Task 2.3: Create horizontal blur shader

**Files**: `shaders/anamorphic_streak_blur.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create shader file with GLSL 330 version
2. Implement horizontal Kawase blur with sharpness-controlled weights
3. Uniforms: `texture0`, `resolution`, `offset`, `sharpness`

**Verify**: File exists with correct shader code.

#### Task 2.4: Create composite shader

**Files**: `shaders/anamorphic_streak_composite.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create shader file with GLSL 330 version
2. Implement additive blend with intensity control
3. Uniforms: `texture0`, `streakTexture`, `intensity`

**Verify**: File exists with correct shader code.

---

### Wave 3: PostEffect Integration

#### Task 3.1: Add shader members and uniform locations

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 2 complete

**Build** (post_effect.h):
1. Add shader members after other optical shaders:
   ```cpp
   Shader anamorphicStreakPrefilterShader;
   Shader anamorphicStreakBlurShader;
   Shader anamorphicStreakCompositeShader;
   ```
2. Add uniform location members after bloom locations:
   ```cpp
   int anamorphicStreakThresholdLoc;
   int anamorphicStreakKneeLoc;
   int anamorphicStreakResolutionLoc;
   int anamorphicStreakOffsetLoc;
   int anamorphicStreakSharpnessLoc;
   int anamorphicStreakIntensityLoc;
   int anamorphicStreakStreakTexLoc;
   ```

**Build** (post_effect.cpp):
1. In `LoadPostEffectShaders()`, load all three shaders after bloom shaders:
   ```cpp
   pe->anamorphicStreakPrefilterShader = LoadShader(0, "shaders/anamorphic_streak_prefilter.fs");
   pe->anamorphicStreakBlurShader = LoadShader(0, "shaders/anamorphic_streak_blur.fs");
   pe->anamorphicStreakCompositeShader = LoadShader(0, "shaders/anamorphic_streak_composite.fs");
   ```
2. Add to success check (all three shader IDs != 0)
3. In `GetShaderUniformLocations()`, get all uniform locations:
   ```cpp
   pe->anamorphicStreakThresholdLoc = GetShaderLocation(pe->anamorphicStreakPrefilterShader, "threshold");
   pe->anamorphicStreakKneeLoc = GetShaderLocation(pe->anamorphicStreakPrefilterShader, "knee");
   pe->anamorphicStreakResolutionLoc = GetShaderLocation(pe->anamorphicStreakBlurShader, "resolution");
   pe->anamorphicStreakOffsetLoc = GetShaderLocation(pe->anamorphicStreakBlurShader, "offset");
   pe->anamorphicStreakSharpnessLoc = GetShaderLocation(pe->anamorphicStreakBlurShader, "sharpness");
   pe->anamorphicStreakIntensityLoc = GetShaderLocation(pe->anamorphicStreakCompositeShader, "intensity");
   pe->anamorphicStreakStreakTexLoc = GetShaderLocation(pe->anamorphicStreakCompositeShader, "streakTexture");
   ```
4. In `PostEffectUninit()`, unload all three shaders

**Verify**: Compiles with no errors.

---

### Wave 4: Shader Setup & Pipeline

#### Task 4.1: Add shader setup function

**Files**: `src/render/shader_setup_optical.h`, `src/render/shader_setup_optical.cpp`
**Depends on**: Wave 3 complete

**Build** (shader_setup_optical.h):
1. Add declaration: `void SetupAnamorphicStreak(PostEffect *pe);`

**Build** (shader_setup_optical.cpp):
1. Add `#include "config/anamorphic_streak_config.h"` if not present
2. Implement `SetupAnamorphicStreak()`:
   ```cpp
   void SetupAnamorphicStreak(PostEffect *pe) {
     const AnamorphicStreakConfig *a = &pe->effects.anamorphicStreak;
     SetShaderValue(pe->anamorphicStreakCompositeShader, pe->anamorphicStreakIntensityLoc,
                    &a->intensity, SHADER_UNIFORM_FLOAT);
     SetShaderValueTexture(pe->anamorphicStreakCompositeShader, pe->anamorphicStreakStreakTexLoc,
                           pe->halfResA.texture);
   }
   ```

**Verify**: Compiles with no errors.

#### Task 4.2: Add render pipeline dispatch and passes

**Files**: `src/render/shader_setup.cpp`, `src/render/render_pipeline.cpp`
**Depends on**: Task 4.1 complete

**Build** (shader_setup.cpp):
1. Add `#include "config/anamorphic_streak_config.h"` if not present
2. Add dispatch case in `GetTransformEffect()`:
   ```cpp
   case TRANSFORM_ANAMORPHIC_STREAK:
     return {&pe->anamorphicStreakCompositeShader, SetupAnamorphicStreak,
             &pe->effects.anamorphicStreak.enabled};
   ```
3. Add `ApplyAnamorphicStreakPasses()` function (after `ApplyBloomPasses`):
   ```cpp
   void ApplyAnamorphicStreakPasses(PostEffect *pe, RenderTexture2D *source) {
     const AnamorphicStreakConfig *a = &pe->effects.anamorphicStreak;
     int iterations = a->iterations;
     if (iterations < 2) iterations = 2;
     if (iterations > 6) iterations = 6;

     const int halfW = pe->screenWidth / 2;
     const int halfH = pe->screenHeight / 2;

     // Prefilter: source -> halfResA
     SetShaderValue(pe->anamorphicStreakPrefilterShader, pe->anamorphicStreakThresholdLoc,
                    &a->threshold, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->anamorphicStreakPrefilterShader, pe->anamorphicStreakKneeLoc,
                    &a->knee, SHADER_UNIFORM_FLOAT);

     BeginTextureMode(pe->halfResA);
     BeginShaderMode(pe->anamorphicStreakPrefilterShader);
     DrawTexturePro(source->texture,
                    {0, 0, (float)source->texture.width, (float)-source->texture.height},
                    {0, 0, (float)halfW, (float)halfH}, {0, 0}, 0.0f, WHITE);
     EndShaderMode();
     EndTextureMode();

     // Blur passes: ping-pong halfResA <-> halfResB
     float halfRes[2] = {(float)halfW, (float)halfH};
     SetShaderValue(pe->anamorphicStreakBlurShader, pe->anamorphicStreakResolutionLoc,
                    halfRes, SHADER_UNIFORM_VEC2);
     SetShaderValue(pe->anamorphicStreakBlurShader, pe->anamorphicStreakSharpnessLoc,
                    &a->sharpness, SHADER_UNIFORM_FLOAT);

     RenderTexture2D *readTex = &pe->halfResA;
     RenderTexture2D *writeTex = &pe->halfResB;

     for (int i = 0; i < iterations; i++) {
       float offset = (float)(i + 0.5f) * a->stretch;
       SetShaderValue(pe->anamorphicStreakBlurShader, pe->anamorphicStreakOffsetLoc,
                      &offset, SHADER_UNIFORM_FLOAT);

       BeginTextureMode(*writeTex);
       BeginShaderMode(pe->anamorphicStreakBlurShader);
       DrawTexturePro(readTex->texture,
                      {0, 0, (float)halfW, (float)-halfH},
                      {0, 0, (float)halfW, (float)halfH}, {0, 0}, 0.0f, WHITE);
       EndShaderMode();
       EndTextureMode();

       // Swap for next iteration
       RenderTexture2D *temp = readTex;
       readTex = writeTex;
       writeTex = temp;
     }

     // Result is in readTex (last write destination after swap)
     // Copy to halfResA if needed for SetupAnamorphicStreak
     if (readTex != &pe->halfResA) {
       BeginTextureMode(pe->halfResA);
       DrawTexturePro(readTex->texture,
                      {0, 0, (float)halfW, (float)-halfH},
                      {0, 0, (float)halfW, (float)halfH}, {0, 0}, 0.0f, WHITE);
       EndTextureMode();
     }
   }
   ```
4. Add declaration in header or as `extern` in shader_setup.h

**Build** (render_pipeline.cpp):
1. Add case in transform loop (after TRANSFORM_BLOOM case):
   ```cpp
   } else if (effectType == TRANSFORM_ANAMORPHIC_STREAK) {
     ApplyAnamorphicStreakPasses(pe, src);
     RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
   ```

**Verify**: Compiles with no errors.

---

### Wave 5: UI & Serialization

#### Task 5.1: Add category mapping

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add case in `GetTransformCategory()` under Optical section (after HEIGHTFIELD_RELIEF):
   ```cpp
   case TRANSFORM_ANAMORPHIC_STREAK:
   ```
   (already returns `{"OPT", 7}` from the existing Optical group)

**Verify**: Compiles with no errors.

#### Task 5.2: Add UI controls

**Files**: `src/ui/imgui_effects_optical.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add `#include "config/anamorphic_streak_config.h"`
2. Add section state: `static bool sectionAnamorphicStreak = false;`
3. Add helper function before `DrawOpticalCategory`:
   ```cpp
   static void DrawOpticalAnamorphicStreak(EffectConfig *e, const ModSources *modSources,
                                           const ImU32 categoryGlow) {
     if (DrawSectionBegin("Anamorphic Streak", categoryGlow, &sectionAnamorphicStreak)) {
       const bool wasEnabled = e->anamorphicStreak.enabled;
       ImGui::Checkbox("Enabled##anamorphicStreak", &e->anamorphicStreak.enabled);
       if (!wasEnabled && e->anamorphicStreak.enabled) {
         MoveTransformToEnd(&e->transformOrder, TRANSFORM_ANAMORPHIC_STREAK);
       }
       if (e->anamorphicStreak.enabled) {
         AnamorphicStreakConfig *a = &e->anamorphicStreak;

         ModulatableSlider("Threshold##anamorphicStreak", &a->threshold,
                           "anamorphicStreak.threshold", "%.2f", modSources);
         ImGui::SliderFloat("Knee##anamorphicStreak", &a->knee, 0.0f, 1.0f, "%.2f");
         ModulatableSlider("Intensity##anamorphicStreak", &a->intensity,
                           "anamorphicStreak.intensity", "%.2f", modSources);
         ModulatableSlider("Stretch##anamorphicStreak", &a->stretch,
                           "anamorphicStreak.stretch", "%.1f", modSources);
         ModulatableSlider("Sharpness##anamorphicStreak", &a->sharpness,
                           "anamorphicStreak.sharpness", "%.2f", modSources);
         ImGui::SliderInt("Iterations##anamorphicStreak", &a->iterations, 2, 6);
       }
       DrawSectionEnd();
     }
   }
   ```
4. Add call in `DrawOpticalCategory` after HeightfieldRelief:
   ```cpp
   ImGui::Spacing();
   DrawOpticalAnamorphicStreak(e, modSources, categoryGlow);
   ```

**Verify**: Compiles with no errors.

#### Task 5.3: Add preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add `#include "anamorphic_streak_config.h"` with other config includes
2. Add JSON macro after BloomConfig:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AnamorphicStreakConfig, enabled,
                                                   threshold, knee, intensity, stretch,
                                                   sharpness, iterations)
   ```
3. Add to_json entry after bloom:
   ```cpp
   if (e.anamorphicStreak.enabled) {
     j["anamorphicStreak"] = e.anamorphicStreak;
   }
   ```
4. Add from_json entry after bloom:
   ```cpp
   e.anamorphicStreak = j.value("anamorphicStreak", e.anamorphicStreak);
   ```

**Verify**: Compiles with no errors.

#### Task 5.4: Add parameter registration

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add entries to PARAM_TABLE after bloom entries:
   ```cpp
   {"anamorphicStreak.threshold",
    {0.0f, 2.0f},
    offsetof(EffectConfig, anamorphicStreak.threshold)},
   {"anamorphicStreak.intensity",
    {0.0f, 2.0f},
    offsetof(EffectConfig, anamorphicStreak.intensity)},
   {"anamorphicStreak.stretch",
    {1.0f, 20.0f},
    offsetof(EffectConfig, anamorphicStreak.stretch)},
   {"anamorphicStreak.sharpness",
    {0.0f, 1.0f},
    offsetof(EffectConfig, anamorphicStreak.sharpness)},
   ```

**Verify**: Compiles with no errors.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release` succeeds
- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in transform order pipeline with "OPT" badge
- [ ] Enabling effect adds it to pipeline list
- [ ] UI sliders modify effect parameters in real-time
- [ ] Horizontal streaks extend from bright areas
- [ ] Sharpness slider changes streak character (soft glow to hard lines)
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters (threshold, intensity, stretch, sharpness)
