// Polygon Subdivide generator - recursive convex polygon subdivision with
// arbitrary-angle cuts, FFT-driven cell brightness, and gradient coloring

#include "polygon_subdivide.h"
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

bool PolygonSubdivideEffectInit(PolygonSubdivideEffect *e,
                                const PolygonSubdivideConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/polygon_subdivide.fs");
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

void PolygonSubdivideEffectSetup(PolygonSubdivideEffect *e,
                                 const PolygonSubdivideConfig *cfg,
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

void PolygonSubdivideEffectUninit(PolygonSubdivideEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void PolygonSubdivideRegisterParams(PolygonSubdivideConfig *cfg) {
  ModEngineRegisterParam("polygonSubdivide.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("polygonSubdivide.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("polygonSubdivide.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("polygonSubdivide.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("polygonSubdivide.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("polygonSubdivide.speed", &cfg->speed, 0.1f, 2.0f);
  ModEngineRegisterParam("polygonSubdivide.threshold", &cfg->threshold, 0.01f,
                         0.9f);
  ModEngineRegisterParam("polygonSubdivide.edgeDarken", &cfg->edgeDarken, 0.0f,
                         1.0f);
  ModEngineRegisterParam("polygonSubdivide.areaFade", &cfg->areaFade, 0.0001f,
                         0.005f);
  ModEngineRegisterParam("polygonSubdivide.desatThreshold",
                         &cfg->desatThreshold, 0.0f, 1.0f);
  ModEngineRegisterParam("polygonSubdivide.desatAmount", &cfg->desatAmount,
                         0.0f, 1.0f);
  ModEngineRegisterParam("polygonSubdivide.blendIntensity",
                         &cfg->blendIntensity, 0.0f, 5.0f);
}

void SetupPolygonSubdivide(PostEffect *pe) {
  PolygonSubdivideEffectSetup(&pe->polygonSubdivide,
                              &pe->effects.polygonSubdivide,
                              pe->currentDeltaTime, pe->fftTexture);
}

void SetupPolygonSubdivideBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.polygonSubdivide.blendIntensity,
                       pe->effects.polygonSubdivide.blendMode);
}

// === UI ===

static void DrawPolygonSubdivideParams(EffectConfig *e, const ModSources *ms,
                                       ImU32 glow) {
  (void)glow;
  PolygonSubdivideConfig *cfg = &e->polygonSubdivide;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##polygonSubdivide", &cfg->baseFreq,
                    "polygonSubdivide.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##polygonSubdivide", &cfg->maxFreq,
                    "polygonSubdivide.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##polygonSubdivide", &cfg->gain,
                    "polygonSubdivide.gain", "%.1f", ms);
  ModulatableSlider("Contrast##polygonSubdivide", &cfg->curve,
                    "polygonSubdivide.curve", "%.2f", ms);
  ModulatableSlider("Base Bright##polygonSubdivide", &cfg->baseBright,
                    "polygonSubdivide.baseBright", "%.2f", ms);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Speed##polygonSubdivide", &cfg->speed,
                    "polygonSubdivide.speed", "%.2f", ms);
  ModulatableSlider("Threshold##polygonSubdivide", &cfg->threshold,
                    "polygonSubdivide.threshold", "%.2f", ms);
  ImGui::SliderInt("Iterations##polygonSubdivide", &cfg->maxIterations, 2, 20);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Edge Darken##polygonSubdivide", &cfg->edgeDarken,
                    "polygonSubdivide.edgeDarken", "%.2f", ms);
  ModulatableSliderLog("Area Fade##polygonSubdivide", &cfg->areaFade,
                       "polygonSubdivide.areaFade", "%.4f", ms);
  ModulatableSlider("Desat Threshold##polygonSubdivide", &cfg->desatThreshold,
                    "polygonSubdivide.desatThreshold", "%.2f", ms);
  ModulatableSlider("Desat Amount##polygonSubdivide", &cfg->desatAmount,
                    "polygonSubdivide.desatAmount", "%.2f", ms);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(polygonSubdivide)
REGISTER_GENERATOR(TRANSFORM_POLYGON_SUBDIVIDE_BLEND, PolygonSubdivide, polygonSubdivide,
                   "Polygon Subdivide", SetupPolygonSubdivideBlend,
                   SetupPolygonSubdivide, 12, DrawPolygonSubdivideParams, DrawOutput_polygonSubdivide)
// clang-format on
