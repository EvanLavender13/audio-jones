// Fractal tree generator
// KIFS-based fractal tree with FFT-driven branching and zoom animation

#include "fractal_tree.h"
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

bool FractalTreeEffectInit(FractalTreeEffect *e, const FractalTreeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/fractal_tree.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->thicknessLoc = GetShaderLocation(e->shader, "thickness");
  e->maxIterLoc = GetShaderLocation(e->shader, "maxIter");
  e->zoomAccumLoc = GetShaderLocation(e->shader, "zoomAccum");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->zoomAccum = 0.0f;
  e->rotationAccum = 0.0f;

  return true;
}

void FractalTreeEffectSetup(FractalTreeEffect *e, const FractalTreeConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture) {
  e->zoomAccum += cfg->zoomSpeed * deltaTime;
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thicknessLoc, &cfg->thickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxIterLoc, &cfg->maxIterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->zoomAccumLoc, &e->zoomAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void FractalTreeEffectUninit(FractalTreeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void FractalTreeRegisterParams(FractalTreeConfig *cfg) {
  ModEngineRegisterParam("fractalTree.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("fractalTree.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("fractalTree.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("fractalTree.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("fractalTree.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fractalTree.thickness", &cfg->thickness, 0.5f, 4.0f);
  ModEngineRegisterParam("fractalTree.zoomSpeed", &cfg->zoomSpeed, -3.0f, 3.0f);
  ModEngineRegisterParam("fractalTree.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("fractalTree.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupFractalTree(PostEffect *pe) {
  FractalTreeEffectSetup(&pe->fractalTree, &pe->effects.fractalTree,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupFractalTreeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.fractalTree.blendIntensity,
                       pe->effects.fractalTree.blendMode);
}

// === UI ===

static void DrawFractalTreeParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  FractalTreeConfig *cfg = &e->fractalTree;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##fractalTree", &cfg->baseFreq,
                    "fractalTree.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##fractalTree", &cfg->maxFreq,
                    "fractalTree.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##fractalTree", &cfg->gain, "fractalTree.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##fractalTree", &cfg->curve, "fractalTree.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##fractalTree", &cfg->baseBright,
                    "fractalTree.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Thickness##fractalTree", &cfg->thickness,
                    "fractalTree.thickness", "%.2f", modSources);
  ImGui::SliderInt("Max Iterations##fractalTree", &cfg->maxIterations, 8, 32);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Zoom Speed##fractalTree", &cfg->zoomSpeed,
                    "fractalTree.zoomSpeed", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rotation Speed##fractalTree", &cfg->rotationSpeed,
                            "fractalTree.rotationSpeed", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(fractalTree)
REGISTER_GENERATOR(TRANSFORM_FRACTAL_TREE_BLEND, FractalTree, fractalTree,
                   "Fractal Tree", SetupFractalTreeBlend,
                   SetupFractalTree, 10, DrawFractalTreeParams, DrawOutput_fractalTree)
// clang-format on
