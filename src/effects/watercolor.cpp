// Watercolor effect module implementation

#include "watercolor.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool WatercolorEffectInit(WatercolorEffect *e) {
  e->shader = LoadShader(NULL, "shaders/watercolor.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->strokeStepLoc = GetShaderLocation(e->shader, "strokeStep");
  e->washStrengthLoc = GetShaderLocation(e->shader, "washStrength");
  e->paperScaleLoc = GetShaderLocation(e->shader, "paperScale");
  e->paperStrengthLoc = GetShaderLocation(e->shader, "paperStrength");
  e->edgePoolLoc = GetShaderLocation(e->shader, "edgePool");
  e->flowCenterLoc = GetShaderLocation(e->shader, "flowCenter");
  e->flowWidthLoc = GetShaderLocation(e->shader, "flowWidth");

  return true;
}

void WatercolorEffectSetup(const WatercolorEffect *e,
                           const WatercolorConfig *cfg) {
  SetShaderValue(e->shader, e->samplesLoc, &cfg->samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->strokeStepLoc, &cfg->strokeStep,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->washStrengthLoc, &cfg->washStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paperScaleLoc, &cfg->paperScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paperStrengthLoc, &cfg->paperStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgePoolLoc, &cfg->edgePool,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowCenterLoc, &cfg->flowCenter,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowWidthLoc, &cfg->flowWidth,
                 SHADER_UNIFORM_FLOAT);
}

void WatercolorEffectUninit(const WatercolorEffect *e) {
  UnloadShader(e->shader);
}

void WatercolorRegisterParams(WatercolorConfig *cfg) {
  ModEngineRegisterParam("watercolor.strokeStep", &cfg->strokeStep, 0.4f, 2.0f);
  ModEngineRegisterParam("watercolor.washStrength", &cfg->washStrength, 0.0f,
                         1.0f);
  ModEngineRegisterParam("watercolor.paperStrength", &cfg->paperStrength, 0.0f,
                         1.0f);
}

WatercolorEffect *GetWatercolorEffect(PostEffect *pe) {
  return (WatercolorEffect *)pe->effectStates[TRANSFORM_WATERCOLOR];
}

void SetupWatercolor(PostEffect *pe) {
  WatercolorEffectSetup(GetWatercolorEffect(pe), &pe->effects.watercolor);
}

// === UI ===

static void DrawWatercolorParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  WatercolorConfig *wc = &e->watercolor;
  ImGui::SeparatorText("Stroke");
  ImGui::SliderInt("Samples##wc", &wc->samples, 8, 32);
  ModulatableSlider("Stroke Step##wc", &wc->strokeStep, "watercolor.strokeStep",
                    "%.2f", ms);
  ModulatableSlider("Wash Strength##wc", &wc->washStrength,
                    "watercolor.washStrength", "%.2f", ms);
  ImGui::SeparatorText("Paper");
  ImGui::SliderFloat("Paper Scale##wc", &wc->paperScale, 1.0f, 20.0f, "%.1f");
  ModulatableSlider("Paper Strength##wc", &wc->paperStrength,
                    "watercolor.paperStrength", "%.2f", ms);
  ImGui::SeparatorText("Flow");
  ImGui::SliderFloat("Edge Pool##wc", &wc->edgePool, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat("Flow Center##wc", &wc->flowCenter, 0.5f, 1.2f, "%.2f");
  ImGui::SliderFloat("Flow Width##wc", &wc->flowWidth, 0.05f, 0.5f, "%.2f");
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_WATERCOLOR, Watercolor, watercolor, "Watercolor",
                "ART", 4, EFFECT_FLAG_HALF_RES, SetupWatercolor, NULL,
                DrawWatercolorParams)
// clang-format on
