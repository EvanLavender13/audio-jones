#include "random_volumetric.h"

#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"

#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"

#include <stddef.h>

static void CacheLocations(RandomVolumetricEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->cameraPhaseLoc = GetShaderLocation(e->shader, "cameraPhase");
  e->seedLoc = GetShaderLocation(e->shader, "seed");
  e->cycleSpeedLoc = GetShaderLocation(e->shader, "cycleSpeed");
  e->iterMinLoc = GetShaderLocation(e->shader, "iterMin");
  e->iterMaxLoc = GetShaderLocation(e->shader, "iterMax");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
  e->zoomPulseLoc = GetShaderLocation(e->shader, "zoomPulse");
  e->paletteRandomnessLoc = GetShaderLocation(e->shader, "paletteRandomness");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
}

bool RandomVolumetricEffectInit(RandomVolumetricEffect *e,
                                const RandomVolumetricConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/random_volumetric.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  CacheLocations(e);
  e->time = 0.0f;
  e->cameraPhase = 0.0f;
  return true;
}

void RandomVolumetricEffectSetup(RandomVolumetricEffect *e,
                                 const RandomVolumetricConfig *cfg,
                                 float deltaTime, const Texture2D &fftTexture) {
  e->time += deltaTime;
  e->cameraPhase += 4.0f * deltaTime;
  const float WRAP = 1000.0f;
  if (e->time > WRAP) {
    e->time -= WRAP;
  }
  if (e->cameraPhase > WRAP) {
    e->cameraPhase -= WRAP;
  }

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraPhaseLoc, &e->cameraPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->seedLoc, &cfg->seed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cycleSpeedLoc, &cfg->cycleSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterMinLoc, &cfg->iterMin, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterMaxLoc, &cfg->iterMax, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomPulseLoc, &cfg->zoomPulse,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paletteRandomnessLoc, &cfg->paletteRandomness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
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
}

void RandomVolumetricEffectUninit(RandomVolumetricEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void RandomVolumetricRegisterParams(RandomVolumetricConfig *cfg) {
  ModEngineRegisterParam("randomVolumetric.seed", &cfg->seed, 0.0f, 1000.0f);
  ModEngineRegisterParam("randomVolumetric.cycleSpeed", &cfg->cycleSpeed, 0.0f,
                         10.0f);
  ModEngineRegisterParam("randomVolumetric.iterMin", &cfg->iterMin, 1.0f,
                         20.0f);
  ModEngineRegisterParam("randomVolumetric.iterMax", &cfg->iterMax, 5.0f,
                         80.0f);
  ModEngineRegisterParam("randomVolumetric.zoom", &cfg->zoom, 1.0f, 20.0f);
  ModEngineRegisterParam("randomVolumetric.zoomPulse", &cfg->zoomPulse, 0.0f,
                         5.0f);
  ModEngineRegisterParam("randomVolumetric.paletteRandomness",
                         &cfg->paletteRandomness, 0.0f, 1.0f);
  ModEngineRegisterParam("randomVolumetric.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("randomVolumetric.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("randomVolumetric.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("randomVolumetric.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("randomVolumetric.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("randomVolumetric.blendIntensity",
                         &cfg->blendIntensity, 0.0f, 5.0f);
}

RandomVolumetricEffect *GetRandomVolumetricEffect(PostEffect *pe) {
  return (RandomVolumetricEffect *)
      pe->effectStates[TRANSFORM_RANDOM_VOLUMETRIC];
}

void SetupRandomVolumetric(PostEffect *pe) {
  RandomVolumetricEffectSetup(GetRandomVolumetricEffect(pe),
                              &pe->effects.randomVolumetric,
                              pe->currentDeltaTime, pe->fftTexture);
}

void SetupRandomVolumetricBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.randomVolumetric.blendIntensity,
                       pe->effects.randomVolumetric.blendMode);
}

// === UI ===

static void DrawRandomVolumetricParams(EffectConfig *e,
                                       const ModSources *modSources,
                                       ImU32 categoryGlow) {
  (void)categoryGlow;
  RandomVolumetricConfig *cfg = &e->randomVolumetric;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##rv", &cfg->baseFreq,
                    "randomVolumetric.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##rv", &cfg->maxFreq,
                    "randomVolumetric.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##rv", &cfg->gain, "randomVolumetric.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##rv", &cfg->curve, "randomVolumetric.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##rv", &cfg->baseBright,
                    "randomVolumetric.baseBright", "%.2f", modSources);

  ImGui::SeparatorText("Pattern");
  ModulatableSlider("Seed##rv", &cfg->seed, "randomVolumetric.seed", "%.1f",
                    modSources);
  ModulatableSlider("Cycle Speed##rv", &cfg->cycleSpeed,
                    "randomVolumetric.cycleSpeed", "%.2f", modSources);
  ModulatableSliderInt("Iter Min##rv", &cfg->iterMin,
                       "randomVolumetric.iterMin", modSources);
  ModulatableSliderInt("Iter Max##rv", &cfg->iterMax,
                       "randomVolumetric.iterMax", modSources);

  ImGui::SeparatorText("Zoom");
  ModulatableSlider("Zoom##rv", &cfg->zoom, "randomVolumetric.zoom", "%.2f",
                    modSources);
  ModulatableSlider("Zoom Pulse##rv", &cfg->zoomPulse,
                    "randomVolumetric.zoomPulse", "%.2f", modSources);

  ImGui::SeparatorText("Color");
  ModulatableSlider("Palette Randomness##rv", &cfg->paletteRandomness,
                    "randomVolumetric.paletteRandomness", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(randomVolumetric)
REGISTER_GENERATOR(TRANSFORM_RANDOM_VOLUMETRIC, RandomVolumetric, randomVolumetric,
                   "Random Volumetric",
                   SetupRandomVolumetricBlend, SetupRandomVolumetric, 13,
                   DrawRandomVolumetricParams, DrawOutput_randomVolumetric)
// clang-format on
