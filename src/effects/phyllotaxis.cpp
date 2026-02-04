// Phyllotaxis cellular transform module implementation

#include "phyllotaxis.h"
#include <stdlib.h>

static const float GOLDEN_ANGLE = 2.39996322972865f;

bool PhyllotaxisEffectInit(PhyllotaxisEffect *e) {
  if (e == NULL) {
    return false;
  }

  e->shader = LoadShader(NULL, "shaders/phyllotaxis.fs");
  if (!IsShaderValid(e->shader)) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->smoothModeLoc = GetShaderLocation(e->shader, "smoothMode");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->divergenceAngleLoc = GetShaderLocation(e->shader, "divergenceAngle");
  e->phaseTimeLoc = GetShaderLocation(e->shader, "phaseTime");
  e->cellRadiusLoc = GetShaderLocation(e->shader, "cellRadius");
  e->isoFrequencyLoc = GetShaderLocation(e->shader, "isoFrequency");
  e->uvDistortIntensityLoc = GetShaderLocation(e->shader, "uvDistortIntensity");
  e->organicFlowIntensityLoc =
      GetShaderLocation(e->shader, "organicFlowIntensity");
  e->edgeIsoIntensityLoc = GetShaderLocation(e->shader, "edgeIsoIntensity");
  e->centerIsoIntensityLoc = GetShaderLocation(e->shader, "centerIsoIntensity");
  e->flatFillIntensityLoc = GetShaderLocation(e->shader, "flatFillIntensity");
  e->edgeGlowIntensityLoc = GetShaderLocation(e->shader, "edgeGlowIntensity");
  e->ratioIntensityLoc = GetShaderLocation(e->shader, "ratioIntensity");
  e->determinantIntensityLoc =
      GetShaderLocation(e->shader, "determinantIntensity");
  e->edgeDetectIntensityLoc =
      GetShaderLocation(e->shader, "edgeDetectIntensity");
  e->spinOffsetLoc = GetShaderLocation(e->shader, "spinOffset");

  e->angleTime = 0.0f;
  e->phaseTime = 0.0f;
  e->spinOffset = 0.0f;

  return true;
}

void PhyllotaxisEffectSetup(PhyllotaxisEffect *e, const PhyllotaxisConfig *cfg,
                            float deltaTime) {
  if (e == NULL || cfg == NULL) {
    return;
  }

  e->angleTime += cfg->angleSpeed * deltaTime;
  e->phaseTime += cfg->phaseSpeed * deltaTime;
  e->spinOffset += cfg->spinSpeed * deltaTime;

  float divergenceAngle = GOLDEN_ANGLE + cfg->divergenceAngle + e->angleTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  int smoothModeInt = cfg->smoothMode ? 1 : 0;
  SetShaderValue(e->shader, e->smoothModeLoc, &smoothModeInt,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->divergenceAngleLoc, &divergenceAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseTimeLoc, &e->phaseTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellRadiusLoc, &cfg->cellRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->isoFrequencyLoc, &cfg->isoFrequency,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->uvDistortIntensityLoc, &cfg->uvDistortIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->organicFlowIntensityLoc,
                 &cfg->organicFlowIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeIsoIntensityLoc, &cfg->edgeIsoIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerIsoIntensityLoc, &cfg->centerIsoIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flatFillIntensityLoc, &cfg->flatFillIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeGlowIntensityLoc, &cfg->edgeGlowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ratioIntensityLoc, &cfg->ratioIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->determinantIntensityLoc,
                 &cfg->determinantIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeDetectIntensityLoc,
                 &cfg->edgeDetectIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spinOffsetLoc, &e->spinOffset,
                 SHADER_UNIFORM_FLOAT);
}

void PhyllotaxisEffectUninit(PhyllotaxisEffect *e) {
  if (e == NULL) {
    return;
  }

  UnloadShader(e->shader);
}

PhyllotaxisConfig PhyllotaxisConfigDefault(void) {
  PhyllotaxisConfig cfg = {};
  cfg.enabled = false;
  cfg.smoothMode = false;
  cfg.scale = 0.06f;
  cfg.divergenceAngle = 0.0f;
  cfg.angleSpeed = 0.0f;
  cfg.phaseSpeed = 0.0f;
  cfg.spinSpeed = 0.0f;
  cfg.cellRadius = 0.8f;
  cfg.isoFrequency = 5.0f;
  cfg.uvDistortIntensity = 0.0f;
  cfg.organicFlowIntensity = 0.0f;
  cfg.edgeIsoIntensity = 0.0f;
  cfg.centerIsoIntensity = 0.0f;
  cfg.flatFillIntensity = 0.0f;
  cfg.edgeGlowIntensity = 0.0f;
  cfg.ratioIntensity = 0.0f;
  cfg.determinantIntensity = 0.0f;
  cfg.edgeDetectIntensity = 0.0f;
  return cfg;
}
