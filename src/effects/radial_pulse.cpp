#include "radial_pulse.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool RadialPulseEffectInit(RadialPulseEffect *e) {
  e->shader = LoadShader(NULL, "shaders/radial_pulse.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->radialFreqLoc = GetShaderLocation(e->shader, "radialFreq");
  e->radialAmpLoc = GetShaderLocation(e->shader, "radialAmp");
  e->segmentsLoc = GetShaderLocation(e->shader, "segments");
  e->angularAmpLoc = GetShaderLocation(e->shader, "angularAmp");
  e->petalAmpLoc = GetShaderLocation(e->shader, "petalAmp");
  e->phaseLoc = GetShaderLocation(e->shader, "phase");
  e->spiralTwistLoc = GetShaderLocation(e->shader, "spiralTwist");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->octaveRotationLoc = GetShaderLocation(e->shader, "octaveRotation");
  e->depthBlendLoc = GetShaderLocation(e->shader, "depthBlend");

  e->time = 0.0f;

  return true;
}

void RadialPulseEffectSetup(RadialPulseEffect *e, const RadialPulseConfig *cfg,
                            float deltaTime) {
  e->time += cfg->phaseSpeed * deltaTime;

  SetShaderValue(e->shader, e->radialFreqLoc, &cfg->radialFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->radialAmpLoc, &cfg->radialAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->segmentsLoc, &cfg->segments, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->angularAmpLoc, &cfg->angularAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->petalAmpLoc, &cfg->petalAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralTwistLoc, &cfg->spiralTwist,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->octaveRotationLoc, &cfg->octaveRotation,
                 SHADER_UNIFORM_FLOAT);

  int depthBlend = cfg->depthBlend ? 1 : 0;
  SetShaderValue(e->shader, e->depthBlendLoc, &depthBlend, SHADER_UNIFORM_INT);
}

void RadialPulseEffectUninit(const RadialPulseEffect *e) {
  UnloadShader(e->shader);
}

void RadialPulseRegisterParams(RadialPulseConfig *cfg) {
  ModEngineRegisterParam("radialPulse.radialFreq", &cfg->radialFreq, 1.0f,
                         30.0f);
  ModEngineRegisterParam("radialPulse.radialAmp", &cfg->radialAmp, -0.3f, 0.3f);
  ModEngineRegisterParam("radialPulse.angularAmp", &cfg->angularAmp, -0.5f,
                         0.5f);
  ModEngineRegisterParam("radialPulse.petalAmp", &cfg->petalAmp, -1.0f, 1.0f);
  ModEngineRegisterParam("radialPulse.spiralTwist", &cfg->spiralTwist,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("radialPulse.octaveRotation", &cfg->octaveRotation,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

// === UI ===

static void DrawRadialPulseParams(EffectConfig *e, const ModSources *ms,
                                  ImU32 glow) {
  (void)glow;
  RadialPulseConfig *rp = &e->radialPulse;

  ModulatableSlider("Radial Freq##radpulse", &rp->radialFreq,
                    "radialPulse.radialFreq", "%.1f", ms);
  ModulatableSlider("Radial Amp##radpulse", &rp->radialAmp,
                    "radialPulse.radialAmp", "%.3f", ms);
  ImGui::SliderInt("Segments##radpulse", &rp->segments, 2, 16);
  ModulatableSlider("Swirl##radpulse", &rp->angularAmp,
                    "radialPulse.angularAmp", "%.3f", ms);
  ModulatableSlider("Petal##radpulse", &rp->petalAmp, "radialPulse.petalAmp",
                    "%.2f", ms);
  ImGui::SliderFloat("Phase Speed##radpulse", &rp->phaseSpeed, -5.0f, 5.0f,
                     "%.2f");
  ModulatableSliderAngleDeg("Spiral Twist##radpulse", &rp->spiralTwist,
                            "radialPulse.spiralTwist", ms);
  ImGui::SliderInt("Octaves##radpulse", &rp->octaves, 1, 8);
  ModulatableSliderAngleDeg("Octave Rotation##radpulse", &rp->octaveRotation,
                            "radialPulse.octaveRotation", ms);
  ImGui::Checkbox("Depth Blend##radpulse", &rp->depthBlend);
}

void SetupRadialPulse(PostEffect *pe) {
  RadialPulseEffectSetup(&pe->radialPulse, &pe->effects.radialPulse,
                         pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_RADIAL_PULSE, RadialPulse, radialPulse,
                "Radial Pulse", "WARP", 1, EFFECT_FLAG_NONE,
                SetupRadialPulse, NULL, DrawRadialPulseParams)
// clang-format on
