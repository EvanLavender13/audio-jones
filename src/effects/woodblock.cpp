// Woodblock effect module implementation

#include "woodblock.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool WoodblockEffectInit(WoodblockEffect *e) {
  e->shader = LoadShader(NULL, "shaders/woodblock.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->levelsLoc = GetShaderLocation(e->shader, "levels");
  e->edgeThresholdLoc = GetShaderLocation(e->shader, "edgeThreshold");
  e->edgeSoftnessLoc = GetShaderLocation(e->shader, "edgeSoftness");
  e->edgeThicknessLoc = GetShaderLocation(e->shader, "edgeThickness");
  e->grainIntensityLoc = GetShaderLocation(e->shader, "grainIntensity");
  e->grainScaleLoc = GetShaderLocation(e->shader, "grainScale");
  e->grainAngleLoc = GetShaderLocation(e->shader, "grainAngle");
  e->registrationOffsetLoc = GetShaderLocation(e->shader, "registrationOffset");
  e->inkDensityLoc = GetShaderLocation(e->shader, "inkDensity");
  e->paperToneLoc = GetShaderLocation(e->shader, "paperTone");

  return true;
}

void WoodblockEffectSetup(const WoodblockEffect *e,
                          const WoodblockConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  int levelsInt = (int)cfg->levels;
  SetShaderValue(e->shader, e->levelsLoc, &levelsInt, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeThresholdLoc, &cfg->edgeThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeSoftnessLoc, &cfg->edgeSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeThicknessLoc, &cfg->edgeThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainIntensityLoc, &cfg->grainIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainScaleLoc, &cfg->grainScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainAngleLoc, &cfg->grainAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->registrationOffsetLoc, &cfg->registrationOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->inkDensityLoc, &cfg->inkDensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paperToneLoc, &cfg->paperTone,
                 SHADER_UNIFORM_FLOAT);
}

void WoodblockEffectUninit(const WoodblockEffect *e) {
  UnloadShader(e->shader);
}

void WoodblockRegisterParams(WoodblockConfig *cfg) {
  ModEngineRegisterParam("woodblock.levels", &cfg->levels, 2.0f, 12.0f);
  ModEngineRegisterParam("woodblock.edgeThreshold", &cfg->edgeThreshold, 0.0f,
                         1.0f);
  ModEngineRegisterParam("woodblock.edgeSoftness", &cfg->edgeSoftness, 0.0f,
                         0.5f);
  ModEngineRegisterParam("woodblock.edgeThickness", &cfg->edgeThickness, 0.5f,
                         3.0f);
  ModEngineRegisterParam("woodblock.grainIntensity", &cfg->grainIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("woodblock.grainScale", &cfg->grainScale, 1.0f, 20.0f);
  ModEngineRegisterParam("woodblock.grainAngle", &cfg->grainAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("woodblock.registrationOffset",
                         &cfg->registrationOffset, 0.0f, 0.02f);
  ModEngineRegisterParam("woodblock.inkDensity", &cfg->inkDensity, 0.3f, 1.5f);
  ModEngineRegisterParam("woodblock.paperTone", &cfg->paperTone, 0.0f, 1.0f);
}

void SetupWoodblock(PostEffect *pe) {
  WoodblockEffectSetup(&pe->woodblock, &pe->effects.woodblock);
}

// === UI ===

static void DrawWoodblockParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  WoodblockConfig *cfg = &e->woodblock;

  ModulatableSliderInt("Levels##woodblock", &cfg->levels, "woodblock.levels",
                       ms);
  ImGui::SeparatorText("Outline");
  ModulatableSlider("Edge Threshold##woodblock", &cfg->edgeThreshold,
                    "woodblock.edgeThreshold", "%.2f", ms);
  ModulatableSlider("Edge Softness##woodblock", &cfg->edgeSoftness,
                    "woodblock.edgeSoftness", "%.2f", ms);
  ModulatableSlider("Thickness##woodblock", &cfg->edgeThickness,
                    "woodblock.edgeThickness", "%.2f", ms);
  ImGui::SeparatorText("Wood Grain");
  ModulatableSlider("Grain##woodblock", &cfg->grainIntensity,
                    "woodblock.grainIntensity", "%.2f", ms);
  ModulatableSlider("Grain Scale##woodblock", &cfg->grainScale,
                    "woodblock.grainScale", "%.1f", ms);
  ModulatableSliderAngleDeg("Grain Angle##woodblock", &cfg->grainAngle,
                            "woodblock.grainAngle", ms);
  ImGui::SeparatorText("Print");
  ModulatableSlider("Misregister##woodblock", &cfg->registrationOffset,
                    "woodblock.registrationOffset", "%.4f", ms);
  ModulatableSlider("Ink Density##woodblock", &cfg->inkDensity,
                    "woodblock.inkDensity", "%.2f", ms);
  ModulatableSlider("Paper Tone##woodblock", &cfg->paperTone,
                    "woodblock.paperTone", "%.2f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_WOODBLOCK, Woodblock, woodblock, "Woodblock", "ART", 4,
                EFFECT_FLAG_NONE, SetupWoodblock, NULL, DrawWoodblockParams)
// clang-format on
