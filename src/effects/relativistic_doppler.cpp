// Relativistic Doppler: velocity-dependent color shift with headlight beaming

#include "relativistic_doppler.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool RelativisticDopplerEffectInit(RelativisticDopplerEffect *e) {
  e->shader = LoadShader(NULL, "shaders/relativistic_doppler.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->velocityLoc = GetShaderLocation(e->shader, "velocity");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->aberrationLoc = GetShaderLocation(e->shader, "aberration");
  e->colorShiftLoc = GetShaderLocation(e->shader, "colorShift");
  e->headlightLoc = GetShaderLocation(e->shader, "headlight");

  return true;
}

void RelativisticDopplerEffectSetup(RelativisticDopplerEffect *e,
                                    const RelativisticDopplerConfig *cfg,
                                    float deltaTime) {
  (void)deltaTime; // No time accumulation in this effect

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->velocityLoc, &cfg->velocity,
                 SHADER_UNIFORM_FLOAT);

  float center[2] = {cfg->centerX, cfg->centerY};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->aberrationLoc, &cfg->aberration,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorShiftLoc, &cfg->colorShift,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->headlightLoc, &cfg->headlight,
                 SHADER_UNIFORM_FLOAT);
}

void RelativisticDopplerEffectUninit(RelativisticDopplerEffect *e) {
  UnloadShader(e->shader);
}

RelativisticDopplerConfig RelativisticDopplerConfigDefault(void) {
  return RelativisticDopplerConfig{};
}

void RelativisticDopplerRegisterParams(RelativisticDopplerConfig *cfg) {
  ModEngineRegisterParam("relativisticDoppler.velocity", &cfg->velocity, 0.0f,
                         0.99f);
  ModEngineRegisterParam("relativisticDoppler.centerX", &cfg->centerX, 0.0f,
                         1.0f);
  ModEngineRegisterParam("relativisticDoppler.centerY", &cfg->centerY, 0.0f,
                         1.0f);
  ModEngineRegisterParam("relativisticDoppler.aberration", &cfg->aberration,
                         0.0f, 1.0f);
  ModEngineRegisterParam("relativisticDoppler.colorShift", &cfg->colorShift,
                         0.0f, 1.0f);
  ModEngineRegisterParam("relativisticDoppler.headlight", &cfg->headlight, 0.0f,
                         1.0f);
}

void SetupRelativisticDoppler(PostEffect *pe) {
  RelativisticDopplerEffectSetup(&pe->relativisticDoppler,
                                 &pe->effects.relativisticDoppler,
                                 pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_RELATIVISTIC_DOPPLER, RelativisticDoppler,
                relativisticDoppler, "Relativistic Doppler", "MOT", 3,
                EFFECT_FLAG_NONE, SetupRelativisticDoppler, NULL)
// clang-format on
