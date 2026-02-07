// Density Wave Spiral effect module implementation

#include "density_wave_spiral.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include <stddef.h>

bool DensityWaveSpiralEffectInit(DensityWaveSpiralEffect *e) {
  e->shader = LoadShader(NULL, "shaders/density_wave_spiral.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->aspectLoc = GetShaderLocation(e->shader, "aspect");
  e->tightnessLoc = GetShaderLocation(e->shader, "tightness");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->globalRotationAccumLoc =
      GetShaderLocation(e->shader, "globalRotationAccum");
  e->thicknessLoc = GetShaderLocation(e->shader, "thickness");
  e->ringCountLoc = GetShaderLocation(e->shader, "ringCount");
  e->falloffLoc = GetShaderLocation(e->shader, "falloff");

  e->rotation = 0.0f;
  e->globalRotation = 0.0f;

  return true;
}

void DensityWaveSpiralEffectSetup(DensityWaveSpiralEffect *e,
                                  const DensityWaveSpiralConfig *cfg,
                                  float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->globalRotation += cfg->globalRotationSpeed * deltaTime;

  float center[2] = {cfg->centerX, cfg->centerY};
  float aspect[2] = {cfg->aspectX, cfg->aspectY};

  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->aspectLoc, aspect, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->tightnessLoc, &cfg->tightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->globalRotationAccumLoc, &e->globalRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thicknessLoc, &cfg->thickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ringCountLoc, &cfg->ringCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffLoc, &cfg->falloff, SHADER_UNIFORM_FLOAT);
}

void DensityWaveSpiralEffectUninit(DensityWaveSpiralEffect *e) {
  UnloadShader(e->shader);
}

DensityWaveSpiralConfig DensityWaveSpiralConfigDefault(void) {
  return DensityWaveSpiralConfig{};
}

void DensityWaveSpiralRegisterParams(DensityWaveSpiralConfig *cfg) {
  ModEngineRegisterParam("densityWaveSpiral.tightness", &cfg->tightness,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("densityWaveSpiral.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("densityWaveSpiral.globalRotationSpeed",
                         &cfg->globalRotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("densityWaveSpiral.thickness", &cfg->thickness, 0.05f,
                         0.5f);
}
