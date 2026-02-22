// Neon glow effect module implementation

#include "neon_glow.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool NeonGlowEffectInit(NeonGlowEffect *e) {
  e->shader = LoadShader(NULL, "shaders/neon_glow.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->glowColorLoc = GetShaderLocation(e->shader, "glowColor");
  e->edgeThresholdLoc = GetShaderLocation(e->shader, "edgeThreshold");
  e->edgePowerLoc = GetShaderLocation(e->shader, "edgePower");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->glowRadiusLoc = GetShaderLocation(e->shader, "glowRadius");
  e->glowSamplesLoc = GetShaderLocation(e->shader, "glowSamples");
  e->originalVisibilityLoc = GetShaderLocation(e->shader, "originalVisibility");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->saturationBoostLoc = GetShaderLocation(e->shader, "saturationBoost");
  e->brightnessBoostLoc = GetShaderLocation(e->shader, "brightnessBoost");

  return true;
}

void NeonGlowEffectSetup(NeonGlowEffect *e, const NeonGlowConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  float glowColor[3] = {cfg->glowR, cfg->glowG, cfg->glowB};
  SetShaderValue(e->shader, e->glowColorLoc, glowColor, SHADER_UNIFORM_VEC3);

  SetShaderValue(e->shader, e->edgeThresholdLoc, &cfg->edgeThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgePowerLoc, &cfg->edgePower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowRadiusLoc, &cfg->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowSamplesLoc, &cfg->glowSamples,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->originalVisibilityLoc, &cfg->originalVisibility,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->saturationBoostLoc, &cfg->saturationBoost,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessBoostLoc, &cfg->brightnessBoost,
                 SHADER_UNIFORM_FLOAT);
}

void NeonGlowEffectUninit(NeonGlowEffect *e) { UnloadShader(e->shader); }

NeonGlowConfig NeonGlowConfigDefault(void) { return NeonGlowConfig{}; }

void NeonGlowRegisterParams(NeonGlowConfig *cfg) {
  ModEngineRegisterParam("neonGlow.glowIntensity", &cfg->glowIntensity, 0.5f,
                         5.0f);
  ModEngineRegisterParam("neonGlow.edgeThreshold", &cfg->edgeThreshold, 0.0f,
                         0.5f);
  ModEngineRegisterParam("neonGlow.originalVisibility",
                         &cfg->originalVisibility, 0.0f, 1.0f);
  ModEngineRegisterParam("neonGlow.saturationBoost", &cfg->saturationBoost,
                         0.0f, 1.0f);
  ModEngineRegisterParam("neonGlow.brightnessBoost", &cfg->brightnessBoost,
                         0.0f, 1.0f);
}

void SetupNeonGlow(PostEffect *pe) {
  NeonGlowEffectSetup(&pe->neonGlow, &pe->effects.neonGlow);
}

// === UI ===

static void DrawNeonGlowParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  NeonGlowConfig *ng = &e->neonGlow;

  const char *colorModeLabels[] = {"Custom Color", "Source Color"};
  ImGui::Combo("Color Mode##neonglow", &ng->colorMode, colorModeLabels, 2);

  if (ng->colorMode == 0) {
    float glowCol[3] = {ng->glowR, ng->glowG, ng->glowB};
    if (ImGui::ColorEdit3("Glow Color##neonglow", glowCol)) {
      ng->glowR = glowCol[0];
      ng->glowG = glowCol[1];
      ng->glowB = glowCol[2];
    }
  } else {
    ModulatableSlider("Saturation Boost##neonglow", &ng->saturationBoost,
                      "neonGlow.saturationBoost", "%.2f", ms);
    ModulatableSlider("Brightness Boost##neonglow", &ng->brightnessBoost,
                      "neonGlow.brightnessBoost", "%.2f", ms);
  }

  ModulatableSlider("Glow Intensity##neonglow", &ng->glowIntensity,
                    "neonGlow.glowIntensity", "%.2f", ms);
  ModulatableSlider("Edge Threshold##neonglow", &ng->edgeThreshold,
                    "neonGlow.edgeThreshold", "%.3f", ms);
  ModulatableSlider("Original Visibility##neonglow", &ng->originalVisibility,
                    "neonGlow.originalVisibility", "%.2f", ms);

  if (TreeNodeAccented("Advanced##neonglow", glow)) {
    ImGui::SliderFloat("Edge Power##neonglow", &ng->edgePower, 0.5f, 3.0f,
                       "%.2f");
    ImGui::SliderFloat("Glow Radius##neonglow", &ng->glowRadius, 0.0f, 10.0f,
                       "%.1f");
    ImGui::SliderInt("Glow Samples##neonglow", &ng->glowSamples, 3, 9);
    TreeNodeAccentedPop();
  }
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_NEON_GLOW, NeonGlow, neonGlow, "Neon Glow", "GFX",
                5, EFFECT_FLAG_NONE, SetupNeonGlow, NULL, DrawNeonGlowParams)
// clang-format on
