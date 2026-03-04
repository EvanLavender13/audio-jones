// Spark Flash effect module implementation
// Twinkling four-arm crosshair sparks with sine lifecycle and FFT-reactive
// brightness

#include "spark_flash.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
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

bool SparkFlashEffectInit(SparkFlashEffect *e, const SparkFlashConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/spark_flash.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->lifetimeLoc = GetShaderLocation(e->shader, "lifetime");
  e->armSoftnessLoc = GetShaderLocation(e->shader, "armSoftness");
  e->starSoftnessLoc = GetShaderLocation(e->shader, "starSoftness");
  e->armBrightnessLoc = GetShaderLocation(e->shader, "armBrightness");
  e->starBrightnessLoc = GetShaderLocation(e->shader, "starBrightness");
  e->armReachLoc = GetShaderLocation(e->shader, "armReach");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void SparkFlashEffectSetup(SparkFlashEffect *e, const SparkFlashConfig *cfg,
                           float deltaTime, Texture2D fftTexture) {
  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  e->time += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lifetimeLoc, &cfg->lifetime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->armSoftnessLoc, &cfg->armSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starSoftnessLoc, &cfg->starSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->armBrightnessLoc, &cfg->armBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starBrightnessLoc, &cfg->starBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->armReachLoc, &cfg->armReach,
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
}

void SparkFlashEffectUninit(SparkFlashEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

SparkFlashConfig SparkFlashConfigDefault(void) { return SparkFlashConfig{}; }

void SparkFlashRegisterParams(SparkFlashConfig *cfg) {
  ModEngineRegisterParam("sparkFlash.lifetime", &cfg->lifetime, 0.05f, 2.0f);
  ModEngineRegisterParam("sparkFlash.armSoftness", &cfg->armSoftness, 0.1f,
                         10.0f);
  ModEngineRegisterParam("sparkFlash.starSoftness", &cfg->starSoftness, 0.1f,
                         10.0f);
  ModEngineRegisterParam("sparkFlash.armBrightness", &cfg->armBrightness, 0.1f,
                         10.0f);
  ModEngineRegisterParam("sparkFlash.starBrightness", &cfg->starBrightness,
                         0.1f, 10.0f);
  ModEngineRegisterParam("sparkFlash.armReach", &cfg->armReach, 0.1f, 2.0f);
  ModEngineRegisterParam("sparkFlash.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("sparkFlash.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("sparkFlash.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("sparkFlash.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("sparkFlash.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("sparkFlash.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSparkFlash(PostEffect *pe) {
  SparkFlashEffectSetup(&pe->sparkFlash, &pe->effects.sparkFlash,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupSparkFlashBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.sparkFlash.blendIntensity,
                       pe->effects.sparkFlash.blendMode);
}

// === UI ===

static void DrawSparkFlashParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  SparkFlashConfig *sf = &e->sparkFlash;

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Layers##sparkFlash", &sf->layers, 2, 16);
  ModulatableSlider("Lifetime##sparkFlash", &sf->lifetime,
                    "sparkFlash.lifetime", "%.2f", modSources);
  ModulatableSlider("Arm Softness##sparkFlash", &sf->armSoftness,
                    "sparkFlash.armSoftness", "%.2f", modSources);
  ModulatableSlider("Star Softness##sparkFlash", &sf->starSoftness,
                    "sparkFlash.starSoftness", "%.2f", modSources);
  ModulatableSlider("Arm Brightness##sparkFlash", &sf->armBrightness,
                    "sparkFlash.armBrightness", "%.1f", modSources);
  ModulatableSlider("Star Brightness##sparkFlash", &sf->starBrightness,
                    "sparkFlash.starBrightness", "%.1f", modSources);
  ModulatableSlider("Arm Reach##sparkFlash", &sf->armReach,
                    "sparkFlash.armReach", "%.2f", modSources);

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##sparkFlash", &sf->baseFreq,
                    "sparkFlash.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##sparkFlash", &sf->maxFreq,
                    "sparkFlash.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##sparkFlash", &sf->gain, "sparkFlash.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##sparkFlash", &sf->curve, "sparkFlash.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##sparkFlash", &sf->baseBright,
                    "sparkFlash.baseBright", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(sparkFlash)
REGISTER_GENERATOR(TRANSFORM_SPARK_FLASH_BLEND, SparkFlash, sparkFlash, "Spark Flash",
                   SetupSparkFlashBlend, SetupSparkFlash, 13, DrawSparkFlashParams, DrawOutput_sparkFlash)
// clang-format on
