// Ink wash effect module implementation

#include "ink_wash.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool InkWashEffectInit(InkWashEffect *e) {
  e->shader = LoadShader(NULL, "shaders/ink_wash.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->granulationLoc = GetShaderLocation(e->shader, "granulation");
  e->bleedStrengthLoc = GetShaderLocation(e->shader, "bleedStrength");
  e->bleedRadiusLoc = GetShaderLocation(e->shader, "bleedRadius");
  e->softnessLoc = GetShaderLocation(e->shader, "softness");

  return true;
}

void InkWashEffectSetup(InkWashEffect *e, const InkWashConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->granulationLoc, &cfg->granulation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bleedStrengthLoc, &cfg->bleedStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bleedRadiusLoc, &cfg->bleedRadius,
                 SHADER_UNIFORM_FLOAT);

  // Cast float softness to int before sending to shader
  int softness = (int)cfg->softness;
  SetShaderValue(e->shader, e->softnessLoc, &softness, SHADER_UNIFORM_INT);
}

void InkWashEffectUninit(InkWashEffect *e) { UnloadShader(e->shader); }

InkWashConfig InkWashConfigDefault(void) { return InkWashConfig{}; }

void InkWashRegisterParams(InkWashConfig *cfg) {
  ModEngineRegisterParam("inkWash.strength", &cfg->strength, 0.0f, 2.0f);
  ModEngineRegisterParam("inkWash.granulation", &cfg->granulation, 0.0f, 1.0f);
  ModEngineRegisterParam("inkWash.bleedStrength", &cfg->bleedStrength, 0.0f,
                         1.0f);
  ModEngineRegisterParam("inkWash.bleedRadius", &cfg->bleedRadius, 1.0f, 10.0f);
  ModEngineRegisterParam("inkWash.softness", &cfg->softness, 0.0f, 5.0f);
}

void SetupInkWash(PostEffect *pe) {
  InkWashEffectSetup(&pe->inkWash, &pe->effects.inkWash);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_INK_WASH, InkWash, inkWash, "Ink Wash", "ART", 4,
                EFFECT_FLAG_NONE, SetupInkWash, NULL)
// clang-format on
