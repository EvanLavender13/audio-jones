#include "sine_warp.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool SineWarpEffectInit(SineWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/sine_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->octaveRotationLoc = GetShaderLocation(e->shader, "octaveRotation");
  e->radialModeLoc = GetShaderLocation(e->shader, "radialMode");
  e->depthBlendLoc = GetShaderLocation(e->shader, "depthBlend");

  e->time = 0.0f;

  return true;
}

void SineWarpEffectSetup(SineWarpEffect *e, const SineWarpConfig *cfg,
                         float deltaTime) {
  e->time += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octaveRotationLoc, &cfg->octaveRotation,
                 SHADER_UNIFORM_FLOAT);

  int radialMode = cfg->radialMode ? 1 : 0;
  SetShaderValue(e->shader, e->radialModeLoc, &radialMode, SHADER_UNIFORM_INT);

  int depthBlend = cfg->depthBlend ? 1 : 0;
  SetShaderValue(e->shader, e->depthBlendLoc, &depthBlend, SHADER_UNIFORM_INT);
}

void SineWarpEffectUninit(const SineWarpEffect *e) { UnloadShader(e->shader); }

void SineWarpRegisterParams(SineWarpConfig *cfg) {
  ModEngineRegisterParam("sineWarp.strength", &cfg->strength, 0.0f, 2.0f);
  ModEngineRegisterParam("sineWarp.octaveRotation", &cfg->octaveRotation,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

// === UI ===

static void DrawSineWarpParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  ImGui::SliderInt("Octaves##sineWarp", &e->sineWarp.octaves, 1, 8);
  ModulatableSlider("Strength##sineWarp", &e->sineWarp.strength,
                    "sineWarp.strength", "%.2f", ms);
  SliderSpeedDeg("Speed##sineWarp", &e->sineWarp.speed, -180.0f, 180.0f);
  ModulatableSliderAngleDeg("Octave Rotation##sineWarp",
                            &e->sineWarp.octaveRotation,
                            "sineWarp.octaveRotation", ms);
  ImGui::Checkbox("Radial Mode##sineWarp", &e->sineWarp.radialMode);
  ImGui::Checkbox("Depth Blend##sineWarp", &e->sineWarp.depthBlend);
}

void SetupSineWarp(PostEffect *pe) {
  SineWarpEffectSetup(&pe->sineWarp, &pe->effects.sineWarp,
                      pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_SINE_WARP, SineWarp, sineWarp, "Sine Warp", "WARP",
                1, EFFECT_FLAG_NONE, SetupSineWarp, NULL, DrawSineWarpParams)
// clang-format on
