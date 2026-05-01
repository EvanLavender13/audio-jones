// Relativistic Doppler: velocity-dependent color shift with headlight beaming

#include "relativistic_doppler.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
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

void RelativisticDopplerEffectSetup(const RelativisticDopplerEffect *e,
                                    const RelativisticDopplerConfig *cfg,
                                    float deltaTime) {
  (void)deltaTime; // No time accumulation in this effect

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->velocityLoc, &cfg->velocity,
                 SHADER_UNIFORM_FLOAT);

  const float center[2] = {cfg->centerX, cfg->centerY};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->aberrationLoc, &cfg->aberration,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorShiftLoc, &cfg->colorShift,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->headlightLoc, &cfg->headlight,
                 SHADER_UNIFORM_FLOAT);
}

void RelativisticDopplerEffectUninit(const RelativisticDopplerEffect *e) {
  UnloadShader(e->shader);
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

// === UI ===

static void DrawRelativisticDopplerParams(EffectConfig *e, const ModSources *ms,
                                          ImU32 glow) {
  (void)glow;
  ModulatableSlider("Velocity##reldop", &e->relativisticDoppler.velocity,
                    "relativisticDoppler.velocity", "%.2f", ms);
  ImGui::SeparatorText("Center");
  ModulatableSlider("X##reldopcenter", &e->relativisticDoppler.centerX,
                    "relativisticDoppler.centerX", "%.2f", ms);
  ModulatableSlider("Y##reldopcenter", &e->relativisticDoppler.centerY,
                    "relativisticDoppler.centerY", "%.2f", ms);
  ModulatableSlider("Aberration##reldop", &e->relativisticDoppler.aberration,
                    "relativisticDoppler.aberration", "%.2f", ms);
  ModulatableSlider("Color Shift##reldop", &e->relativisticDoppler.colorShift,
                    "relativisticDoppler.colorShift", "%.2f", ms);
  ModulatableSlider("Headlight##reldop", &e->relativisticDoppler.headlight,
                    "relativisticDoppler.headlight", "%.2f", ms);
}

RelativisticDopplerEffect *GetRelativisticDopplerEffect(PostEffect *pe) {
  return (RelativisticDopplerEffect *)
      pe->effectStates[TRANSFORM_RELATIVISTIC_DOPPLER];
}

void SetupRelativisticDoppler(PostEffect *pe) {
  RelativisticDopplerEffectSetup(GetRelativisticDopplerEffect(pe),
                                 &pe->effects.relativisticDoppler,
                                 pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_RELATIVISTIC_DOPPLER, RelativisticDoppler,
                relativisticDoppler, "Relativistic Doppler", "MOT", 3,
                EFFECT_FLAG_NONE, SetupRelativisticDoppler, NULL,
                DrawRelativisticDopplerParams)
// clang-format on
