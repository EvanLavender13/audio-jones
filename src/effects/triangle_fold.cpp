#include "triangle_fold.h"

#include "automation/modulation_engine.h"
#include <stddef.h>

bool TriangleFoldEffectInit(TriangleFoldEffect *e) {
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
  UnloadShader(e->shader);
}

TriangleFoldConfig TriangleFoldConfigDefault(void) {
  return TriangleFoldConfig{};
}

void TriangleFoldRegisterParams(TriangleFoldConfig *cfg) {
  ModEngineRegisterParam("triangleFold.rotationSpeed", &cfg->rotationSpeed,
                         -3.14159265f, 3.14159265f);
  ModEngineRegisterParam("triangleFold.twistSpeed", &cfg->twistSpeed,
                         -3.14159265f, 3.14159265f);
}
