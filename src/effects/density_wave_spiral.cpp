// Density Wave Spiral effect module implementation

#include "density_wave_spiral.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
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

  const float center[2] = {cfg->centerX, cfg->centerY};
  const float aspect[2] = {cfg->aspectX, cfg->aspectY};

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

void DensityWaveSpiralEffectUninit(const DensityWaveSpiralEffect *e) {
  UnloadShader(e->shader);
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

// === UI ===

static void DrawDensityWaveSpiralParams(EffectConfig *e, const ModSources *ms,
                                        ImU32 glow) {
  (void)glow;
  DensityWaveSpiralConfig *dws = &e->densityWaveSpiral;
  ImGui::SeparatorText("Center");
  ImGui::SliderFloat("X##dwscenter", &dws->centerX, -0.5f, 0.5f, "%.2f");
  ImGui::SliderFloat("Y##dwscenter", &dws->centerY, -0.5f, 0.5f, "%.2f");
  ImGui::SeparatorText("Aspect");
  ImGui::SliderFloat("X##dwsaspect", &dws->aspectX, 0.1f, 1.0f, "%.2f");
  ImGui::SliderFloat("Y##dwsaspect", &dws->aspectY, 0.1f, 1.0f, "%.2f");
  ModulatableSliderAngleDeg("Tightness##dws", &dws->tightness,
                            "densityWaveSpiral.tightness", ms);
  ModulatableSliderSpeedDeg("Rotation Speed##dws", &dws->rotationSpeed,
                            "densityWaveSpiral.rotationSpeed", ms);
  ModulatableSliderSpeedDeg("Global Rotation##dws", &dws->globalRotationSpeed,
                            "densityWaveSpiral.globalRotationSpeed", ms);
  ModulatableSlider("Thickness##dws", &dws->thickness,
                    "densityWaveSpiral.thickness", "%.2f", ms);
  ImGui::SliderInt("Ring Count##dws", &dws->ringCount, 10, 50);
  ImGui::SliderFloat("Falloff##dws", &dws->falloff, 0.5f, 2.0f, "%.2f");
}

DensityWaveSpiralEffect *GetDensityWaveSpiralEffect(PostEffect *pe) {
  return (DensityWaveSpiralEffect *)
      pe->effectStates[TRANSFORM_DENSITY_WAVE_SPIRAL];
}

void SetupDensityWaveSpiral(PostEffect *pe) {
  DensityWaveSpiralEffectSetup(GetDensityWaveSpiralEffect(pe),
                               &pe->effects.densityWaveSpiral,
                               pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_DENSITY_WAVE_SPIRAL, DensityWaveSpiral,
                densityWaveSpiral, "Density Wave Spiral", "MOT", 3,
                EFFECT_FLAG_NONE, SetupDensityWaveSpiral, NULL,
                DrawDensityWaveSpiralParams)
// clang-format on
