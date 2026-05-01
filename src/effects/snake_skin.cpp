// Snake Skin generator
// Tiled scale grid with spoke ridges, horizontal wave sway, and FFT twinkle

#include "snake_skin.h"
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

bool SnakeSkinEffectInit(SnakeSkinEffect *e, const SnakeSkinConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/snake_skin.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->scaleSizeLoc = GetShaderLocation(e->shader, "scaleSize");
  e->spikeAmountLoc = GetShaderLocation(e->shader, "spikeAmount");
  e->spikeFrequencyLoc = GetShaderLocation(e->shader, "spikeFrequency");
  e->sagLoc = GetShaderLocation(e->shader, "sag");
  e->scrollPhaseLoc = GetShaderLocation(e->shader, "scrollPhase");
  e->wavePhaseLoc = GetShaderLocation(e->shader, "wavePhase");
  e->waveAmplitudeLoc = GetShaderLocation(e->shader, "waveAmplitude");
  e->twinklePhaseLoc = GetShaderLocation(e->shader, "twinklePhase");
  e->twinkleIntensityLoc = GetShaderLocation(e->shader, "twinkleIntensity");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->scrollPhase = 0.0f;
  e->wavePhase = 0.0f;
  e->twinklePhase = 0.0f;

  return true;
}

void SnakeSkinEffectSetup(SnakeSkinEffect *e, const SnakeSkinConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture) {
  e->scrollPhase += cfg->scrollSpeed * deltaTime;
  e->wavePhase += cfg->waveSpeed * deltaTime;
  e->twinklePhase += cfg->twinkleSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->scaleSizeLoc, &cfg->scaleSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spikeAmountLoc, &cfg->spikeAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spikeFrequencyLoc, &cfg->spikeFrequency,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sagLoc, &cfg->sag, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->scrollPhaseLoc, &e->scrollPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wavePhaseLoc, &e->wavePhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveAmplitudeLoc, &cfg->waveAmplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twinklePhaseLoc, &e->twinklePhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twinkleIntensityLoc, &cfg->twinkleIntensity,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
}

void SnakeSkinEffectUninit(SnakeSkinEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void SnakeSkinRegisterParams(SnakeSkinConfig *cfg) {
  ModEngineRegisterParam("snakeSkin.scaleSize", &cfg->scaleSize, 3.0f, 30.0f);
  ModEngineRegisterParam("snakeSkin.spikeAmount", &cfg->spikeAmount, -1.0f,
                         2.0f);
  ModEngineRegisterParam("snakeSkin.spikeFrequency", &cfg->spikeFrequency, 0.0f,
                         16.0f);
  ModEngineRegisterParam("snakeSkin.sag", &cfg->sag, -2.0f, 0.0f);
  ModEngineRegisterParam("snakeSkin.scrollSpeed", &cfg->scrollSpeed, -5.0f,
                         5.0f);
  ModEngineRegisterParam("snakeSkin.waveSpeed", &cfg->waveSpeed, 0.1f, 5.0f);
  ModEngineRegisterParam("snakeSkin.waveAmplitude", &cfg->waveAmplitude, 0.0f,
                         0.5f);
  ModEngineRegisterParam("snakeSkin.twinkleSpeed", &cfg->twinkleSpeed, 0.1f,
                         5.0f);
  ModEngineRegisterParam("snakeSkin.twinkleIntensity", &cfg->twinkleIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("snakeSkin.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("snakeSkin.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("snakeSkin.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("snakeSkin.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("snakeSkin.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("snakeSkin.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

SnakeSkinEffect *GetSnakeSkinEffect(PostEffect *pe) {
  return (SnakeSkinEffect *)pe->effectStates[TRANSFORM_SNAKE_SKIN_BLEND];
}

void SetupSnakeSkin(PostEffect *pe) {
  SnakeSkinEffectSetup(GetSnakeSkinEffect(pe), &pe->effects.snakeSkin,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupSnakeSkinBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.snakeSkin.blendIntensity,
                       pe->effects.snakeSkin.blendMode);
}

// === UI ===

static void DrawSnakeSkinParams(EffectConfig *ec, const ModSources *ms,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  SnakeSkinConfig *c = &ec->snakeSkin;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##snakeSkin", &c->baseFreq,
                    "snakeSkin.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##snakeSkin", &c->maxFreq,
                    "snakeSkin.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##snakeSkin", &c->gain, "snakeSkin.gain", "%.1f", ms);
  ModulatableSlider("Contrast##snakeSkin", &c->curve, "snakeSkin.curve", "%.2f",
                    ms);
  ModulatableSlider("Base Bright##snakeSkin", &c->baseBright,
                    "snakeSkin.baseBright", "%.2f", ms);

  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Scale Size##snakeSkin", &c->scaleSize,
                    "snakeSkin.scaleSize", "%.1f", ms);
  ModulatableSlider("Spike Amount##snakeSkin", &c->spikeAmount,
                    "snakeSkin.spikeAmount", "%.2f", ms);
  ModulatableSlider("Spike Frequency##snakeSkin", &c->spikeFrequency,
                    "snakeSkin.spikeFrequency", "%.1f", ms);
  ModulatableSlider("Sag##snakeSkin", &c->sag, "snakeSkin.sag", "%.2f", ms);

  ImGui::SeparatorText("Animation");
  ModulatableSlider("Scroll Speed##snakeSkin", &c->scrollSpeed,
                    "snakeSkin.scrollSpeed", "%.2f", ms);
  ModulatableSlider("Wave Speed##snakeSkin", &c->waveSpeed,
                    "snakeSkin.waveSpeed", "%.2f", ms);
  ModulatableSlider("Wave Amplitude##snakeSkin", &c->waveAmplitude,
                    "snakeSkin.waveAmplitude", "%.2f", ms);

  ImGui::SeparatorText("Twinkle");
  ModulatableSlider("Twinkle Speed##snakeSkin", &c->twinkleSpeed,
                    "snakeSkin.twinkleSpeed", "%.2f", ms);
  ModulatableSlider("Twinkle Intensity##snakeSkin", &c->twinkleIntensity,
                    "snakeSkin.twinkleIntensity", "%.2f", ms);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(snakeSkin)
REGISTER_GENERATOR(TRANSFORM_SNAKE_SKIN_BLEND, SnakeSkin, snakeSkin,
                   "Snake Skin", SetupSnakeSkinBlend, SetupSnakeSkin,
                   12, DrawSnakeSkinParams, DrawOutput_snakeSkin)
// clang-format on
