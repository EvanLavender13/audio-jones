// Infinity Matrix effect module implementation
// Infinite recursive fractal zoom through self-similar binary digit glyphs

#include "infinity_matrix.h"
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

bool InfinityMatrixEffectInit(InfinityMatrixEffect *e,
                              const InfinityMatrixConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/infinity_matrix.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->zoomPhaseLoc = GetShaderLocation(e->shader, "zoomPhase");
  e->zoomScaleLoc = GetShaderLocation(e->shader, "zoomScale");
  e->recursionDepthLoc = GetShaderLocation(e->shader, "recursionDepth");
  e->fadeDepthLoc = GetShaderLocation(e->shader, "fadeDepth");
  e->waveAmplitudeLoc = GetShaderLocation(e->shader, "waveAmplitude");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->wavePhaseLoc = GetShaderLocation(e->shader, "wavePhase");
  e->colorFreqMapLoc = GetShaderLocation(e->shader, "colorFreqMap");
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

  e->zoomPhase = 0.0f;
  e->wavePhase = 0.0f;

  return true;
}

void InfinityMatrixEffectSetup(InfinityMatrixEffect *e,
                               const InfinityMatrixConfig *cfg, float deltaTime,
                               const Texture2D &fftTexture) {
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  e->zoomPhase += cfg->zoomSpeed * deltaTime;
  e->wavePhase += cfg->waveSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValue(e->shader, e->zoomPhaseLoc, &e->zoomPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomScaleLoc, &cfg->zoomScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->recursionDepthLoc, &cfg->recursionDepth,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->fadeDepthLoc, &cfg->fadeDepth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveAmplitudeLoc, &cfg->waveAmplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wavePhaseLoc, &e->wavePhase,
                 SHADER_UNIFORM_FLOAT);
  const int colorFreqMapInt = cfg->colorFreqMap ? 1 : 0;
  SetShaderValue(e->shader, e->colorFreqMapLoc, &colorFreqMapInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void InfinityMatrixEffectUninit(InfinityMatrixEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void InfinityMatrixRegisterParams(InfinityMatrixConfig *cfg) {
  ModEngineRegisterParam("infinityMatrix.zoomSpeed", &cfg->zoomSpeed, -3.0f,
                         3.0f);
  ModEngineRegisterParam("infinityMatrix.zoomScale", &cfg->zoomScale, 0.01f,
                         1.0f);
  ModEngineRegisterParam("infinityMatrix.fadeDepth", &cfg->fadeDepth, 1.0f,
                         6.0f);
  ModEngineRegisterParam("infinityMatrix.waveAmplitude", &cfg->waveAmplitude,
                         0.0f, 0.5f);
  ModEngineRegisterParam("infinityMatrix.waveFreq", &cfg->waveFreq, 0.5f, 8.0f);
  ModEngineRegisterParam("infinityMatrix.waveSpeed", &cfg->waveSpeed, 0.1f,
                         3.0f);
  ModEngineRegisterParam("infinityMatrix.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("infinityMatrix.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("infinityMatrix.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("infinityMatrix.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("infinityMatrix.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("infinityMatrix.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

InfinityMatrixEffect *GetInfinityMatrixEffect(PostEffect *pe) {
  return (InfinityMatrixEffect *)
      pe->effectStates[TRANSFORM_INFINITY_MATRIX_BLEND];
}

void SetupInfinityMatrix(PostEffect *pe) {
  InfinityMatrixEffectSetup(GetInfinityMatrixEffect(pe),
                            &pe->effects.infinityMatrix, pe->currentDeltaTime,
                            pe->fftTexture);
}

void SetupInfinityMatrixBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.infinityMatrix.blendIntensity,
                       pe->effects.infinityMatrix.blendMode);
}

// === UI ===

static void DrawInfinityMatrixParams(EffectConfig *e,
                                     const ModSources *modSources,
                                     ImU32 categoryGlow) {
  (void)categoryGlow;
  InfinityMatrixConfig *cfg = &e->infinityMatrix;

  // Audio
  ImGui::SeparatorText("Audio");
  ImGui::Checkbox("Color Freq Map##infinityMatrix", &cfg->colorFreqMap);
  ModulatableSlider("Base Freq (Hz)##infinityMatrix", &cfg->baseFreq,
                    "infinityMatrix.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##infinityMatrix", &cfg->maxFreq,
                    "infinityMatrix.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##infinityMatrix", &cfg->gain, "infinityMatrix.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##infinityMatrix", &cfg->curve,
                    "infinityMatrix.curve", "%.2f", modSources);
  ModulatableSlider("Base Bright##infinityMatrix", &cfg->baseBright,
                    "infinityMatrix.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Recursion##infinityMatrix", &cfg->recursionDepth, 2, 8);
  ModulatableSlider("Fade Depth##infinityMatrix", &cfg->fadeDepth,
                    "infinityMatrix.fadeDepth", "%.2f", modSources);
  ModulatableSlider("Zoom Scale##infinityMatrix", &cfg->zoomScale,
                    "infinityMatrix.zoomScale", "%.3f", modSources);

  // Wave
  ImGui::SeparatorText("Wave");
  ModulatableSlider("Wave Amplitude##infinityMatrix", &cfg->waveAmplitude,
                    "infinityMatrix.waveAmplitude", "%.3f", modSources);
  ModulatableSlider("Wave Freq##infinityMatrix", &cfg->waveFreq,
                    "infinityMatrix.waveFreq", "%.2f", modSources);
  ModulatableSlider("Wave Speed##infinityMatrix", &cfg->waveSpeed,
                    "infinityMatrix.waveSpeed", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Zoom Speed##infinityMatrix", &cfg->zoomSpeed,
                    "infinityMatrix.zoomSpeed", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(infinityMatrix)
REGISTER_GENERATOR(TRANSFORM_INFINITY_MATRIX_BLEND, InfinityMatrix, infinityMatrix, "Infinity Matrix",
                   SetupInfinityMatrixBlend, SetupInfinityMatrix, 12, DrawInfinityMatrixParams, DrawOutput_infinityMatrix)
// clang-format on
