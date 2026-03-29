#include "poincare_disk.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

bool PoincareDiskEffectInit(PoincareDiskEffect *e) {
  e->shader = LoadShader(NULL, "shaders/poincare_disk.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->tilePLoc = GetShaderLocation(e->shader, "tileP");
  e->tileQLoc = GetShaderLocation(e->shader, "tileQ");
  e->tileRLoc = GetShaderLocation(e->shader, "tileR");
  e->translationLoc = GetShaderLocation(e->shader, "translation");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->diskScaleLoc = GetShaderLocation(e->shader, "diskScale");

  e->time = 0.0f;
  e->rotation = 0.0f;
  e->currentTranslation[0] = 0.0f;
  e->currentTranslation[1] = 0.0f;

  return true;
}

void PoincareDiskEffectSetup(PoincareDiskEffect *e,
                             const PoincareDiskConfig *cfg, float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->time += cfg->translationSpeed * deltaTime;

  // Compute circular translation motion
  e->currentTranslation[0] =
      cfg->translationX + cfg->translationAmplitude * sinf(e->time);
  e->currentTranslation[1] =
      cfg->translationY + cfg->translationAmplitude * cosf(e->time);

  SetShaderValue(e->shader, e->tilePLoc, &cfg->tileP, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->tileQLoc, &cfg->tileQ, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->tileRLoc, &cfg->tileR, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->translationLoc, e->currentTranslation,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->diskScaleLoc, &cfg->diskScale,
                 SHADER_UNIFORM_FLOAT);
}

void PoincareDiskEffectUninit(const PoincareDiskEffect *e) {
  UnloadShader(e->shader);
}

void PoincareDiskRegisterParams(PoincareDiskConfig *cfg) {
  ModEngineRegisterParam("poincareDisk.translationX", &cfg->translationX, -0.9f,
                         0.9f);
  ModEngineRegisterParam("poincareDisk.translationY", &cfg->translationY, -0.9f,
                         0.9f);
  ModEngineRegisterParam("poincareDisk.translationSpeed",
                         &cfg->translationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("poincareDisk.translationAmplitude",
                         &cfg->translationAmplitude, 0.0f, 0.9f);
  ModEngineRegisterParam("poincareDisk.diskScale", &cfg->diskScale, 0.5f, 2.0f);
  ModEngineRegisterParam("poincareDisk.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
}

// === UI ===

static void DrawPoincareDiskParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  PoincareDiskConfig *pd = &e->poincareDisk;

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Tile P##poincare", &pd->tileP, 2, 12);
  ImGui::SliderInt("Tile Q##poincare", &pd->tileQ, 2, 12);
  ImGui::SliderInt("Tile R##poincare", &pd->tileR, 2, 12);
  ModulatableSlider("Translation X##poincare", &pd->translationX,
                    "poincareDisk.translationX", "%.2f", ms);
  ModulatableSlider("Translation Y##poincare", &pd->translationY,
                    "poincareDisk.translationY", "%.2f", ms);
  ModulatableSlider("Disk Scale##poincare", &pd->diskScale,
                    "poincareDisk.diskScale", "%.2f", ms);
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Motion Radius##poincare", &pd->translationAmplitude,
                    "poincareDisk.translationAmplitude", "%.2f", ms);
  ModulatableSliderSpeedDeg("Motion Speed##poincare", &pd->translationSpeed,
                            "poincareDisk.translationSpeed", ms);
  ModulatableSliderSpeedDeg("Rotation Speed##poincare", &pd->rotationSpeed,
                            "poincareDisk.rotationSpeed", ms);
}

void SetupPoincareDisk(PostEffect *pe) {
  PoincareDiskEffectSetup(&pe->poincareDisk, &pe->effects.poincareDisk,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(
    TRANSFORM_POINCARE_DISK, PoincareDisk, poincareDisk,
    "Poincare Disk", "SYM", 0, EFFECT_FLAG_NONE,
    SetupPoincareDisk, NULL, DrawPoincareDiskParams)
// clang-format on
