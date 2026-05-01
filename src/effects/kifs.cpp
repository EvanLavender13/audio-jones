#include "kifs.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
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
  const float offset[2] = {cfg->offsetX, cfg->offsetY};

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

void KifsEffectUninit(const KifsEffect *e) { UnloadShader(e->shader); }

void KifsRegisterParams(KifsConfig *cfg) {
  ModEngineRegisterParam("kifs.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("kifs.twistSpeed", &cfg->twistSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("kifs.polarFoldSmoothing", &cfg->polarFoldSmoothing,
                         0.0f, 0.5f);
}

// === UI ===

static void DrawKifsParams(EffectConfig *e, const ModSources *ms, ImU32 glow) {
  (void)glow;
  KifsConfig *k = &e->kifs;

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##kifs", &k->iterations, 1, 6);
  ImGui::SliderFloat("Scale##kifs", &k->scale, 1.5f, 2.5f, "%.2f");
  ImGui::SliderFloat("Offset X##kifs", &k->offsetX, 0.0f, 2.0f, "%.2f");
  ImGui::SliderFloat("Offset Y##kifs", &k->offsetY, 0.0f, 2.0f, "%.2f");
  ImGui::SeparatorText("Fold");
  ImGui::Checkbox("Octant Fold##kifs", &k->octantFold);
  ImGui::Checkbox("Polar Fold##kifs", &k->polarFold);
  if (k->polarFold) {
    ImGui::SliderInt("Segments##kifsPolar", &k->polarFoldSegments, 2, 12);
    ModulatableSlider("Smoothing##kifsPolar", &k->polarFoldSmoothing,
                      "kifs.polarFoldSmoothing", "%.2f", ms);
  }
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Spin##kifs", &k->rotationSpeed,
                            "kifs.rotationSpeed", ms);
  ModulatableSliderSpeedDeg("Twist##kifs", &k->twistSpeed, "kifs.twistSpeed",
                            ms);
}

KifsEffect *GetKifsEffect(PostEffect *pe) {
  return (KifsEffect *)pe->effectStates[TRANSFORM_KIFS];
}

void SetupKifs(PostEffect *pe) {
  KifsEffectSetup(GetKifsEffect(pe), &pe->effects.kifs, pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(
    TRANSFORM_KIFS, Kifs, kifs,
    "KIFS", "SYM", 0, EFFECT_FLAG_NONE,
    SetupKifs, NULL, DrawKifsParams)
// clang-format on
