// Vignette effect module implementation

#include "vignette.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"

#include "imgui.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"

#include <stddef.h>

bool VignetteEffectInit(VignetteEffect *e) {
  e->shader = LoadShader(NULL, "shaders/vignette.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->radiusLoc = GetShaderLocation(e->shader, "radius");
  e->softnessLoc = GetShaderLocation(e->shader, "softness");
  e->roundnessLoc = GetShaderLocation(e->shader, "roundness");
  e->colorLoc = GetShaderLocation(e->shader, "vignetteColor");

  return true;
}

void VignetteEffectSetup(const VignetteEffect *e, const VignetteConfig *cfg) {
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radiusLoc, &cfg->radius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->softnessLoc, &cfg->softness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->roundnessLoc, &cfg->roundness,
                 SHADER_UNIFORM_FLOAT);
  const float color[3] = {cfg->colorR, cfg->colorG, cfg->colorB};
  SetShaderValue(e->shader, e->colorLoc, color, SHADER_UNIFORM_VEC3);
}

void VignetteEffectUninit(const VignetteEffect *e) { UnloadShader(e->shader); }

void VignetteRegisterParams(VignetteConfig *cfg) {
  ModEngineRegisterParam("vignette.intensity", &cfg->intensity, 0.0f, 1.0f);
  ModEngineRegisterParam("vignette.radius", &cfg->radius, 0.0f, 1.0f);
  ModEngineRegisterParam("vignette.softness", &cfg->softness, 0.01f, 1.0f);
  ModEngineRegisterParam("vignette.roundness", &cfg->roundness, 0.0f, 1.0f);
}

void SetupVignette(PostEffect *pe) {
  VignetteEffectSetup(&pe->vignette, &pe->effects.vignette);
}

// === UI ===

static void DrawVignetteParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  VignetteConfig *c = &e->vignette;

  ModulatableSlider("Intensity##vignette", &c->intensity, "vignette.intensity",
                    "%.2f", ms);
  ModulatableSlider("Radius##vignette", &c->radius, "vignette.radius", "%.2f",
                    ms);
  ModulatableSliderLog("Softness##vignette", &c->softness, "vignette.softness",
                       "%.2f", ms);
  ModulatableSlider("Roundness##vignette", &c->roundness, "vignette.roundness",
                    "%.2f", ms);
  ImGui::ColorEdit3("Color##vignette", &c->colorR);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_VIGNETTE, Vignette, vignette,
                "Vignette", "OPT", 7, EFFECT_FLAG_NONE,
                SetupVignette, NULL, DrawVignetteParams)
// clang-format on
