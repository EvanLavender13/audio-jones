#include "triangle_fold.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
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
  const float offset[2] = {cfg->offsetX, cfg->offsetY};

  // Set uniforms
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetLoc, offset, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &e->twist, SHADER_UNIFORM_FLOAT);
}

void TriangleFoldEffectUninit(const TriangleFoldEffect *e) {
  UnloadShader(e->shader);
}

void TriangleFoldRegisterParams(TriangleFoldConfig *cfg) {
  ModEngineRegisterParam("triangleFold.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("triangleFold.twistSpeed", &cfg->twistSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
}

// === UI ===

static void DrawTriangleFoldParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  TriangleFoldConfig *t = &e->triangleFold;

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##trianglefold", &t->iterations, 1, 6);
  ImGui::SliderFloat("Scale##trianglefold", &t->scale, 1.5f, 2.5f, "%.2f");
  ImGui::SliderFloat("Offset X##trianglefold", &t->offsetX, 0.0f, 2.0f, "%.2f");
  ImGui::SliderFloat("Offset Y##trianglefold", &t->offsetY, 0.0f, 2.0f, "%.2f");

  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Spin##trianglefold", &t->rotationSpeed,
                            "triangleFold.rotationSpeed", ms);
  ModulatableSliderSpeedDeg("Twist##trianglefold", &t->twistSpeed,
                            "triangleFold.twistSpeed", ms);
}

void SetupTriangleFold(PostEffect *pe) {
  TriangleFoldEffectSetup(&pe->triangleFold, &pe->effects.triangleFold,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(
    TRANSFORM_TRIANGLE_FOLD, TriangleFold, triangleFold,
    "Triangle Fold", "SYM", 0, EFFECT_FLAG_NONE,
    SetupTriangleFold, NULL, DrawTriangleFoldParams)
// clang-format on
