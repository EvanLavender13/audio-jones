// Dream Fractal effect module implementation
// FFT-driven Menger sponge raymarcher with carve modes, space-folding, orbital
// camera, Julia offset, and gradient output

#include "dream_fractal.h"
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

bool DreamFractalEffectInit(DreamFractalEffect *e,
                            const DreamFractalConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/dream_fractal.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->orbitPhaseLoc = GetShaderLocation(e->shader, "orbitPhase");
  e->driftPhaseLoc = GetShaderLocation(e->shader, "driftPhase");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->fractalItersLoc = GetShaderLocation(e->shader, "fractalIters");
  e->carveRadiusLoc = GetShaderLocation(e->shader, "carveRadius");
  e->carveModeLoc = GetShaderLocation(e->shader, "carveMode");
  e->foldEnabledLoc = GetShaderLocation(e->shader, "foldEnabled");
  e->foldModeLoc = GetShaderLocation(e->shader, "foldMode");
  e->juliaOffsetLoc = GetShaderLocation(e->shader, "juliaOffset");
  e->scaleFactorLoc = GetShaderLocation(e->shader, "scaleFactor");
  e->colorScaleLoc = GetShaderLocation(e->shader, "colorScale");
  e->turbulenceIntensityLoc =
      GetShaderLocation(e->shader, "turbulenceIntensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->orbitPhase = 0.0f;
  e->driftPhase = 0.0f;

  return true;
}

void DreamFractalEffectSetup(DreamFractalEffect *e,
                             const DreamFractalConfig *cfg, float deltaTime,
                             const Texture2D &fftTexture) {
  const float sampleRate = (float)AUDIO_SAMPLE_RATE;
  e->orbitPhase += cfg->orbitSpeed * deltaTime;
  e->driftPhase += cfg->driftSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitPhaseLoc, &e->orbitPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftPhaseLoc, &e->driftPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->fractalItersLoc, &cfg->fractalIters,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->carveRadiusLoc, &cfg->carveRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->carveModeLoc, &cfg->carveMode,
                 SHADER_UNIFORM_INT);
  const int foldEn = cfg->foldEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->foldEnabledLoc, &foldEn, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->foldModeLoc, &cfg->foldMode, SHADER_UNIFORM_INT);
  const float juliaOffset[3] = {cfg->juliaX, cfg->juliaY, cfg->juliaZ};
  SetShaderValue(e->shader, e->juliaOffsetLoc, juliaOffset,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(e->shader, e->scaleFactorLoc, &cfg->scaleFactor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorScaleLoc, &cfg->colorScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->turbulenceIntensityLoc,
                 &cfg->turbulenceIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void DreamFractalEffectUninit(DreamFractalEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void DreamFractalRegisterParams(DreamFractalConfig *cfg) {
  ModEngineRegisterParam("dreamFractal.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("dreamFractal.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("dreamFractal.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("dreamFractal.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("dreamFractal.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dreamFractal.orbitSpeed", &cfg->orbitSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("dreamFractal.driftSpeed", &cfg->driftSpeed, -0.5f,
                         0.5f);
  ModEngineRegisterParam("dreamFractal.carveRadius", &cfg->carveRadius, 0.3f,
                         1.5f);
  ModEngineRegisterParam("dreamFractal.juliaX", &cfg->juliaX, -1.0f, 1.0f);
  ModEngineRegisterParam("dreamFractal.juliaY", &cfg->juliaY, -1.0f, 1.0f);
  ModEngineRegisterParam("dreamFractal.juliaZ", &cfg->juliaZ, -1.0f, 1.0f);
  ModEngineRegisterParam("dreamFractal.scaleFactor", &cfg->scaleFactor, 2.0f,
                         5.0f);
  ModEngineRegisterParam("dreamFractal.colorScale", &cfg->colorScale, 1.0f,
                         30.0f);
  ModEngineRegisterParam("dreamFractal.turbulenceIntensity",
                         &cfg->turbulenceIntensity, 0.1f, 3.0f);
  ModEngineRegisterParam("dreamFractal.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupDreamFractal(PostEffect *pe) {
  DreamFractalEffectSetup(&pe->dreamFractal, &pe->effects.dreamFractal,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupDreamFractalBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.dreamFractal.blendIntensity,
                       pe->effects.dreamFractal.blendMode);
}

// === UI ===

static void DrawDreamFractalParams(EffectConfig *e,
                                   const ModSources *modSources,
                                   ImU32 categoryGlow) {
  (void)categoryGlow;
  DreamFractalConfig *d = &e->dreamFractal;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##dreamFractal", &d->baseFreq,
                    "dreamFractal.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##dreamFractal", &d->maxFreq,
                    "dreamFractal.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##dreamFractal", &d->gain, "dreamFractal.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##dreamFractal", &d->curve, "dreamFractal.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##dreamFractal", &d->baseBright,
                    "dreamFractal.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("March Steps##dreamFractal", &d->marchSteps, 30, 120);
  ImGui::SliderInt("Iterations##dreamFractal", &d->fractalIters, 3, 12);
  ImGui::Combo("Carve Mode##dreamFractal", &d->carveMode,
               "Sphere\0Box\0Cross\0Cylinder\0Octahedron\0");
  ModulatableSlider("Carve Radius##dreamFractal", &d->carveRadius,
                    "dreamFractal.carveRadius", "%.2f", modSources);
  ModulatableSlider("Scale Factor##dreamFractal", &d->scaleFactor,
                    "dreamFractal.scaleFactor", "%.2f", modSources);

  // Fold
  ImGui::SeparatorText("Fold");
  ImGui::Checkbox("Fold##dreamFractal", &d->foldEnabled);
  if (d->foldEnabled) {
    ImGui::Combo("Fold Mode##dreamFractal", &d->foldMode,
                 "Box\0Sierpinski\0Menger\0Burning Ship\0");
  }

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Orbit Speed##dreamFractal", &d->orbitSpeed,
                            "dreamFractal.orbitSpeed", modSources);
  ModulatableSlider("Drift Speed##dreamFractal", &d->driftSpeed,
                    "dreamFractal.driftSpeed", "%.3f", modSources);

  // Julia
  ImGui::SeparatorText("Julia");
  ModulatableSlider("Julia X##dreamFractal", &d->juliaX, "dreamFractal.juliaX",
                    "%.2f", modSources);
  ModulatableSlider("Julia Y##dreamFractal", &d->juliaY, "dreamFractal.juliaY",
                    "%.2f", modSources);
  ModulatableSlider("Julia Z##dreamFractal", &d->juliaZ, "dreamFractal.juliaZ",
                    "%.2f", modSources);

  // Color
  ImGui::SeparatorText("Color");
  ModulatableSlider("Color Scale##dreamFractal", &d->colorScale,
                    "dreamFractal.colorScale", "%.1f", modSources);
  ModulatableSlider("Turbulence##dreamFractal", &d->turbulenceIntensity,
                    "dreamFractal.turbulenceIntensity", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(dreamFractal)
REGISTER_GENERATOR(TRANSFORM_DREAM_FRACTAL_BLEND, DreamFractal, dreamFractal, "Dream Fractal",
                   SetupDreamFractalBlend, SetupDreamFractal, 13, DrawDreamFractalParams, DrawOutput_dreamFractal)
// clang-format on
