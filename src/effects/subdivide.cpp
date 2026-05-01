// Subdivide generator - recursive BSP subdivision with squishy Catmull-Rom
// cut positions, FFT-driven cell brightness, and gradient coloring

#include "subdivide.h"
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

bool SubdivideEffectInit(SubdivideEffect *e, const SubdivideConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/subdivide.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->squishLoc = GetShaderLocation(e->shader, "squish");
  e->thresholdLoc = GetShaderLocation(e->shader, "threshold");
  e->maxIterationsLoc = GetShaderLocation(e->shader, "maxIterations");
  e->edgeDarkenLoc = GetShaderLocation(e->shader, "edgeDarken");
  e->areaFadeLoc = GetShaderLocation(e->shader, "areaFade");
  e->desatThresholdLoc = GetShaderLocation(e->shader, "desatThreshold");
  e->desatAmountLoc = GetShaderLocation(e->shader, "desatAmount");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void SubdivideEffectSetup(SubdivideEffect *e, const SubdivideConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture) {
  e->time += cfg->speed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

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
  SetShaderValue(e->shader, e->squishLoc, &cfg->squish, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thresholdLoc, &cfg->threshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxIterationsLoc, &cfg->maxIterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeDarkenLoc, &cfg->edgeDarken,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->areaFadeLoc, &cfg->areaFade,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->desatThresholdLoc, &cfg->desatThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->desatAmountLoc, &cfg->desatAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SubdivideEffectUninit(SubdivideEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void SubdivideRegisterParams(SubdivideConfig *cfg) {
  ModEngineRegisterParam("subdivide.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("subdivide.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("subdivide.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("subdivide.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("subdivide.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("subdivide.speed", &cfg->speed, 0.1f, 2.0f);
  ModEngineRegisterParam("subdivide.squish", &cfg->squish, 0.001f, 0.05f);
  ModEngineRegisterParam("subdivide.threshold", &cfg->threshold, 0.01f, 0.9f);
  ModEngineRegisterParam("subdivide.edgeDarken", &cfg->edgeDarken, 0.0f, 1.0f);
  ModEngineRegisterParam("subdivide.areaFade", &cfg->areaFade, 0.0001f, 0.005f);
  ModEngineRegisterParam("subdivide.desatThreshold", &cfg->desatThreshold, 0.0f,
                         1.0f);
  ModEngineRegisterParam("subdivide.desatAmount", &cfg->desatAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("subdivide.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

SubdivideEffect *GetSubdivideEffect(PostEffect *pe) {
  return (SubdivideEffect *)pe->effectStates[TRANSFORM_SUBDIVIDE_BLEND];
}

void SetupSubdivide(PostEffect *pe) {
  SubdivideEffectSetup(GetSubdivideEffect(pe), &pe->effects.subdivide,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupSubdivideBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.subdivide.blendIntensity,
                       pe->effects.subdivide.blendMode);
}

// === UI ===

static void DrawSubdivideParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  SubdivideConfig *cfg = &e->subdivide;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##subdivide", &cfg->baseFreq,
                    "subdivide.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##subdivide", &cfg->maxFreq,
                    "subdivide.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##subdivide", &cfg->gain, "subdivide.gain", "%.1f",
                    ms);
  ModulatableSlider("Contrast##subdivide", &cfg->curve, "subdivide.curve",
                    "%.2f", ms);
  ModulatableSlider("Base Bright##subdivide", &cfg->baseBright,
                    "subdivide.baseBright", "%.2f", ms);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Speed##subdivide", &cfg->speed, "subdivide.speed", "%.2f",
                    ms);
  ModulatableSliderLog("Squish##subdivide", &cfg->squish, "subdivide.squish",
                       "%.4f", ms);
  ModulatableSlider("Threshold##subdivide", &cfg->threshold,
                    "subdivide.threshold", "%.2f", ms);
  ImGui::SliderInt("Iterations##subdivide", &cfg->maxIterations, 2, 20);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Edge Darken##subdivide", &cfg->edgeDarken,
                    "subdivide.edgeDarken", "%.2f", ms);
  ModulatableSliderLog("Area Fade##subdivide", &cfg->areaFade,
                       "subdivide.areaFade", "%.4f", ms);
  ModulatableSlider("Desat Threshold##subdivide", &cfg->desatThreshold,
                    "subdivide.desatThreshold", "%.2f", ms);
  ModulatableSlider("Desat Amount##subdivide", &cfg->desatAmount,
                    "subdivide.desatAmount", "%.2f", ms);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(subdivide)
REGISTER_GENERATOR(TRANSFORM_SUBDIVIDE_BLEND, Subdivide, subdivide,
                   "Subdivide", SetupSubdivideBlend,
                   SetupSubdivide, 12, DrawSubdivideParams, DrawOutput_subdivide)
// clang-format on
