// Neon glow effect module implementation

#include "neon_glow.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool NeonGlowEffectInit(NeonGlowEffect *e) {
  e->shader = LoadShader(NULL, "shaders/neon_glow.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->glowRadiusLoc = GetShaderLocation(e->shader, "glowRadius");
  e->coreSharpnessLoc = GetShaderLocation(e->shader, "coreSharpness");
  e->edgeThresholdLoc = GetShaderLocation(e->shader, "edgeThreshold");
  e->edgePowerLoc = GetShaderLocation(e->shader, "edgePower");
  e->saturationBoostLoc = GetShaderLocation(e->shader, "saturationBoost");
  e->brightnessBoostLoc = GetShaderLocation(e->shader, "brightnessBoost");
  e->originalVisibilityLoc = GetShaderLocation(e->shader, "originalVisibility");

  return true;
}

void NeonGlowEffectSetup(NeonGlowEffect *e, const NeonGlowConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowRadiusLoc, &cfg->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->coreSharpnessLoc, &cfg->coreSharpness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeThresholdLoc, &cfg->edgeThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgePowerLoc, &cfg->edgePower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->saturationBoostLoc, &cfg->saturationBoost,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessBoostLoc, &cfg->brightnessBoost,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->originalVisibilityLoc, &cfg->originalVisibility,
                 SHADER_UNIFORM_FLOAT);
}

void NeonGlowEffectUninit(NeonGlowEffect *e) { UnloadShader(e->shader); }

NeonGlowConfig NeonGlowConfigDefault(void) { return NeonGlowConfig{}; }

void NeonGlowRegisterParams(NeonGlowConfig *cfg) {
  ModEngineRegisterParam("neonGlow.glowIntensity", &cfg->glowIntensity, 0.5f,
                         5.0f);
  ModEngineRegisterParam("neonGlow.glowRadius", &cfg->glowRadius, 1.0f, 8.0f);
  ModEngineRegisterParam("neonGlow.coreSharpness", &cfg->coreSharpness, 0.0f,
                         1.0f);
  ModEngineRegisterParam("neonGlow.edgeThreshold", &cfg->edgeThreshold, 0.0f,
                         0.5f);
  ModEngineRegisterParam("neonGlow.edgePower", &cfg->edgePower, 0.5f, 3.0f);
  ModEngineRegisterParam("neonGlow.saturationBoost", &cfg->saturationBoost,
                         0.0f, 1.0f);
  ModEngineRegisterParam("neonGlow.brightnessBoost", &cfg->brightnessBoost,
                         0.0f, 1.0f);
  ModEngineRegisterParam("neonGlow.originalVisibility",
                         &cfg->originalVisibility, 0.0f, 1.0f);
}

void SetupNeonGlow(PostEffect *pe) {
  NeonGlowEffectSetup(&pe->neonGlow, &pe->effects.neonGlow);
}

// === UI ===

static void DrawNeonGlowParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  NeonGlowConfig *ng = &e->neonGlow;

  ModulatableSlider("Glow Intensity##neonglow", &ng->glowIntensity,
                    "neonGlow.glowIntensity", "%.2f", ms);
  ModulatableSlider("Glow Radius##neonglow", &ng->glowRadius,
                    "neonGlow.glowRadius", "%.1f", ms);
  ModulatableSlider("Core Sharpness##neonglow", &ng->coreSharpness,
                    "neonGlow.coreSharpness", "%.2f", ms);
  ModulatableSlider("Edge Threshold##neonglow", &ng->edgeThreshold,
                    "neonGlow.edgeThreshold", "%.3f", ms);
  ModulatableSlider("Edge Power##neonglow", &ng->edgePower,
                    "neonGlow.edgePower", "%.2f", ms);
  ModulatableSlider("Saturation Boost##neonglow", &ng->saturationBoost,
                    "neonGlow.saturationBoost", "%.2f", ms);
  ModulatableSlider("Brightness Boost##neonglow", &ng->brightnessBoost,
                    "neonGlow.brightnessBoost", "%.2f", ms);
  ModulatableSlider("Original Visibility##neonglow", &ng->originalVisibility,
                    "neonGlow.originalVisibility", "%.2f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_NEON_GLOW, NeonGlow, neonGlow, "Neon Glow", "GFX",
                5, EFFECT_FLAG_NONE, SetupNeonGlow, NULL, DrawNeonGlowParams)
// clang-format on
