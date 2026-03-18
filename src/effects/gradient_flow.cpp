#include "gradient_flow.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool GradientFlowEffectInit(GradientFlowEffect *e) {
  e->shader = LoadShader(NULL, "shaders/gradient_flow.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->edgeWeightLoc = GetShaderLocation(e->shader, "edgeWeight");
  e->randomDirectionLoc = GetShaderLocation(e->shader, "randomDirection");

  return true;
}

void GradientFlowEffectSetup(const GradientFlowEffect *e,
                             const GradientFlowConfig *cfg, int screenWidth,
                             int screenHeight) {
  const float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeWeightLoc, &cfg->edgeWeight,
                 SHADER_UNIFORM_FLOAT);

  int randomDirection = cfg->randomDirection ? 1 : 0;
  SetShaderValue(e->shader, e->randomDirectionLoc, &randomDirection,
                 SHADER_UNIFORM_INT);
}

void GradientFlowEffectUninit(const GradientFlowEffect *e) {
  UnloadShader(e->shader);
}

void GradientFlowRegisterParams(GradientFlowConfig *cfg) {
  ModEngineRegisterParam("gradientFlow.strength", &cfg->strength, 0.0f, 0.1f);
  ModEngineRegisterParam("gradientFlow.edgeWeight", &cfg->edgeWeight, 0.0f,
                         1.0f);
}

// === UI ===

static void DrawGradientFlowParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  ModulatableSlider("Strength##gradflow", &e->gradientFlow.strength,
                    "gradientFlow.strength", "%.3f", ms);
  ImGui::SliderInt("Iterations##gradflow", &e->gradientFlow.iterations, 1, 8);
  ModulatableSlider("Edge Weight##gradflow", &e->gradientFlow.edgeWeight,
                    "gradientFlow.edgeWeight", "%.2f", ms);
  ImGui::Checkbox("Random Direction##gradflow",
                  &e->gradientFlow.randomDirection);
}

void SetupGradientFlow(PostEffect *pe) {
  GradientFlowEffectSetup(&pe->gradientFlow, &pe->effects.gradientFlow,
                          pe->screenWidth, pe->screenHeight);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_GRADIENT_FLOW, GradientFlow, gradientFlow,
                "Gradient Flow", "WARP", 1, EFFECT_FLAG_NONE,
                SetupGradientFlow, NULL, DrawGradientFlowParams)
// clang-format on
