// Isoflow effect module implementation
// Raymarched gyroid isosurface fly-through with FFT-reactive iso-lines and
// gradient coloring

#include "isoflow.h"
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

bool IsoflowEffectInit(IsoflowEffect *e, const IsoflowConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/isoflow.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->flyPhaseLoc = GetShaderLocation(e->shader, "flyPhase");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->gyroidScaleLoc = GetShaderLocation(e->shader, "gyroidScale");
  e->surfaceThicknessLoc = GetShaderLocation(e->shader, "surfaceThickness");
  e->turbulenceScaleLoc = GetShaderLocation(e->shader, "turbulenceScale");
  e->turbulenceAmountLoc = GetShaderLocation(e->shader, "turbulenceAmount");
  e->isoFreqLoc = GetShaderLocation(e->shader, "isoFreq");
  e->isoStrengthLoc = GetShaderLocation(e->shader, "isoStrength");
  e->baseOffsetLoc = GetShaderLocation(e->shader, "baseOffset");
  e->tonemapGainLoc = GetShaderLocation(e->shader, "tonemapGain");
  e->camDriftLoc = GetShaderLocation(e->shader, "camDrift");
  e->panLoc = GetShaderLocation(e->shader, "pan");
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

  e->flyPhase = 0.0f;

  return true;
}

static void BindUniforms(const IsoflowEffect *e, const IsoflowConfig *cfg) {
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gyroidScaleLoc, &cfg->gyroidScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->surfaceThicknessLoc, &cfg->surfaceThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->turbulenceScaleLoc, &cfg->turbulenceScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->turbulenceAmountLoc, &cfg->turbulenceAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->isoFreqLoc, &cfg->isoFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->isoStrengthLoc, &cfg->isoStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseOffsetLoc, &cfg->baseOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tonemapGainLoc, &cfg->tonemapGain,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void IsoflowEffectSetup(IsoflowEffect *e, IsoflowConfig *cfg, float deltaTime,
                        const Texture2D &fftTexture) {
  e->flyPhase += cfg->flySpeed * 50.0f * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flyPhaseLoc, &e->flyPhase, SHADER_UNIFORM_FLOAT);

  BindUniforms(e, cfg);

  float panX;
  float panY;
  DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &panX, &panY);
  const float pan[2] = {panX, panY};
  SetShaderValue(e->shader, e->panLoc, pan, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->camDriftLoc, &cfg->camDrift,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void IsoflowEffectUninit(IsoflowEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void IsoflowRegisterParams(IsoflowConfig *cfg) {
  ModEngineRegisterParam("isoflow.gyroidScale", &cfg->gyroidScale, 20.0f,
                         200.0f);
  ModEngineRegisterParam("isoflow.surfaceThickness", &cfg->surfaceThickness,
                         1.0f, 64.0f);
  ModEngineRegisterParam("isoflow.turbulenceScale", &cfg->turbulenceScale, 4.0f,
                         64.0f);
  ModEngineRegisterParam("isoflow.turbulenceAmount", &cfg->turbulenceAmount,
                         0.0f, 32.0f);
  ModEngineRegisterParam("isoflow.isoFreq", &cfg->isoFreq, 0.1f, 2.0f);
  ModEngineRegisterParam("isoflow.isoStrength", &cfg->isoStrength, 0.0f, 1.0f);
  ModEngineRegisterParam("isoflow.baseOffset", &cfg->baseOffset, 50.0f, 200.0f);
  ModEngineRegisterParam("isoflow.tonemapGain", &cfg->tonemapGain, 0.0001f,
                         0.002f);
  ModEngineRegisterParam("isoflow.flySpeed", &cfg->flySpeed, 0.1f, 5.0f);
  ModEngineRegisterParam("isoflow.camDrift", &cfg->camDrift, 0.0f, 1.0f);
  ModEngineRegisterParam("isoflow.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("isoflow.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("isoflow.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("isoflow.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("isoflow.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("isoflow.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 2.0f);
  ModEngineRegisterParam("isoflow.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("isoflow.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupIsoflow(PostEffect *pe) {
  IsoflowEffectSetup(&pe->isoflow, &pe->effects.isoflow, pe->currentDeltaTime,
                     pe->fftTexture);
}

void SetupIsoflowBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.isoflow.blendIntensity,
                       pe->effects.isoflow.blendMode);
}

// === UI ===

static void DrawIsoflowParams(EffectConfig *e, const ModSources *modSources,
                              ImU32 categoryGlow) {
  (void)categoryGlow;
  IsoflowConfig *cfg = &e->isoflow;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##isoflow", &cfg->baseFreq,
                    "isoflow.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##isoflow", &cfg->maxFreq, "isoflow.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##isoflow", &cfg->gain, "isoflow.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##isoflow", &cfg->curve, "isoflow.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##isoflow", &cfg->baseBright,
                    "isoflow.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("March Steps##isoflow", &cfg->marchSteps, 32, 128);
  ModulatableSlider("Gyroid Scale##isoflow", &cfg->gyroidScale,
                    "isoflow.gyroidScale", "%.1f", modSources);
  ModulatableSlider("Thickness##isoflow", &cfg->surfaceThickness,
                    "isoflow.surfaceThickness", "%.1f", modSources);
  ModulatableSlider("Turb Scale##isoflow", &cfg->turbulenceScale,
                    "isoflow.turbulenceScale", "%.1f", modSources);
  ModulatableSlider("Turb Amount##isoflow", &cfg->turbulenceAmount,
                    "isoflow.turbulenceAmount", "%.1f", modSources);
  ModulatableSlider("Iso Freq##isoflow", &cfg->isoFreq, "isoflow.isoFreq",
                    "%.2f", modSources);
  ModulatableSlider("Iso Strength##isoflow", &cfg->isoStrength,
                    "isoflow.isoStrength", "%.2f", modSources);
  ModulatableSlider("Base Offset##isoflow", &cfg->baseOffset,
                    "isoflow.baseOffset", "%.1f", modSources);
  ModulatableSliderLog("Tonemap Gain##isoflow", &cfg->tonemapGain,
                       "isoflow.tonemapGain", "%.4f", modSources);

  // Motion
  ImGui::SeparatorText("Motion");
  ModulatableSlider("Fly Speed##isoflow", &cfg->flySpeed, "isoflow.flySpeed",
                    "%.2f", modSources);
  ModulatableSlider("Cam Drift##isoflow", &cfg->camDrift, "isoflow.camDrift",
                    "%.2f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  DrawLissajousControls(&cfg->lissajous, "iso_cam", "isoflow.lissajous",
                        modSources, 2.0f);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(isoflow)
REGISTER_GENERATOR(TRANSFORM_ISOFLOW_BLEND, Isoflow, isoflow, "Isoflow",
                   SetupIsoflowBlend, SetupIsoflow, 13, DrawIsoflowParams, DrawOutput_isoflow)
// clang-format on
