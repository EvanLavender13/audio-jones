// Phi Blur effect module implementation

#include "phi_blur.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool PhiBlurEffectInit(PhiBlurEffect *e) {
  e->shader = LoadShader(NULL, "shaders/phi_blur.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->shapeLoc = GetShaderLocation(e->shader, "shape");
  e->radiusLoc = GetShaderLocation(e->shader, "radius");
  e->shapeAngleLoc = GetShaderLocation(e->shader, "shapeAngle");
  e->starPointsLoc = GetShaderLocation(e->shader, "starPoints");
  e->starInnerRadiusLoc = GetShaderLocation(e->shader, "starInnerRadius");
  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->gammaLoc = GetShaderLocation(e->shader, "gamma");

  return true;
}

void PhiBlurEffectSetup(PhiBlurEffect *e, const PhiBlurConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->shapeLoc, &cfg->shape, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->radiusLoc, &cfg->radius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shapeAngleLoc, &cfg->shapeAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starPointsLoc, &cfg->starPoints,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->starInnerRadiusLoc, &cfg->starInnerRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->samplesLoc, &cfg->samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gammaLoc, &cfg->gamma, SHADER_UNIFORM_FLOAT);
}

void PhiBlurEffectUninit(PhiBlurEffect *e) { UnloadShader(e->shader); }

PhiBlurConfig PhiBlurConfigDefault(void) { return PhiBlurConfig{}; }

void PhiBlurRegisterParams(PhiBlurConfig *cfg) {
  ModEngineRegisterParam("phiBlur.radius", &cfg->radius, 0.0f, 50.0f);
  ModEngineRegisterParam("phiBlur.shapeAngle", &cfg->shapeAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("phiBlur.starInnerRadius", &cfg->starInnerRadius, 0.1f,
                         0.9f);
  ModEngineRegisterParam("phiBlur.gamma", &cfg->gamma, 1.0f, 6.0f);
}

void SetupPhiBlur(PostEffect *pe) {
  PhiBlurEffectSetup(&pe->phiBlur, &pe->effects.phiBlur);
}

// === UI ===

static void DrawPhiBlurParams(EffectConfig *e, const ModSources *ms,
                              ImU32 glow) {
  (void)glow;
  PhiBlurConfig *p = &e->phiBlur;

  ImGui::Combo("Shape##phiBlur", &p->shape, "Disc\0Box\0Hex\0Star\0");
  ModulatableSlider("Radius##phiBlur", &p->radius, "phiBlur.radius", "%.1f",
                    ms);
  ImGui::SliderInt("Samples##phiBlur", &p->samples, 8, 128);
  ModulatableSlider("Gamma##phiBlur", &p->gamma, "phiBlur.gamma", "%.1f", ms);
  if (p->shape != 0) {
    ModulatableSliderAngleDeg("Shape Angle##phiBlur", &p->shapeAngle,
                              "phiBlur.shapeAngle", ms);
  }
  if (p->shape == 3) {
    ImGui::SliderInt("Star Points##phiBlur", &p->starPoints, 3, 8);
    ModulatableSlider("Inner Radius##phiBlur", &p->starInnerRadius,
                      "phiBlur.starInnerRadius", "%.2f", ms);
  }
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_PHI_BLUR, PhiBlur, phiBlur, "Phi Blur", "OPT", 7,
                EFFECT_FLAG_NONE, SetupPhiBlur, NULL, DrawPhiBlurParams)
// clang-format on
