// Stripe Shift: flat RGB bars displaced by input brightness

#include "stripe_shift.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

static void CacheLocations(StripeShiftEffect *e) {
  e->phaseLoc = GetShaderLocation(e->shader, "phase");
  e->stripeCountLoc = GetShaderLocation(e->shader, "stripeCount");
  e->stripeWidthLoc = GetShaderLocation(e->shader, "stripeWidth");
  e->displacementLoc = GetShaderLocation(e->shader, "displacement");
  e->channelSpreadLoc = GetShaderLocation(e->shader, "channelSpread");
  e->colorDisplaceLoc = GetShaderLocation(e->shader, "colorDisplace");
  e->hardEdgeLoc = GetShaderLocation(e->shader, "hardEdge");
}

bool StripeShiftEffectInit(StripeShiftEffect *e) {
  e->shader = LoadShader(NULL, "shaders/stripe_shift.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);
  e->phase = 0.0f;

  return true;
}

void StripeShiftEffectSetup(StripeShiftEffect *e, const StripeShiftConfig *cfg,
                            float deltaTime) {
  e->phase += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->phaseLoc, &e->phase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeCountLoc, &cfg->stripeCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeWidthLoc, &cfg->stripeWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->displacementLoc, &cfg->displacement,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->channelSpreadLoc, &cfg->channelSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorDisplaceLoc, &cfg->colorDisplace,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->hardEdgeLoc, &cfg->hardEdge,
                 SHADER_UNIFORM_FLOAT);
}

void StripeShiftEffectUninit(const StripeShiftEffect *e) {
  UnloadShader(e->shader);
}

void StripeShiftRegisterParams(StripeShiftConfig *cfg) {
  ModEngineRegisterParam("stripeShift.stripeCount", &cfg->stripeCount, 4.0f,
                         64.0f);
  ModEngineRegisterParam("stripeShift.stripeWidth", &cfg->stripeWidth, 0.05f,
                         0.5f);
  ModEngineRegisterParam("stripeShift.displacement", &cfg->displacement, 0.0f,
                         4.0f);
  ModEngineRegisterParam("stripeShift.speed", &cfg->speed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("stripeShift.channelSpread", &cfg->channelSpread, 0.0f,
                         0.5f);
  ModEngineRegisterParam("stripeShift.colorDisplace", &cfg->colorDisplace, 0.0f,
                         1.0f);
  ModEngineRegisterParam("stripeShift.hardEdge", &cfg->hardEdge, 0.0f, 1.0f);
}

// === UI ===

static void DrawStripeShiftParams(EffectConfig *e, const ModSources *ms,
                                  ImU32 glow) {
  (void)glow;
  StripeShiftConfig *cfg = &e->stripeShift;

  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Density##stripeShift", &cfg->stripeCount,
                    "stripeShift.stripeCount", "%.1f", ms);
  ModulatableSlider("Width##stripeShift", &cfg->stripeWidth,
                    "stripeShift.stripeWidth", "%.2f", ms);
  ModulatableSlider("Displace##stripeShift", &cfg->displacement,
                    "stripeShift.displacement", "%.2f", ms);
  ModulatableSlider("Hard Edge##stripeShift", &cfg->hardEdge,
                    "stripeShift.hardEdge", "%.2f", ms);

  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Speed##stripeShift", &cfg->speed,
                            "stripeShift.speed", ms);

  ImGui::SeparatorText("Color");
  ModulatableSlider("Spread##stripeShift", &cfg->channelSpread,
                    "stripeShift.channelSpread", "%.2f", ms);
  ModulatableSlider("Color Displace##stripeShift", &cfg->colorDisplace,
                    "stripeShift.colorDisplace", "%.2f", ms);
}

StripeShiftEffect *GetStripeShiftEffect(PostEffect *pe) {
  return (StripeShiftEffect *)pe->effectStates[TRANSFORM_STRIPE_SHIFT];
}

void SetupStripeShift(PostEffect *pe) {
  StripeShiftEffectSetup(GetStripeShiftEffect(pe), &pe->effects.stripeShift,
                         pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_STRIPE_SHIFT, StripeShift, stripeShift,
                "Stripe Shift", "RET", 6, EFFECT_FLAG_NONE,
                SetupStripeShift, NULL, DrawStripeShiftParams)
// clang-format on
