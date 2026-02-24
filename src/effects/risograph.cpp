// Risograph effect module implementation

#include "risograph.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool RisographEffectInit(RisographEffect *e) {
  e->shader = LoadShader(NULL, "shaders/risograph.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->grainScaleLoc = GetShaderLocation(e->shader, "grainScale");
  e->grainIntensityLoc = GetShaderLocation(e->shader, "grainIntensity");
  e->grainTimeLoc = GetShaderLocation(e->shader, "grainTime");
  e->misregAmountLoc = GetShaderLocation(e->shader, "misregAmount");
  e->misregTimeLoc = GetShaderLocation(e->shader, "misregTime");
  e->inkDensityLoc = GetShaderLocation(e->shader, "inkDensity");
  e->posterizeLoc = GetShaderLocation(e->shader, "posterize");
  e->paperToneLoc = GetShaderLocation(e->shader, "paperTone");

  e->grainTime = 0.0f;
  e->misregTime = 0.0f;

  return true;
}

void RisographEffectSetup(RisographEffect *e, const RisographConfig *cfg,
                          float deltaTime) {
  e->grainTime += cfg->grainSpeed * deltaTime;
  e->misregTime += cfg->misregSpeed * deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->grainScaleLoc, &cfg->grainScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainIntensityLoc, &cfg->grainIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainTimeLoc, &e->grainTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->misregAmountLoc, &cfg->misregAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->misregTimeLoc, &e->misregTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->inkDensityLoc, &cfg->inkDensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->posterizeLoc, &cfg->posterize,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->paperToneLoc, &cfg->paperTone,
                 SHADER_UNIFORM_FLOAT);
}

void RisographEffectUninit(RisographEffect *e) { UnloadShader(e->shader); }

RisographConfig RisographConfigDefault(void) { return RisographConfig{}; }

void RisographRegisterParams(RisographConfig *cfg) {
  ModEngineRegisterParam("risograph.grainScale", &cfg->grainScale, 50.0f,
                         800.0f);
  ModEngineRegisterParam("risograph.grainIntensity", &cfg->grainIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("risograph.grainSpeed", &cfg->grainSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("risograph.misregAmount", &cfg->misregAmount, 0.0f,
                         0.02f);
  ModEngineRegisterParam("risograph.misregSpeed", &cfg->misregSpeed, 0.0f,
                         2.0f);
  ModEngineRegisterParam("risograph.inkDensity", &cfg->inkDensity, 0.2f, 1.5f);
  ModEngineRegisterParam("risograph.paperTone", &cfg->paperTone, 0.0f, 1.0f);
}

void SetupRisograph(PostEffect *pe) {
  RisographEffectSetup(&pe->risograph, &pe->effects.risograph,
                       pe->currentDeltaTime);
}

// === UI ===

static void DrawRisographParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  RisographConfig *cfg = &e->risograph;

  ModulatableSlider("Grain Scale##risograph", &cfg->grainScale,
                    "risograph.grainScale", "%.0f", ms);
  ModulatableSlider("Grain##risograph", &cfg->grainIntensity,
                    "risograph.grainIntensity", "%.2f", ms);
  ModulatableSlider("Grain Speed##risograph", &cfg->grainSpeed,
                    "risograph.grainSpeed", "%.2f", ms);
  ModulatableSlider("Misregister##risograph", &cfg->misregAmount,
                    "risograph.misregAmount", "%.4f", ms);
  ModulatableSlider("Misreg Speed##risograph", &cfg->misregSpeed,
                    "risograph.misregSpeed", "%.2f", ms);
  ModulatableSlider("Ink Density##risograph", &cfg->inkDensity,
                    "risograph.inkDensity", "%.2f", ms);
  ImGui::SliderInt("Posterize##risograph", &cfg->posterize, 0, 16);
  ModulatableSlider("Paper Tone##risograph", &cfg->paperTone,
                    "risograph.paperTone", "%.2f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_RISOGRAPH, Risograph, risograph, "Risograph", "GFX", 5,
                EFFECT_FLAG_NONE, SetupRisograph, NULL, DrawRisographParams)
// clang-format on
