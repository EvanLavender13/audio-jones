// Digital Shard generator
// Noise-driven angular shards with per-cell FFT reactivity

#include "digital_shard.h"
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

bool DigitalShardEffectInit(DigitalShardEffect *e,
                            const DigitalShardConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/digital_shard.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
  e->aberrationSpreadLoc = GetShaderLocation(e->shader, "aberrationSpread");
  e->noiseScaleLoc = GetShaderLocation(e->shader, "noiseScale");
  e->rotationLevelsLoc = GetShaderLocation(e->shader, "rotationLevels");
  e->softnessLoc = GetShaderLocation(e->shader, "softness");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void DigitalShardEffectSetup(DigitalShardEffect *e,
                             const DigitalShardConfig *cfg, float deltaTime,
                             const Texture2D &fftTexture) {
  e->time += cfg->speed * deltaTime;
  e->time = fmodf(e->time, 1000.0f);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->aberrationSpreadLoc, &cfg->aberrationSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseScaleLoc, &cfg->noiseScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLevelsLoc, &cfg->rotationLevels,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->softnessLoc, &cfg->softness,
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

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
}

void DigitalShardEffectUninit(DigitalShardEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void DigitalShardRegisterParams(DigitalShardConfig *cfg) {
  ModEngineRegisterParam("digitalShard.zoom", &cfg->zoom, 0.1f, 2.0f);
  ModEngineRegisterParam("digitalShard.aberrationSpread",
                         &cfg->aberrationSpread, 0.001f, 0.05f);
  ModEngineRegisterParam("digitalShard.noiseScale", &cfg->noiseScale, 16.0f,
                         256.0f);
  ModEngineRegisterParam("digitalShard.rotationLevels", &cfg->rotationLevels,
                         2.0f, 16.0f);
  ModEngineRegisterParam("digitalShard.softness", &cfg->softness, 0.0f, 1.0f);
  ModEngineRegisterParam("digitalShard.speed", &cfg->speed, 0.1f, 5.0f);
  ModEngineRegisterParam("digitalShard.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("digitalShard.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("digitalShard.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("digitalShard.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("digitalShard.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("digitalShard.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

DigitalShardEffect *GetDigitalShardEffect(PostEffect *pe) {
  return (DigitalShardEffect *)pe->effectStates[TRANSFORM_DIGITAL_SHARD_BLEND];
}

void SetupDigitalShard(PostEffect *pe) {
  DigitalShardEffectSetup(GetDigitalShardEffect(pe), &pe->effects.digitalShard,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupDigitalShardBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.digitalShard.blendIntensity,
                       pe->effects.digitalShard.blendMode);
}

// === UI ===

static void DrawDigitalShardParams(EffectConfig *ec, const ModSources *ms,
                                   ImU32 categoryGlow) {
  (void)categoryGlow;
  DigitalShardConfig *c = &ec->digitalShard;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##digitalShard", &c->baseFreq,
                    "digitalShard.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##digitalShard", &c->maxFreq,
                    "digitalShard.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##digitalShard", &c->gain, "digitalShard.gain", "%.1f",
                    ms);
  ModulatableSlider("Contrast##digitalShard", &c->curve, "digitalShard.curve",
                    "%.2f", ms);
  ModulatableSlider("Base Bright##digitalShard", &c->baseBright,
                    "digitalShard.baseBright", "%.2f", ms);

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##digitalShard", &c->iterations, 20, 100);
  ModulatableSlider("Zoom##digitalShard", &c->zoom, "digitalShard.zoom", "%.2f",
                    ms);
  ModulatableSlider("Aberration##digitalShard", &c->aberrationSpread,
                    "digitalShard.aberrationSpread", "%.3f", ms);
  ModulatableSlider("Noise Scale##digitalShard", &c->noiseScale,
                    "digitalShard.noiseScale", "%.1f", ms);
  ModulatableSlider("Rotation Levels##digitalShard", &c->rotationLevels,
                    "digitalShard.rotationLevels", "%.1f", ms);
  ModulatableSlider("Softness##digitalShard", &c->softness,
                    "digitalShard.softness", "%.2f", ms);
  ModulatableSlider("Speed##digitalShard", &c->speed, "digitalShard.speed",
                    "%.2f", ms);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(digitalShard)
REGISTER_GENERATOR(TRANSFORM_DIGITAL_SHARD_BLEND, DigitalShard, digitalShard,
                   "Digital Shard", SetupDigitalShardBlend, SetupDigitalShard,
                   12, DrawDigitalShardParams, DrawOutput_digitalShard)
// clang-format on
