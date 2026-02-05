#include "moire_interference.h"

#include "automation/modulation_engine.h"
#include <stddef.h>

bool MoireInterferenceEffectInit(MoireInterferenceEffect *e) {
  e->shader = LoadShader(NULL, "shaders/moire_interference.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->rotationAngleLoc = GetShaderLocation(e->shader, "rotationAngle");
  e->scaleDiffLoc = GetShaderLocation(e->shader, "scaleDiff");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->blendModeLoc = GetShaderLocation(e->shader, "blendMode");
  e->centerXLoc = GetShaderLocation(e->shader, "centerX");
  e->centerYLoc = GetShaderLocation(e->shader, "centerY");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");

  e->rotationAccum = 0.0f;

  return true;
}

void MoireInterferenceEffectSetup(MoireInterferenceEffect *e,
                                  const MoireInterferenceConfig *cfg,
                                  float deltaTime) {
  e->rotationAccum += cfg->animationSpeed * deltaTime;

  SetShaderValue(e->shader, e->rotationAngleLoc, &cfg->rotationAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleDiffLoc, &cfg->scaleDiff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->blendModeLoc, &cfg->blendMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->centerXLoc, &cfg->centerX, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerYLoc, &cfg->centerY, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
}

void MoireInterferenceEffectUninit(MoireInterferenceEffect *e) {
  UnloadShader(e->shader);
}

MoireInterferenceConfig MoireInterferenceConfigDefault(void) {
  return MoireInterferenceConfig{};
}

void MoireInterferenceRegisterParams(MoireInterferenceConfig *cfg) {
  ModEngineRegisterParam("moireInterference.rotationAngle", &cfg->rotationAngle,
                         -3.14159265f, 3.14159265f);
  ModEngineRegisterParam("moireInterference.scaleDiff", &cfg->scaleDiff, 0.5f,
                         2.0f);
  ModEngineRegisterParam("moireInterference.animationSpeed",
                         &cfg->animationSpeed, -3.14159265f, 3.14159265f);
}
