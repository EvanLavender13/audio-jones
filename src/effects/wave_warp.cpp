#include "wave_warp.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool WaveWarpEffectInit(WaveWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/wave_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->waveTypeLoc = GetShaderLocation(e->shader, "waveType");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");

  e->time = 0.0f;

  return true;
}

void WaveWarpEffectSetup(WaveWarpEffect *e, const WaveWarpConfig *cfg,
                         float deltaTime) {
  e->time += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->waveTypeLoc, &cfg->waveType, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
}

void WaveWarpEffectUninit(const WaveWarpEffect *e) { UnloadShader(e->shader); }

void WaveWarpRegisterParams(WaveWarpConfig *cfg) {
  ModEngineRegisterParam("waveWarp.strength", &cfg->strength, 0.0f, 1.0f);
  ModEngineRegisterParam("waveWarp.speed", &cfg->speed, 0.0f, 10.0f);
  ModEngineRegisterParam("waveWarp.scale", &cfg->scale, 0.1f, 4.0f);
}

// === UI ===

static void DrawWaveWarpParams(EffectConfig *ec, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  WaveWarpConfig *cfg = &ec->waveWarp;

  ImGui::Combo("Wave Type##waveWarp", &cfg->waveType,
               "Triangle\0Sine\0Sawtooth\0Square\0");
  ModulatableSlider("Strength##waveWarp", &cfg->strength, "waveWarp.strength",
                    "%.2f", ms);
  ModulatableSlider("Speed##waveWarp", &cfg->speed, "waveWarp.speed", "%.1f",
                    ms);
  ImGui::SliderInt("Octaves##waveWarp", &cfg->octaves, 1, 6);
  ModulatableSlider("Scale##waveWarp", &cfg->scale, "waveWarp.scale", "%.2f",
                    ms);
}

WaveWarpEffect *GetWaveWarpEffect(PostEffect *pe) {
  return (WaveWarpEffect *)pe->effectStates[TRANSFORM_WAVE_WARP];
}

void SetupWaveWarp(PostEffect *pe) {
  WaveWarpEffectSetup(GetWaveWarpEffect(pe), &pe->effects.waveWarp,
                      pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_WAVE_WARP, WaveWarp, waveWarp,
                "Wave Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupWaveWarp, NULL, DrawWaveWarpParams)
// clang-format on
