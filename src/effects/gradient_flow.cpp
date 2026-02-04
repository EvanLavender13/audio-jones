#include "gradient_flow.h"

#include <stddef.h>

bool GradientFlowEffectInit(GradientFlowEffect *e) {
  e->shader = LoadShader(NULL, "shaders/gradient_flow.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->edgeWeightLoc = GetShaderLocation(e->shader, "edgeWeight");
  e->randomDirectionLoc = GetShaderLocation(e->shader, "randomDirection");

  return true;
}

void GradientFlowEffectSetup(GradientFlowEffect *e,
                             const GradientFlowConfig *cfg, int screenWidth,
                             int screenHeight) {
  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeWeightLoc, &cfg->edgeWeight,
                 SHADER_UNIFORM_FLOAT);

  int randomDirection = cfg->randomDirection ? 1 : 0;
  SetShaderValue(e->shader, e->randomDirectionLoc, &randomDirection,
                 SHADER_UNIFORM_INT);
}

void GradientFlowEffectUninit(GradientFlowEffect *e) {
  UnloadShader(e->shader);
}

GradientFlowConfig GradientFlowConfigDefault(void) {
  GradientFlowConfig cfg;
  cfg.enabled = false;
  cfg.strength = 0.01f;
  cfg.iterations = 8;
  cfg.edgeWeight = 1.0f;
  cfg.randomDirection = false;
  return cfg;
}
