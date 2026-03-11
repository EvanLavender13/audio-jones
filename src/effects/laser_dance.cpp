// Laser Dance generator
// Raymarched cosine field with animated laser-like beams

#include "laser_dance.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool LaserDanceEffectInit(LaserDanceEffect *e, const LaserDanceConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/laser_dance.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->freqRatioLoc = GetShaderLocation(e->shader, "freqRatio");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->colorPhaseLoc = GetShaderLocation(e->shader, "colorPhase");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->warpAmountLoc = GetShaderLocation(e->shader, "warpAmount");
  e->warpTimeLoc = GetShaderLocation(e->shader, "warpTime");
  e->warpFreqLoc = GetShaderLocation(e->shader, "warpFreq");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;
  e->warpTime = 0.0f;
  e->colorPhase = 0.0f;

  return true;
}

void LaserDanceEffectSetup(LaserDanceEffect *e, const LaserDanceConfig *cfg,
                           float deltaTime, Texture2D fftTexture) {
  e->time += cfg->speed * deltaTime;
  e->warpTime += cfg->warpSpeed * deltaTime;
  e->warpTime = fmodf(e->warpTime, 62.831853f);
  e->colorPhase += cfg->colorSpeed * deltaTime;
  e->colorPhase = fmodf(e->colorPhase, 1.0f);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqRatioLoc, &cfg->freqRatio,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorPhaseLoc, &e->colorPhase,
                 SHADER_UNIFORM_FLOAT);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpAmountLoc, &cfg->warpAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpTimeLoc, &e->warpTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpFreqLoc, &cfg->warpFreq,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
}

void LaserDanceEffectUninit(LaserDanceEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

LaserDanceConfig LaserDanceConfigDefault(void) { return LaserDanceConfig{}; }

void LaserDanceRegisterParams(LaserDanceConfig *cfg) {
  ModEngineRegisterParam("laserDance.speed", &cfg->speed, 0.1f, 5.0f);
  ModEngineRegisterParam("laserDance.freqRatio", &cfg->freqRatio, 0.3f, 1.5f);
  ModEngineRegisterParam("laserDance.brightness", &cfg->brightness, 0.5f, 3.0f);
  ModEngineRegisterParam("laserDance.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("laserDance.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("laserDance.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("laserDance.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("laserDance.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("laserDance.warpAmount", &cfg->warpAmount, 0.0f, 1.5f);
  ModEngineRegisterParam("laserDance.warpSpeed", &cfg->warpSpeed, 0.1f, 3.0f);
  ModEngineRegisterParam("laserDance.warpFreq", &cfg->warpFreq, 0.1f, 2.0f);
  ModEngineRegisterParam("laserDance.colorSpeed", &cfg->colorSpeed, 0.0f, 3.0f);
  ModEngineRegisterParam("laserDance.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupLaserDance(PostEffect *pe) {
  LaserDanceEffectSetup(&pe->laserDance, &pe->effects.laserDance,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupLaserDanceBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.laserDance.blendIntensity,
                       pe->effects.laserDance.blendMode);
}

// === UI ===

static void DrawLaserDanceParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  LaserDanceConfig *c = &e->laserDance;

  ModulatableSlider("Speed##laserDance", &c->speed, "laserDance.speed", "%.2f",
                    modSources);
  ModulatableSlider("Freq Ratio##laserDance", &c->freqRatio,
                    "laserDance.freqRatio", "%.2f", modSources);
  ModulatableSlider("Brightness##laserDance", &c->brightness,
                    "laserDance.brightness", "%.2f", modSources);

  ImGui::SeparatorText("Warp");
  ModulatableSlider("Amount##laserDanceWarp", &c->warpAmount,
                    "laserDance.warpAmount", "%.2f", modSources);
  ModulatableSlider("Speed##laserDanceWarp", &c->warpSpeed,
                    "laserDance.warpSpeed", "%.2f", modSources);
  ModulatableSlider("Freq##laserDanceWarp", &c->warpFreq, "laserDance.warpFreq",
                    "%.2f", modSources);

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##laserDance", &c->baseFreq,
                    "laserDance.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##laserDance", &c->maxFreq,
                    "laserDance.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##laserDance", &c->gain, "laserDance.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##laserDance", &c->curve, "laserDance.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##laserDance", &c->baseBright,
                    "laserDance.baseBright", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(laserDance)
REGISTER_GENERATOR(TRANSFORM_LASER_DANCE_BLEND, LaserDance, laserDance,
                   "Laser Dance", SetupLaserDanceBlend, SetupLaserDance,
                   11, DrawLaserDanceParams, DrawOutput_laserDance)
// clang-format on
