// DoG Filter - Difference-of-Gaussians edge detection with thresholded
// sharpening

#include "dog_filter.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool DogFilterEffectInit(DogFilterEffect *e) {
  e->shader = LoadShader(NULL, "shaders/dog_filter.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->sigmaLoc = GetShaderLocation(e->shader, "sigma");
  e->tauLoc = GetShaderLocation(e->shader, "tau");
  e->phiLoc = GetShaderLocation(e->shader, "phi");

  return true;
}

void DogFilterEffectSetup(const DogFilterEffect *e,
                          const DogFilterConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->sigmaLoc, &cfg->sigma, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tauLoc, &cfg->tau, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phiLoc, &cfg->phi, SHADER_UNIFORM_FLOAT);
}

void DogFilterEffectUninit(const DogFilterEffect *e) {
  UnloadShader(e->shader);
}

void DogFilterRegisterParams(DogFilterConfig *cfg) {
  ModEngineRegisterParam("dogFilter.sigma", &cfg->sigma, 0.5f, 5.0f);
  ModEngineRegisterParam("dogFilter.tau", &cfg->tau, 0.9f, 1.0f);
  ModEngineRegisterParam("dogFilter.phi", &cfg->phi, 0.5f, 10.0f);
}

void SetupDogFilter(PostEffect *pe) {
  DogFilterEffectSetup(&pe->dogFilter, &pe->effects.dogFilter);
}

// === UI ===

static void DrawDogFilterParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  DogFilterConfig *df = &e->dogFilter;

  ModulatableSlider("Sigma##dogFilter", &df->sigma, "dogFilter.sigma", "%.2f",
                    ms);
  ModulatableSlider("Tau##dogFilter", &df->tau, "dogFilter.tau", "%.3f", ms);
  ModulatableSlider("Phi##dogFilter", &df->phi, "dogFilter.phi", "%.1f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_DOG_FILTER, DogFilter, dogFilter, "DoG Filter",
                "PRT", 5, EFFECT_FLAG_NONE, SetupDogFilter, NULL,
                DrawDogFilterParams)
// clang-format on
