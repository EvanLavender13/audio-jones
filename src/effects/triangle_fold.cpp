#include "triangle_fold.h"

#include <stddef.h>

bool TriangleFoldEffectInit(TriangleFoldEffect *e) {
  if (e == NULL) {
    return false;
  }

  e->shader = LoadShader(NULL, "shaders/triangle_fold.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->offsetLoc = GetShaderLocation(e->shader, "triangleOffset");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->twistAngleLoc = GetShaderLocation(e->shader, "twistAngle");

  e->rotation = 0.0f;
  e->twist = 0.0f;

  return true;
}

void TriangleFoldEffectSetup(TriangleFoldEffect *e,
                             const TriangleFoldConfig *cfg, float deltaTime) {
  if (e == NULL || cfg == NULL) {
    return;
  }

  // Accumulate animation state
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->twist += cfg->twistSpeed * deltaTime;

  // Pack offset into vec2
  float offset[2] = {cfg->offsetX, cfg->offsetY};

  // Set uniforms
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetLoc, offset, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &e->twist, SHADER_UNIFORM_FLOAT);
}

void TriangleFoldEffectUninit(TriangleFoldEffect *e) {
  if (e == NULL) {
    return;
  }

  UnloadShader(e->shader);
}

TriangleFoldConfig TriangleFoldConfigDefault(void) {
  TriangleFoldConfig cfg;
  cfg.enabled = false;
  cfg.iterations = 3;
  cfg.scale = 2.0f;
  cfg.offsetX = 0.5f;
  cfg.offsetY = 0.5f;
  cfg.rotationSpeed = 0.0f;
  cfg.twistSpeed = 0.0f;
  return cfg;
}
