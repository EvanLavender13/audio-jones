// Relativistic Doppler: velocity-dependent color shift with headlight beaming

#include "relativistic_doppler.h"

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
  RelativisticDopplerConfig cfg;
  cfg.enabled = false;
  cfg.velocity = 0.5f;
  cfg.centerX = 0.5f;
  cfg.centerY = 0.5f;
  cfg.aberration = 1.0f;
  cfg.colorShift = 1.0f;
  cfg.headlight = 0.3f;
  return cfg;
}
