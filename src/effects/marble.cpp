// Marble effect module implementation
// FFT-driven inversive fractal raymarcher with orbit traps and gradient output

#include "marble.h"
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

bool MarbleEffectInit(MarbleEffect *e, const MarbleConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/marble.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->orbitPhaseLoc = GetShaderLocation(e->shader, "orbitPhase");
  e->perturbPhaseLoc = GetShaderLocation(e->shader, "perturbPhase");
  e->fractalItersLoc = GetShaderLocation(e->shader, "fractalIters");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->stepSizeLoc = GetShaderLocation(e->shader, "stepSize");
  e->foldScaleLoc = GetShaderLocation(e->shader, "foldScale");
  e->foldOffsetLoc = GetShaderLocation(e->shader, "foldOffset");
  e->trapSensitivityLoc = GetShaderLocation(e->shader, "trapSensitivity");
  e->sphereRadiusLoc = GetShaderLocation(e->shader, "sphereRadius");
  e->perturbAmpLoc = GetShaderLocation(e->shader, "perturbAmp");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
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

  e->orbitPhase = 0.0f;
  e->perturbPhase = 0.0f;

  return true;
}

void MarbleEffectSetup(MarbleEffect *e, const MarbleConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture) {
  const float sampleRate = (float)AUDIO_SAMPLE_RATE;
  e->orbitPhase += cfg->orbitSpeed * deltaTime;
  e->perturbPhase += cfg->perturbSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitPhaseLoc, &e->orbitPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->perturbPhaseLoc, &e->perturbPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fractalItersLoc, &cfg->fractalIters,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->stepSizeLoc, &cfg->stepSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->foldScaleLoc, &cfg->foldScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->foldOffsetLoc, &cfg->foldOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->trapSensitivityLoc, &cfg->trapSensitivity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereRadiusLoc, &cfg->sphereRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->perturbAmpLoc, &cfg->perturbAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void MarbleEffectUninit(MarbleEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void MarbleRegisterParams(MarbleConfig *cfg) {
  ModEngineRegisterParam("marble.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("marble.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("marble.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("marble.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("marble.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("marble.stepSize", &cfg->stepSize, 0.005f, 0.1f);
  ModEngineRegisterParam("marble.foldScale", &cfg->foldScale, 0.3f, 1.2f);
  ModEngineRegisterParam("marble.foldOffset", &cfg->foldOffset, -1.5f, 0.0f);
  ModEngineRegisterParam("marble.trapSensitivity", &cfg->trapSensitivity, 5.0f,
                         40.0f);
  ModEngineRegisterParam("marble.sphereRadius", &cfg->sphereRadius, 1.0f, 3.0f);
  ModEngineRegisterParam("marble.orbitSpeed", &cfg->orbitSpeed, -2.0f, 2.0f);
  ModEngineRegisterParam("marble.perturbAmp", &cfg->perturbAmp, 0.0f, 0.5f);
  ModEngineRegisterParam("marble.perturbSpeed", &cfg->perturbSpeed, 0.01f,
                         1.0f);
  ModEngineRegisterParam("marble.zoom", &cfg->zoom, 0.5f, 3.0f);
  ModEngineRegisterParam("marble.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupMarble(PostEffect *pe) {
  MarbleEffectSetup(&pe->marble, &pe->effects.marble, pe->currentDeltaTime,
                    pe->fftTexture);
}

void SetupMarbleBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.marble.blendIntensity,
                       pe->effects.marble.blendMode);
}

// === UI ===

static void DrawMarbleParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  (void)categoryGlow;
  MarbleConfig *m = &e->marble;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##marble", &m->baseFreq, "marble.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##marble", &m->maxFreq, "marble.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##marble", &m->gain, "marble.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##marble", &m->curve, "marble.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##marble", &m->baseBright, "marble.baseBright",
                    "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##marble", &m->fractalIters, 4, 12);
  ImGui::SliderInt("March Steps##marble", &m->marchSteps, 32, 128);
  ModulatableSliderLog("Step Size##marble", &m->stepSize, "marble.stepSize",
                       "%.3f", modSources);
  ModulatableSlider("Fold Scale##marble", &m->foldScale, "marble.foldScale",
                    "%.2f", modSources);
  ModulatableSlider("Fold Offset##marble", &m->foldOffset, "marble.foldOffset",
                    "%.2f", modSources);
  ModulatableSlider("Trap Sensitivity##marble", &m->trapSensitivity,
                    "marble.trapSensitivity", "%.1f", modSources);
  ModulatableSlider("Sphere Radius##marble", &m->sphereRadius,
                    "marble.sphereRadius", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Orbit Speed##marble", &m->orbitSpeed,
                            "marble.orbitSpeed", modSources);
  ModulatableSliderSpeedDeg("Perturb Speed##marble", &m->perturbSpeed,
                            "marble.perturbSpeed", modSources);
  ModulatableSlider("Perturb Amp##marble", &m->perturbAmp, "marble.perturbAmp",
                    "%.2f", modSources);
  ModulatableSlider("Zoom##marble", &m->zoom, "marble.zoom", "%.2f",
                    modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(marble)
REGISTER_GENERATOR(TRANSFORM_MARBLE_BLEND, Marble, marble, "Marble",
                   SetupMarbleBlend, SetupMarble, 13, DrawMarbleParams, DrawOutput_marble)
// clang-format on
