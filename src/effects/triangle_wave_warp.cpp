#include "triangle_wave_warp.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool TriangleWaveWarpEffectInit(TriangleWaveWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/triangle_wave_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->noiseStrengthLoc = GetShaderLocation(e->shader, "noiseStrength");
  e->timeLoc = GetShaderLocation(e->shader, "time");

  e->time = 0.0f;

  return true;
}

void TriangleWaveWarpEffectSetup(TriangleWaveWarpEffect *e,
                                 const TriangleWaveWarpConfig *cfg,
                                 float deltaTime) {
  e->time += cfg->noiseSpeed * deltaTime;

  SetShaderValue(e->shader, e->noiseStrengthLoc, &cfg->noiseStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
}

void TriangleWaveWarpEffectUninit(TriangleWaveWarpEffect *e) {
  UnloadShader(e->shader);
}

TriangleWaveWarpConfig TriangleWaveWarpConfigDefault(void) {
  return TriangleWaveWarpConfig{};
}

void TriangleWaveWarpRegisterParams(TriangleWaveWarpConfig *cfg) {
  ModEngineRegisterParam("triangleWaveWarp.noiseStrength", &cfg->noiseStrength,
                         0.0f, 1.0f);
  ModEngineRegisterParam("triangleWaveWarp.noiseSpeed", &cfg->noiseSpeed, 0.0f,
                         10.0f);
}

// === UI ===

static void DrawTriangleWaveWarpParams(EffectConfig *e, const ModSources *ms,
                                       ImU32 glow) {
  (void)glow;
  TriangleWaveWarpConfig *tw = &e->triangleWaveWarp;

  ModulatableSlider("Strength##triWaveWarp", &tw->noiseStrength,
                    "triangleWaveWarp.noiseStrength", "%.2f", ms);
  ModulatableSlider("Speed##triWaveWarp", &tw->noiseSpeed,
                    "triangleWaveWarp.noiseSpeed", "%.1f", ms);
}

void SetupTriangleWaveWarp(PostEffect *pe) {
  TriangleWaveWarpEffectSetup(&pe->triangleWaveWarp,
                              &pe->effects.triangleWaveWarp,
                              pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_TRIANGLE_WAVE_WARP, TriangleWaveWarp, triangleWaveWarp,
                "Tri-Wave Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupTriangleWaveWarp, NULL, DrawTriangleWaveWarpParams)
// clang-format on
