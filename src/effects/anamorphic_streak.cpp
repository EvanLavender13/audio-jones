// Anamorphic streak effect module implementation

#include "anamorphic_streak.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

static void InitMips(AnamorphicStreakEffect *e, int width, int height) {
  int w = width / 2;
  const int h = height / 2;
  for (int i = 0; i < STREAK_MIP_COUNT; i++) {
    RenderUtilsInitTextureHDR(&e->mips[i], w, h, "STREAK");
    RenderUtilsInitTextureHDR(&e->mipsUp[i], w, h, "STREAK_UP");
    w /= 2;
    if (w < 1) {
      w = 1;
    }
  }
}

static void UnloadMips(const AnamorphicStreakEffect *e) {
  for (int i = 0; i < STREAK_MIP_COUNT; i++) {
    UnloadRenderTexture(e->mips[i]);
    UnloadRenderTexture(e->mipsUp[i]);
  }
}

bool AnamorphicStreakEffectInit(AnamorphicStreakEffect *e, int width,
                                int height) {
  e->prefilterShader =
      LoadShader(NULL, "shaders/anamorphic_streak_prefilter.fs");
  if (e->prefilterShader.id == 0) {
    return false;
  }

  e->downsampleShader =
      LoadShader(NULL, "shaders/anamorphic_streak_downsample.fs");
  if (e->downsampleShader.id == 0) {
    UnloadShader(e->prefilterShader);
    return false;
  }

  e->upsampleShader = LoadShader(NULL, "shaders/anamorphic_streak_upsample.fs");
  if (e->upsampleShader.id == 0) {
    UnloadShader(e->prefilterShader);
    UnloadShader(e->downsampleShader);
    return false;
  }

  e->compositeShader =
      LoadShader(NULL, "shaders/anamorphic_streak_composite.fs");
  if (e->compositeShader.id == 0) {
    UnloadShader(e->prefilterShader);
    UnloadShader(e->downsampleShader);
    UnloadShader(e->upsampleShader);
    return false;
  }

  // Prefilter shader uniform locations
  e->thresholdLoc = GetShaderLocation(e->prefilterShader, "threshold");
  e->kneeLoc = GetShaderLocation(e->prefilterShader, "knee");

  // Downsample shader uniform locations
  e->downsampleTexelLoc = GetShaderLocation(e->downsampleShader, "texelSize");

  // Upsample shader uniform locations
  e->upsampleTexelLoc = GetShaderLocation(e->upsampleShader, "texelSize");
  e->highResTexLoc = GetShaderLocation(e->upsampleShader, "highResTex");
  e->stretchLoc = GetShaderLocation(e->upsampleShader, "stretch");

  // Composite shader uniform locations
  e->intensityLoc = GetShaderLocation(e->compositeShader, "intensity");
  e->tintLoc = GetShaderLocation(e->compositeShader, "tint");
  e->streakTexLoc = GetShaderLocation(e->compositeShader, "streakTexture");

  InitMips(e, width, height);

  return true;
}

