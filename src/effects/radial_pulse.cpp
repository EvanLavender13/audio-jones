#include "radial_pulse.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

void RadialPulseEffectUninit(RadialPulseEffect *e) { UnloadShader(e->shader); }

RadialPulseConfig RadialPulseConfigDefault(void) { return RadialPulseConfig{}; }

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

void SetupRadialPulse(PostEffect *pe) {
  RadialPulseEffectSetup(&pe->radialPulse, &pe->effects.radialPulse,
                         pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_RADIAL_PULSE, RadialPulse, radialPulse,
                "Radial Pulse", "WARP", 1, EFFECT_FLAG_NONE,
                SetupRadialPulse, NULL)
// clang-format on
