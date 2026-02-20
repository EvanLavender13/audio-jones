// Anamorphic streak effect module implementation

#include "anamorphic_streak.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
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

static void UnloadMips(AnamorphicStreakEffect *e) {
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

void AnamorphicStreakEffectSetup(AnamorphicStreakEffect *e,
                                 const AnamorphicStreakConfig *cfg) {
  SetShaderValue(e->compositeShader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  float tint[3] = {cfg->tintR, cfg->tintG, cfg->tintB};
  SetShaderValue(e->compositeShader, e->tintLoc, tint, SHADER_UNIFORM_VEC3);
  SetShaderValueTexture(e->compositeShader, e->streakTexLoc,
                        e->mipsUp[0].texture);
}

void AnamorphicStreakEffectResize(AnamorphicStreakEffect *e, int width,
                                  int height) {
  UnloadMips(e);
  InitMips(e, width, height);
}

void AnamorphicStreakEffectUninit(AnamorphicStreakEffect *e) {
  UnloadShader(e->prefilterShader);
  UnloadShader(e->downsampleShader);
  UnloadShader(e->upsampleShader);
  UnloadShader(e->compositeShader);
  UnloadMips(e);
}

AnamorphicStreakConfig AnamorphicStreakConfigDefault(void) {
  return AnamorphicStreakConfig{};
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
static bool Init_anamorphicStreak(PostEffect *pe, int w, int h) {
  return AnamorphicStreakEffectInit(&pe->anamorphicStreak, w, h);
}
static void Uninit_anamorphicStreak(PostEffect *pe) {
  AnamorphicStreakEffectUninit(&pe->anamorphicStreak);
}
static void Resize_anamorphicStreak(PostEffect *pe, int w, int h) {
  AnamorphicStreakEffectResize(&pe->anamorphicStreak, w, h);
}
static void Register_anamorphicStreak(EffectConfig *cfg) {
  AnamorphicStreakRegisterParams(&cfg->anamorphicStreak);
}
static Shader *GetShader_anamorphicStreak(PostEffect *pe) {
  return &pe->anamorphicStreak.compositeShader;
}

void SetupAnamorphicStreak(PostEffect *pe) {
  AnamorphicStreakEffectSetup(&pe->anamorphicStreak,
                              &pe->effects.anamorphicStreak);
}

// clang-format off
static bool reg_anamorphicStreak = EffectDescriptorRegister(
    TRANSFORM_ANAMORPHIC_STREAK,
    EffectDescriptor{TRANSFORM_ANAMORPHIC_STREAK, "Anamorphic Streak", "OPT", 7,
     offsetof(EffectConfig, anamorphicStreak.enabled),
     (uint8_t)(EFFECT_FLAG_NEEDS_RESIZE),
     Init_anamorphicStreak, Uninit_anamorphicStreak, Resize_anamorphicStreak,
     Register_anamorphicStreak, GetShader_anamorphicStreak,
     SetupAnamorphicStreak});
// clang-format on
