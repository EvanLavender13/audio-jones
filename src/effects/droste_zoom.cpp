#include "droste_zoom.h"

#include <stddef.h>

bool DrosteZoomEffectInit(DrosteZoomEffect *e) {
  e->shader = LoadShader(NULL, "shaders/droste_zoom.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->spiralAngleLoc = GetShaderLocation(e->shader, "spiralAngle");
  e->shearCoeffLoc = GetShaderLocation(e->shader, "shearCoeff");
  e->innerRadiusLoc = GetShaderLocation(e->shader, "innerRadius");
  e->branchesLoc = GetShaderLocation(e->shader, "branches");

  e->time = 0.0f;

  return true;
}

void DrosteZoomEffectSetup(DrosteZoomEffect *e, const DrosteZoomConfig *cfg,
                           float deltaTime) {
  e->time += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralAngleLoc, &cfg->spiralAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shearCoeffLoc, &cfg->shearCoeff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->innerRadiusLoc, &cfg->innerRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->branchesLoc, &cfg->branches, SHADER_UNIFORM_INT);
}

void DrosteZoomEffectUninit(DrosteZoomEffect *e) { UnloadShader(e->shader); }

DrosteZoomConfig DrosteZoomConfigDefault(void) { return DrosteZoomConfig{}; }
