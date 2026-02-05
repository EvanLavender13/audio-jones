#include "mandelbox.h"

#include <stddef.h>

bool MandelboxEffectInit(MandelboxEffect *e) {
  e->shader = LoadShader(NULL, "shaders/mandelbox.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->boxLimitLoc = GetShaderLocation(e->shader, "boxLimit");
  e->sphereMinLoc = GetShaderLocation(e->shader, "sphereMin");
  e->sphereMaxLoc = GetShaderLocation(e->shader, "sphereMax");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->offsetLoc = GetShaderLocation(e->shader, "mandelboxOffset");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->twistAngleLoc = GetShaderLocation(e->shader, "twistAngle");
  e->boxIntensityLoc = GetShaderLocation(e->shader, "boxIntensity");
  e->sphereIntensityLoc = GetShaderLocation(e->shader, "sphereIntensity");
  e->polarFoldLoc = GetShaderLocation(e->shader, "polarFold");
  e->polarFoldSegmentsLoc = GetShaderLocation(e->shader, "polarFoldSegments");

  e->rotation = 0.0f;
  e->twist = 0.0f;

  return true;
}

void MandelboxEffectSetup(MandelboxEffect *e, const MandelboxConfig *cfg,
                          float deltaTime) {
  // Accumulate animation state
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->twist += cfg->twistSpeed * deltaTime;

  // Pack offset into vec2
  float offset[2] = {cfg->offsetX, cfg->offsetY};

  // Convert bool to int for shader
  int polarFoldInt = cfg->polarFold ? 1 : 0;

  // Set uniforms
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->boxLimitLoc, &cfg->boxLimit,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereMinLoc, &cfg->sphereMin,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereMaxLoc, &cfg->sphereMax,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetLoc, offset, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &e->twist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->boxIntensityLoc, &cfg->boxIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereIntensityLoc, &cfg->sphereIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->polarFoldLoc, &polarFoldInt, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->polarFoldSegmentsLoc, &cfg->polarFoldSegments,
                 SHADER_UNIFORM_INT);
}

void MandelboxEffectUninit(MandelboxEffect *e) { UnloadShader(e->shader); }

MandelboxConfig MandelboxConfigDefault(void) { return MandelboxConfig{}; }
