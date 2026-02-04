#include "kifs.h"

#include <stddef.h>

bool KifsEffectInit(KifsEffect *e) {
  e->shader = LoadShader(NULL, "shaders/kifs.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->offsetLoc = GetShaderLocation(e->shader, "kifsOffset");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->twistAngleLoc = GetShaderLocation(e->shader, "twistAngle");
  e->octantFoldLoc = GetShaderLocation(e->shader, "octantFold");
  e->polarFoldLoc = GetShaderLocation(e->shader, "polarFold");
  e->polarFoldSegmentsLoc = GetShaderLocation(e->shader, "polarFoldSegments");
  e->polarFoldSmoothingLoc = GetShaderLocation(e->shader, "polarFoldSmoothing");

  e->rotation = 0.0f;
  e->twist = 0.0f;

  return true;
}

void KifsEffectSetup(KifsEffect *e, const KifsConfig *cfg, float deltaTime) {
  // Accumulate animation state
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->twist += cfg->twistSpeed * deltaTime;

  // Pack offset into vec2
  float offset[2] = {cfg->offsetX, cfg->offsetY};

  // Convert bools to ints for shader
  int octantFoldInt = cfg->octantFold ? 1 : 0;
  int polarFoldInt = cfg->polarFold ? 1 : 0;

  // Set uniforms
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetLoc, offset, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &e->twist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octantFoldLoc, &octantFoldInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->polarFoldLoc, &polarFoldInt, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->polarFoldSegmentsLoc, &cfg->polarFoldSegments,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->polarFoldSmoothingLoc, &cfg->polarFoldSmoothing,
                 SHADER_UNIFORM_FLOAT);
}

void KifsEffectUninit(KifsEffect *e) { UnloadShader(e->shader); }

KifsConfig KifsConfigDefault(void) {
  KifsConfig cfg;
  cfg.enabled = false;
  cfg.iterations = 4;
  cfg.scale = 2.0f;
  cfg.offsetX = 1.0f;
  cfg.offsetY = 1.0f;
  cfg.rotationSpeed = 0.0f;
  cfg.twistSpeed = 0.0f;
  cfg.octantFold = false;
  cfg.polarFold = false;
  cfg.polarFoldSegments = 6;
  cfg.polarFoldSmoothing = 0.0f;
  return cfg;
}
