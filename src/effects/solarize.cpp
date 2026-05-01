// Solarize effect - inverts tones around a threshold for Sabattier-style
// partial negatives

#include "solarize.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool SolarizeEffectInit(SolarizeEffect *e) {
  e->shader = LoadShader(NULL, "shaders/solarize.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->amountLoc = GetShaderLocation(e->shader, "amount");
  e->thresholdLoc = GetShaderLocation(e->shader, "threshold");

  return true;
}

void SolarizeEffectSetup(const SolarizeEffect *e, const SolarizeConfig *cfg) {
  SetShaderValue(e->shader, e->amountLoc, &cfg->amount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thresholdLoc, &cfg->threshold,
                 SHADER_UNIFORM_FLOAT);
}

void SolarizeEffectUninit(const SolarizeEffect *e) { UnloadShader(e->shader); }

void SolarizeRegisterParams(SolarizeConfig *cfg) {
  ModEngineRegisterParam("solarize.amount", &cfg->amount, 0.0f, 1.0f);
  ModEngineRegisterParam("solarize.threshold", &cfg->threshold, 0.0f, 1.0f);
}

// === UI ===

static void DrawSolarizeParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  SolarizeConfig *sol = &e->solarize;

  ModulatableSlider("Amount##solarize", &sol->amount, "solarize.amount", "%.2f",
                    ms);
  ModulatableSlider("Threshold##solarize", &sol->threshold,
                    "solarize.threshold", "%.2f", ms);
}

SolarizeEffect *GetSolarizeEffect(PostEffect *pe) {
  return (SolarizeEffect *)pe->effectStates[TRANSFORM_SOLARIZE];
}

void SetupSolarize(PostEffect *pe) {
  SolarizeEffectSetup(GetSolarizeEffect(pe), &pe->effects.solarize);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_SOLARIZE, Solarize, solarize, "Solarize", "COL", 8,
                EFFECT_FLAG_NONE, SetupSolarize, NULL, DrawSolarizeParams)
// clang-format on
