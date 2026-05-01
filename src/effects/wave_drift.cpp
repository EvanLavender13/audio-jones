#include "wave_drift.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool WaveDriftEffectInit(WaveDriftEffect *e) {
  e->shader = LoadShader(NULL, "shaders/wave_drift.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->waveTypeLoc = GetShaderLocation(e->shader, "waveType");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->octaveRotationLoc = GetShaderLocation(e->shader, "octaveRotation");
  e->radialModeLoc = GetShaderLocation(e->shader, "radialMode");
  e->depthBlendLoc = GetShaderLocation(e->shader, "depthBlend");

  e->time = 0.0f;

  return true;
}

void WaveDriftEffectSetup(WaveDriftEffect *e, const WaveDriftConfig *cfg,
                          float deltaTime) {
  e->time += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveTypeLoc, &cfg->waveType, SHADER_UNIFORM_INT);
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

void WaveDriftEffectUninit(const WaveDriftEffect *e) {
  UnloadShader(e->shader);
}

void WaveDriftRegisterParams(WaveDriftConfig *cfg) {
  ModEngineRegisterParam("waveDrift.strength", &cfg->strength, 0.0f, 2.0f);
  ModEngineRegisterParam("waveDrift.octaveRotation", &cfg->octaveRotation,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

// === UI ===

static void DrawWaveDriftParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Octaves##waveDrift", &e->waveDrift.octaves, 1, 8);
  ModulatableSlider("Strength##waveDrift", &e->waveDrift.strength,
                    "waveDrift.strength", "%.2f", ms);
  ModulatableSliderAngleDeg("Octave Rotation##waveDrift",
                            &e->waveDrift.octaveRotation,
                            "waveDrift.octaveRotation", ms);
  ImGui::SeparatorText("Wave");
  ImGui::Combo("Wave Type##waveDrift", &e->waveDrift.waveType,
               "Triangle\0Sine\0Sawtooth\0Square\0");
  ImGui::Checkbox("Radial Mode##waveDrift", &e->waveDrift.radialMode);
  ImGui::Checkbox("Depth Blend##waveDrift", &e->waveDrift.depthBlend);
  ImGui::SeparatorText("Animation");
  SliderSpeedDeg("Speed##waveDrift", &e->waveDrift.speed, -180.0f, 180.0f);
}

WaveDriftEffect *GetWaveDriftEffect(PostEffect *pe) {
  return (WaveDriftEffect *)pe->effectStates[TRANSFORM_WAVE_DRIFT];
}

void SetupWaveDrift(PostEffect *pe) {
  WaveDriftEffectSetup(GetWaveDriftEffect(pe), &pe->effects.waveDrift,
                       pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_WAVE_DRIFT, WaveDrift, waveDrift, "Wave Drift", "MOT",
                3, EFFECT_FLAG_NONE, SetupWaveDrift, NULL, DrawWaveDriftParams)
// clang-format on
