// Neon glow effect module implementation

#include "neon_glow.h"
#include <stddef.h>

bool NeonGlowEffectInit(NeonGlowEffect *e) {
  e->shader = LoadShader(NULL, "shaders/neon_glow.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->glowColorLoc = GetShaderLocation(e->shader, "glowColor");
  e->edgeThresholdLoc = GetShaderLocation(e->shader, "edgeThreshold");
  e->edgePowerLoc = GetShaderLocation(e->shader, "edgePower");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->glowRadiusLoc = GetShaderLocation(e->shader, "glowRadius");
  e->glowSamplesLoc = GetShaderLocation(e->shader, "glowSamples");
  e->originalVisibilityLoc = GetShaderLocation(e->shader, "originalVisibility");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->saturationBoostLoc = GetShaderLocation(e->shader, "saturationBoost");
  e->brightnessBoostLoc = GetShaderLocation(e->shader, "brightnessBoost");

  return true;
}

void NeonGlowEffectSetup(NeonGlowEffect *e, const NeonGlowConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  float glowColor[3] = {cfg->glowR, cfg->glowG, cfg->glowB};
  SetShaderValue(e->shader, e->glowColorLoc, glowColor, SHADER_UNIFORM_VEC3);

  SetShaderValue(e->shader, e->edgeThresholdLoc, &cfg->edgeThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgePowerLoc, &cfg->edgePower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowRadiusLoc, &cfg->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowSamplesLoc, &cfg->glowSamples,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->originalVisibilityLoc, &cfg->originalVisibility,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->saturationBoostLoc, &cfg->saturationBoost,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessBoostLoc, &cfg->brightnessBoost,
                 SHADER_UNIFORM_FLOAT);
}

void NeonGlowEffectUninit(NeonGlowEffect *e) { UnloadShader(e->shader); }

NeonGlowConfig NeonGlowConfigDefault(void) { return NeonGlowConfig{}; }
