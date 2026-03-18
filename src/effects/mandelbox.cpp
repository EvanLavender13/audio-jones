#include "mandelbox.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
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
  const float offset[2] = {cfg->offsetX, cfg->offsetY};

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

void MandelboxEffectUninit(const MandelboxEffect *e) {
  UnloadShader(e->shader);
}

void MandelboxRegisterParams(MandelboxConfig *cfg) {
  ModEngineRegisterParam("mandelbox.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("mandelbox.twistSpeed", &cfg->twistSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("mandelbox.boxIntensity", &cfg->boxIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("mandelbox.sphereIntensity", &cfg->sphereIntensity,
                         0.0f, 1.0f);
}

// === UI ===

static void DrawMandelboxParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  MandelboxConfig *m = &e->mandelbox;

  ImGui::SliderInt("Iterations##mandelbox", &m->iterations, 1, 6);
  ImGui::SliderFloat("Scale##mandelbox", &m->scale, -3.0f, 3.0f, "%.2f");
  ImGui::SliderFloat("Offset X##mandelbox", &m->offsetX, 0.0f, 2.0f, "%.2f");
  ImGui::SliderFloat("Offset Y##mandelbox", &m->offsetY, 0.0f, 2.0f, "%.2f");
  ModulatableSliderSpeedDeg("Spin##mandelbox", &m->rotationSpeed,
                            "mandelbox.rotationSpeed", ms);
  ModulatableSliderSpeedDeg("Twist##mandelbox", &m->twistSpeed,
                            "mandelbox.twistSpeed", ms);

  if (TreeNodeAccented("Box Fold##mandelbox", glow)) {
    ImGui::SliderFloat("Limit##boxfold", &m->boxLimit, 0.5f, 2.0f, "%.2f");
    ModulatableSlider("Intensity##boxfold", &m->boxIntensity,
                      "mandelbox.boxIntensity", "%.2f", ms);
    TreeNodeAccentedPop();
  }

  if (TreeNodeAccented("Sphere Fold##mandelbox", glow)) {
    ImGui::SliderFloat("Min Radius##spherefold", &m->sphereMin, 0.1f, 0.5f,
                       "%.2f");
    ImGui::SliderFloat("Max Radius##spherefold", &m->sphereMax, 0.5f, 2.0f,
                       "%.2f");
    ModulatableSlider("Intensity##spherefold", &m->sphereIntensity,
                      "mandelbox.sphereIntensity", "%.2f", ms);
    TreeNodeAccentedPop();
  }

  ImGui::Checkbox("Polar Fold##mandelbox", &m->polarFold);
  if (m->polarFold) {
    ImGui::SliderInt("Segments##mandelboxPolar", &m->polarFoldSegments, 2, 12);
  }
}

void SetupMandelbox(PostEffect *pe) {
  MandelboxEffectSetup(&pe->mandelbox, &pe->effects.mandelbox,
                       pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(
    TRANSFORM_MANDELBOX, Mandelbox, mandelbox,
    "Mandelbox", "SYM", 0, EFFECT_FLAG_NONE,
    SetupMandelbox, NULL, DrawMandelboxParams)
// clang-format on
