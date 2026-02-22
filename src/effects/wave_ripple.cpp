#include "wave_ripple.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool WaveRippleEffectInit(WaveRippleEffect *e) {
  e->shader = LoadShader(NULL, "shaders/wave_ripple.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->frequencyLoc = GetShaderLocation(e->shader, "frequency");
  e->steepnessLoc = GetShaderLocation(e->shader, "steepness");
  e->decayLoc = GetShaderLocation(e->shader, "decay");
  e->centerHoleLoc = GetShaderLocation(e->shader, "centerHole");
  e->originLoc = GetShaderLocation(e->shader, "origin");
  e->shadeEnabledLoc = GetShaderLocation(e->shader, "shadeEnabled");
  e->shadeIntensityLoc = GetShaderLocation(e->shader, "shadeIntensity");

  e->time = 0.0f;

  return true;
}

void WaveRippleEffectSetup(WaveRippleEffect *e, WaveRippleConfig *cfg,
                           float deltaTime) {
  e->time += cfg->speed * deltaTime;

  // Compute origin: use Lissajous if amplitude > 0, else static originX/Y
  float originX = cfg->originX;
  float originY = cfg->originY;
  if (cfg->originLissajous.amplitude > 0.0f) {
    float offsetX;
    float offsetY;
    DualLissajousUpdate(&cfg->originLissajous, deltaTime, 0.0f, &offsetX,
                        &offsetY);
    originX += offsetX;
    originY += offsetY;
  }
  float origin[2] = {originX, originY};

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->frequencyLoc, &cfg->frequency,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->steepnessLoc, &cfg->steepness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->decayLoc, &cfg->decay, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerHoleLoc, &cfg->centerHole,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->originLoc, origin, SHADER_UNIFORM_VEC2);

  int shadeEnabled = cfg->shadeEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->shadeEnabledLoc, &shadeEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->shadeIntensityLoc, &cfg->shadeIntensity,
                 SHADER_UNIFORM_FLOAT);
}

void WaveRippleEffectUninit(WaveRippleEffect *e) { UnloadShader(e->shader); }

WaveRippleConfig WaveRippleConfigDefault(void) { return WaveRippleConfig{}; }

void WaveRippleRegisterParams(WaveRippleConfig *cfg) {
  ModEngineRegisterParam("waveRipple.strength", &cfg->strength, 0.0f, 0.5f);
  ModEngineRegisterParam("waveRipple.frequency", &cfg->frequency, 1.0f, 20.0f);
  ModEngineRegisterParam("waveRipple.steepness", &cfg->steepness, 0.0f, 1.0f);
  ModEngineRegisterParam("waveRipple.decay", &cfg->decay, 0.0f, 50.0f);
  ModEngineRegisterParam("waveRipple.centerHole", &cfg->centerHole, 0.0f, 0.5f);
  ModEngineRegisterParam("waveRipple.originX", &cfg->originX, 0.0f, 1.0f);
  ModEngineRegisterParam("waveRipple.originY", &cfg->originY, 0.0f, 1.0f);
  ModEngineRegisterParam("waveRipple.shadeIntensity", &cfg->shadeIntensity,
                         0.0f, 0.5f);
}

// === UI ===

static void DrawWaveRippleParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  ImGui::SliderInt("Octaves##waveripple", &e->waveRipple.octaves, 1, 4);
  ModulatableSlider("Strength##waveripple", &e->waveRipple.strength,
                    "waveRipple.strength", "%.3f", ms);
  ImGui::SliderFloat("Speed##waveripple", &e->waveRipple.speed, 0.0f, 5.0f,
                     "%.2f rad/s");
  ModulatableSlider("Frequency##waveripple", &e->waveRipple.frequency,
                    "waveRipple.frequency", "%.1f", ms);
  ModulatableSlider("Steepness##waveripple", &e->waveRipple.steepness,
                    "waveRipple.steepness", "%.2f", ms);
  ModulatableSlider("Decay##waveripple", &e->waveRipple.decay,
                    "waveRipple.decay", "%.1f", ms);
  ModulatableSlider("Center Hole##waveripple", &e->waveRipple.centerHole,
                    "waveRipple.centerHole", "%.2f", ms);
  if (TreeNodeAccented("Origin##waveripple", glow)) {
    ModulatableSlider("X##waveripple", &e->waveRipple.originX,
                      "waveRipple.originX", "%.2f", ms);
    ModulatableSlider("Y##waveripple", &e->waveRipple.originY,
                      "waveRipple.originY", "%.2f", ms);
    DrawLissajousControls(&e->waveRipple.originLissajous, "waveripple_origin",
                          NULL, NULL, 5.0f);
    TreeNodeAccentedPop();
  }
  ImGui::Checkbox("Shading##waveripple", &e->waveRipple.shadeEnabled);
  if (e->waveRipple.shadeEnabled) {
    ModulatableSlider("Shade Intensity##waveripple",
                      &e->waveRipple.shadeIntensity,
                      "waveRipple.shadeIntensity", "%.2f", ms);
  }
}

void SetupWaveRipple(PostEffect *pe) {
  WaveRippleEffectSetup(&pe->waveRipple, &pe->effects.waveRipple,
                        pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_WAVE_RIPPLE, WaveRipple, waveRipple, "Wave Ripple",
                "WARP", 1, EFFECT_FLAG_NONE, SetupWaveRipple, NULL, DrawWaveRippleParams)
// clang-format on
