// Color Stretch effect module implementation
// FFT-reactive recursive grid zoom with glyph subdivision

#include "color_stretch.h"
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

bool ColorStretchEffectInit(ColorStretchEffect *e,
                            const ColorStretchConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/color_stretch.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->zoomPhaseLoc = GetShaderLocation(e->shader, "zoomPhase");
  e->zoomSpeedLoc = GetShaderLocation(e->shader, "zoomSpeed");
  e->zoomScaleLoc = GetShaderLocation(e->shader, "zoomScale");
  e->glyphSizeLoc = GetShaderLocation(e->shader, "glyphSize");
  e->recursionCountLoc = GetShaderLocation(e->shader, "recursionCount");
  e->curvatureLoc = GetShaderLocation(e->shader, "curvature");
  e->spinPhaseLoc = GetShaderLocation(e->shader, "spinPhase");
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

  e->zoomPhase = 0.0f;
  e->spinPhase = 0.0f;

  return true;
}

void ColorStretchEffectSetup(ColorStretchEffect *e,
                             const ColorStretchConfig *cfg, float deltaTime,
                             const Texture2D &fftTexture) {
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  e->zoomPhase += cfg->zoomSpeed * deltaTime;
  e->spinPhase += cfg->spinSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomPhaseLoc, &e->zoomPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomSpeedLoc, &cfg->zoomSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomScaleLoc, &cfg->zoomScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glyphSizeLoc, &cfg->glyphSize,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->recursionCountLoc, &cfg->recursionCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->curvatureLoc, &cfg->curvature,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spinPhaseLoc, &e->spinPhase,
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

void ColorStretchEffectUninit(ColorStretchEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void ColorStretchRegisterParams(ColorStretchConfig *cfg) {
  ModEngineRegisterParam("colorStretch.zoomSpeed", &cfg->zoomSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("colorStretch.zoomScale", &cfg->zoomScale, 0.01f,
                         1.0f);
  ModEngineRegisterParam("colorStretch.curvature", &cfg->curvature, 0.0f,
                         10.0f);
  ModEngineRegisterParam("colorStretch.spinSpeed", &cfg->spinSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("colorStretch.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("colorStretch.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("colorStretch.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("colorStretch.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("colorStretch.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("colorStretch.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupColorStretch(PostEffect *pe) {
  ColorStretchEffectSetup(&pe->colorStretch, &pe->effects.colorStretch,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupColorStretchBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.colorStretch.blendIntensity,
                       pe->effects.colorStretch.blendMode);
}

// === UI ===

static void DrawColorStretchParams(EffectConfig *e,
                                   const ModSources *modSources,
                                   ImU32 categoryGlow) {
  (void)categoryGlow;
  ColorStretchConfig *cfg = &e->colorStretch;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##colorStretch", &cfg->baseFreq,
                    "colorStretch.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##colorStretch", &cfg->maxFreq,
                    "colorStretch.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##colorStretch", &cfg->gain, "colorStretch.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##colorStretch", &cfg->curve, "colorStretch.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##colorStretch", &cfg->baseBright,
                    "colorStretch.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Grid Size##colorStretch", &cfg->glyphSize, 2, 8);
  ImGui::SliderInt("Recursion##colorStretch", &cfg->recursionCount, 2, 12);
  ModulatableSlider("Zoom Scale##colorStretch", &cfg->zoomScale,
                    "colorStretch.zoomScale", "%.3f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Zoom Speed##colorStretch", &cfg->zoomSpeed,
                    "colorStretch.zoomSpeed", "%.2f", modSources);
  ModulatableSlider("Curvature##colorStretch", &cfg->curvature,
                    "colorStretch.curvature", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Spin Speed##colorStretch", &cfg->spinSpeed,
                            "colorStretch.spinSpeed", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(colorStretch)
REGISTER_GENERATOR(TRANSFORM_COLOR_STRETCH_BLEND, ColorStretch, colorStretch, "Color Stretch",
                   SetupColorStretchBlend, SetupColorStretch, 12, DrawColorStretchParams, DrawOutput_colorStretch)
// clang-format on
