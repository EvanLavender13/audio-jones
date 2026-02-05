// Anamorphic streak effect module implementation

#include "anamorphic_streak.h"
#include "automation/modulation_engine.h"
#include <stddef.h>

bool AnamorphicStreakEffectInit(AnamorphicStreakEffect *e) {
  e->prefilterShader =
      LoadShader(NULL, "shaders/anamorphic_streak_prefilter.fs");
  if (e->prefilterShader.id == 0) {
    return false;
  }

  e->blurShader = LoadShader(NULL, "shaders/anamorphic_streak_blur.fs");
  if (e->blurShader.id == 0) {
    UnloadShader(e->prefilterShader);
    return false;
  }

  e->compositeShader =
      LoadShader(NULL, "shaders/anamorphic_streak_composite.fs");
  if (e->compositeShader.id == 0) {
    UnloadShader(e->prefilterShader);
    UnloadShader(e->blurShader);
    return false;
  }

  // Prefilter shader uniform locations
  e->thresholdLoc = GetShaderLocation(e->prefilterShader, "threshold");
  e->kneeLoc = GetShaderLocation(e->prefilterShader, "knee");

  // Blur shader uniform locations
  e->resolutionLoc = GetShaderLocation(e->blurShader, "resolution");
  e->offsetLoc = GetShaderLocation(e->blurShader, "offset");
  e->sharpnessLoc = GetShaderLocation(e->blurShader, "sharpness");

  // Composite shader uniform locations
  e->intensityLoc = GetShaderLocation(e->compositeShader, "intensity");
  e->streakTexLoc = GetShaderLocation(e->compositeShader, "streakTexture");

  return true;
}

void AnamorphicStreakEffectSetup(AnamorphicStreakEffect *e,
                                 const AnamorphicStreakConfig *cfg,
                                 Texture2D streakTex) {
  SetShaderValue(e->compositeShader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->compositeShader, e->streakTexLoc, streakTex);
}

void AnamorphicStreakEffectUninit(AnamorphicStreakEffect *e) {
  UnloadShader(e->prefilterShader);
  UnloadShader(e->blurShader);
  UnloadShader(e->compositeShader);
}

AnamorphicStreakConfig AnamorphicStreakConfigDefault(void) {
  return AnamorphicStreakConfig{};
}

void AnamorphicStreakRegisterParams(AnamorphicStreakConfig *cfg) {
  ModEngineRegisterParam("anamorphicStreak.threshold", &cfg->threshold, 0.0f,
                         2.0f);
  ModEngineRegisterParam("anamorphicStreak.intensity", &cfg->intensity, 0.0f,
                         2.0f);
  ModEngineRegisterParam("anamorphicStreak.stretch", &cfg->stretch, 0.5f, 8.0f);
  ModEngineRegisterParam("anamorphicStreak.sharpness", &cfg->sharpness, 0.0f,
                         1.0f);
}
