// Bilateral filter effect module implementation

#include "bilateral.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool BilateralEffectInit(BilateralEffect *e) {
  e->shader = LoadShader(NULL, "shaders/bilateral.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->spatialSigmaLoc = GetShaderLocation(e->shader, "spatialSigma");
  e->rangeSigmaLoc = GetShaderLocation(e->shader, "rangeSigma");

  return true;
}

void BilateralEffectSetup(const BilateralEffect *e,
                          const BilateralConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->spatialSigmaLoc, &cfg->spatialSigma,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rangeSigmaLoc, &cfg->rangeSigma,
                 SHADER_UNIFORM_FLOAT);
}

void BilateralEffectUninit(const BilateralEffect *e) {
  UnloadShader(e->shader);
}

void BilateralRegisterParams(BilateralConfig *cfg) {
  ModEngineRegisterParam("bilateral.spatialSigma", &cfg->spatialSigma, 1.0f,
                         10.0f);
  ModEngineRegisterParam("bilateral.rangeSigma", &cfg->rangeSigma, 0.01f, 0.5f);
}

BilateralEffect *GetBilateralEffect(PostEffect *pe) {
  return (BilateralEffect *)pe->effectStates[TRANSFORM_BILATERAL];
}

void SetupBilateral(PostEffect *pe) {
  BilateralEffectSetup(GetBilateralEffect(pe), &pe->effects.bilateral);
}

// === UI ===

static void DrawBilateralParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  ModulatableSlider("Spatial Sigma##bilateral", &e->bilateral.spatialSigma,
                    "bilateral.spatialSigma", "%.1f", ms);
  ModulatableSliderLog("Range Sigma##bilateral", &e->bilateral.rangeSigma,
                       "bilateral.rangeSigma", "%.3f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_BILATERAL, Bilateral, bilateral, "Bilateral", "ART", 4,
                EFFECT_FLAG_NONE, SetupBilateral, NULL, DrawBilateralParams)
// clang-format on