void AnamorphicStreakEffectSetup(const AnamorphicStreakEffect *e,
                                 const AnamorphicStreakConfig *cfg) {
  SetShaderValue(e->compositeShader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  const float tint[3] = {cfg->tintR, cfg->tintG, cfg->tintB};
  SetShaderValue(e->compositeShader, e->tintLoc, tint, SHADER_UNIFORM_VEC3);
  SetShaderValueTexture(e->compositeShader, e->streakTexLoc,
                        e->mipsUp[0].texture);
}

void AnamorphicStreakEffectResize(AnamorphicStreakEffect *e, int width,
                                  int height) {
  UnloadMips(e);
  InitMips(e, width, height);
}

void AnamorphicStreakEffectUninit(const AnamorphicStreakEffect *e) {
  UnloadShader(e->prefilterShader);
  UnloadShader(e->downsampleShader);
  UnloadShader(e->upsampleShader);
  UnloadShader(e->compositeShader);
  UnloadMips(e);
}

void ApplyAnamorphicStreakPasses(PostEffect *pe,
                                 const RenderTexture2D *source) {
  const AnamorphicStreakConfig *a = &pe->effects.anamorphicStreak;
  AnamorphicStreakEffect *e = GetAnamorphicStreakEffect(pe);

  int iterations = a->iterations;
  if (iterations < 3) {
    iterations = 3;
  }
  if (iterations > STREAK_MIP_COUNT) {
    iterations = STREAK_MIP_COUNT;
  }

  // Prefilter: extract bright pixels from source into mips[0]
  SetShaderValue(e->prefilterShader, e->thresholdLoc, &a->threshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->prefilterShader, e->kneeLoc, &a->knee,
                 SHADER_UNIFORM_FLOAT);
  BeginTextureMode(e->mips[0]);
  BeginShaderMode(e->prefilterShader);
  DrawTexturePro(
      source->texture,
      {0, 0, (float)source->texture.width, (float)-source->texture.height},
      {0, 0, (float)e->mips[0].texture.width, (float)e->mips[0].texture.height},
      {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  // Downsample: mips[0] -> mips[1] -> ... -> mips[iterations-1]
  for (int i = 1; i < iterations; i++) {
    float texelSize = 1.0f / (float)e->mips[i - 1].texture.width;
    SetShaderValue(e->downsampleShader, e->downsampleTexelLoc, &texelSize,
                   SHADER_UNIFORM_FLOAT);

    BeginTextureMode(e->mips[i]);
    ClearBackground(BLACK);
    BeginShaderMode(e->downsampleShader);
    DrawTexturePro(e->mips[i - 1].texture,
                   {0, 0, (float)e->mips[i - 1].texture.width,
                    (float)-e->mips[i - 1].texture.height},
                   {0, 0, (float)e->mips[i].texture.width,
                    (float)e->mips[i].texture.height},
                   {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
  }

  // Upsample: walk back up the mip chain using separate down/up arrays.
  // Reads from mips[] (unmodified down chain), writes to mipsUp[].
  // Kino pattern: lastRT starts at the smallest mip, each level lerps
  // mips[i] (high-res) with upsampled lastRT, controlled by stretch.
  RenderTexture2D *lastRT = &e->mips[iterations - 1];
  for (int i = iterations - 2; i >= 0; i--) {
    float texelSize = 1.0f / (float)lastRT->texture.width;
    SetShaderValue(e->upsampleShader, e->upsampleTexelLoc, &texelSize,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->upsampleShader, e->stretchLoc, &a->stretch,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(e->upsampleShader, e->highResTexLoc,
                          e->mips[i].texture);

    BeginTextureMode(e->mipsUp[i]);
    ClearBackground(BLACK);
    BeginShaderMode(e->upsampleShader);
    DrawTexturePro(
        lastRT->texture,
        {0, 0, (float)lastRT->texture.width, (float)-lastRT->texture.height},
        {0, 0, (float)e->mipsUp[i].texture.width,
         (float)e->mipsUp[i].texture.height},
        {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    lastRT = &e->mipsUp[i];
  }

  // Final composite uses SetupAnamorphicStreak to bind uniforms, called by
  // render_pipeline
}

void AnamorphicStreakRegisterParams(AnamorphicStreakConfig *cfg) {
  ModEngineRegisterParam("anamorphicStreak.threshold", &cfg->threshold, 0.0f,
                         2.0f);
  ModEngineRegisterParam("anamorphicStreak.intensity", &cfg->intensity, 0.0f,
                         2.0f);
  ModEngineRegisterParam("anamorphicStreak.stretch", &cfg->stretch, 0.0f, 1.0f);
  ModEngineRegisterParam("anamorphicStreak.tintR", &cfg->tintR, 0.0f, 1.0f);
  ModEngineRegisterParam("anamorphicStreak.tintG", &cfg->tintG, 0.0f, 1.0f);
  ModEngineRegisterParam("anamorphicStreak.tintB", &cfg->tintB, 0.0f, 1.0f);
}

// Manual registration: custom GetShader (compositeShader) and Resize wrapper
static AnamorphicStreakEffect g_anamorphicStreakState;

static bool Init_anamorphicStreak(PostEffect *pe, int w, int h) {
  return AnamorphicStreakEffectInit(GetAnamorphicStreakEffect(pe), w, h);
}
static void Uninit_anamorphicStreak(PostEffect *pe) {
  AnamorphicStreakEffectUninit(GetAnamorphicStreakEffect(pe));
}
static void Resize_anamorphicStreak(PostEffect *pe, int w, int h) {
  AnamorphicStreakEffectResize(GetAnamorphicStreakEffect(pe), w, h);
}
static void Register_anamorphicStreak(EffectConfig *cfg) {
  AnamorphicStreakRegisterParams(&cfg->anamorphicStreak);
}
static Shader *GetShader_anamorphicStreak(PostEffect *pe) {
  return &GetAnamorphicStreakEffect(pe)->compositeShader;
}

AnamorphicStreakEffect *GetAnamorphicStreakEffect(PostEffect *pe) {
  return (AnamorphicStreakEffect *)
      pe->effectStates[TRANSFORM_ANAMORPHIC_STREAK];
}

void SetupAnamorphicStreak(PostEffect *pe) {
  AnamorphicStreakEffectSetup(GetAnamorphicStreakEffect(pe),
                              &pe->effects.anamorphicStreak);
}

// === UI ===

static void DrawAnamorphicStreakParams(EffectConfig *e, const ModSources *ms,
                                       ImU32 glow) {
  (void)glow;
  AnamorphicStreakConfig *a = &e->anamorphicStreak;

  ModulatableSlider("Threshold##anamorphicStreak", &a->threshold,
                    "anamorphicStreak.threshold", "%.2f", ms);
  ImGui::SliderFloat("Knee##anamorphicStreak", &a->knee, 0.0f, 1.0f, "%.2f");
  ModulatableSlider("Intensity##anamorphicStreak", &a->intensity,
                    "anamorphicStreak.intensity", "%.2f", ms);
  ModulatableSlider("Stretch##anamorphicStreak", &a->stretch,
                    "anamorphicStreak.stretch", "%.2f", ms);
  ImGui::ColorEdit3("Tint##anamorphicStreak", &a->tintR);
  ImGui::SliderInt("Iterations##anamorphicStreak", &a->iterations, 3, 7);
}

// clang-format off
static bool reg_anamorphicStreak = EffectDescriptorRegister(
    TRANSFORM_ANAMORPHIC_STREAK,
    EffectDescriptor{TRANSFORM_ANAMORPHIC_STREAK, "Anamorphic Streak", "OPT", 7,
     offsetof(EffectConfig, anamorphicStreak.enabled), "anamorphicStreak.",
     (uint8_t)(EFFECT_FLAG_NEEDS_RESIZE),
     Init_anamorphicStreak, Uninit_anamorphicStreak, Resize_anamorphicStreak,
     Register_anamorphicStreak, GetShader_anamorphicStreak,
     SetupAnamorphicStreak,
     nullptr, nullptr, nullptr,
     DrawAnamorphicStreakParams, nullptr,
     &g_anamorphicStreakState});
// clang-format on
