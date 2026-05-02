// Dream Zoom effect module implementation
// Continuous zoom through Dream/Jacobi escape-time fractal with orbit-trap
// coloring

#include "dream_zoom.h"

#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/dual_lissajous_config.h"
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
#include <math.h>
#include <stddef.h>

static const char *VARIANT_NAMES[] = {"Dream", "Jacobi"};

bool DreamZoomEffectInit(DreamZoomEffect *e, const DreamZoomConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/dream_zoom.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->variantLoc = GetShaderLocation(e->shader, "variant");
  e->zoomPhaseLoc = GetShaderLocation(e->shader, "zoomPhase");
  e->globalRotationPhaseLoc =
      GetShaderLocation(e->shader, "globalRotationPhase");
  e->rotationPhaseLoc = GetShaderLocation(e->shader, "rotationPhase");
  e->jacobiRepeatsLoc = GetShaderLocation(e->shader, "jacobiRepeats");
  e->formulaMixLoc = GetShaderLocation(e->shader, "formulaMix");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->cmapScaleLoc = GetShaderLocation(e->shader, "cmapScale");
  e->cmapOffsetLoc = GetShaderLocation(e->shader, "cmapOffset");
  e->trapOffsetLoc = GetShaderLocation(e->shader, "trapOffset");
  e->originLoc = GetShaderLocation(e->shader, "origin");
  e->constantOffsetLoc = GetShaderLocation(e->shader, "constantOffset");

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
  e->globalRotationPhase = 0.0f;
  e->rotationPhase = 0.0f;

  return true;
}

void DreamZoomEffectSetup(DreamZoomEffect *e, DreamZoomConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture) {
  e->zoomPhase += cfg->zoomSpeed * deltaTime;
  // Keep float precision bounded; the shader does its own mod(zoomPhase, 1.0).
  if (e->zoomPhase > 1024.0f) {
    e->zoomPhase -= 1024.0f;
  }
  if (e->zoomPhase < -1024.0f) {
    e->zoomPhase += 1024.0f;
  }

  e->globalRotationPhase += cfg->globalRotationSpeed * deltaTime;
  e->globalRotationPhase =
      fmodf(e->globalRotationPhase + PI_F, 2.0f * PI_F) - PI_F;

  e->rotationPhase += cfg->rotationSpeed * deltaTime;
  e->rotationPhase = fmodf(e->rotationPhase + PI_F, 2.0f * PI_F) - PI_F;

  float trapX = 0.0f;
  float trapY = 0.0f;
  float originX = 0.0f;
  float originY = 0.0f;
  float kX = 0.0f;
  float kY = 0.0f;
  DualLissajousUpdate(&cfg->trapOffset, deltaTime, 0.0f, &trapX, &trapY);
  DualLissajousUpdate(&cfg->origin, deltaTime, 0.0f, &originX, &originY);
  DualLissajousUpdate(&cfg->constantOffset, deltaTime, 0.0f, &kX, &kY);
  const float trapVec[2] = {trapX, trapY};
  const float originVec[2] = {originX, originY};
  const float kVec[2] = {kX, kY};

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  SetShaderValue(e->shader, e->variantLoc, &cfg->variant, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->zoomPhaseLoc, &e->zoomPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->globalRotationPhaseLoc, &e->globalRotationPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationPhaseLoc, &e->rotationPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->jacobiRepeatsLoc, &cfg->jacobiRepeats,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->formulaMixLoc, &cfg->formulaMix,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->cmapScaleLoc, &cfg->cmapScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cmapOffsetLoc, &cfg->cmapOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->trapOffsetLoc, trapVec, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->originLoc, originVec, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->constantOffsetLoc, kVec, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void DreamZoomEffectUninit(DreamZoomEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void DreamZoomRegisterParams(DreamZoomConfig *cfg) {
  ModEngineRegisterParam("dreamZoom.zoomSpeed", &cfg->zoomSpeed, -1.0f, 1.0f);
  ModEngineRegisterParam("dreamZoom.globalRotationSpeed",
                         &cfg->globalRotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("dreamZoom.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("dreamZoom.jacobiRepeats", &cfg->jacobiRepeats, 0.5f,
                         4.0f);
  ModEngineRegisterParam("dreamZoom.formulaMix", &cfg->formulaMix, 0.0f, 1.0f);
  ModEngineRegisterParam("dreamZoom.cmapScale", &cfg->cmapScale, 1.0f, 200.0f);
  ModEngineRegisterParam("dreamZoom.cmapOffset", &cfg->cmapOffset, 0.0f, 10.0f);
  ModEngineRegisterParam("dreamZoom.trapOffset.amplitude",
                         &cfg->trapOffset.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("dreamZoom.trapOffset.motionSpeed",
                         &cfg->trapOffset.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("dreamZoom.origin.amplitude", &cfg->origin.amplitude,
                         0.0f, 0.5f);
  ModEngineRegisterParam("dreamZoom.origin.motionSpeed",
                         &cfg->origin.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("dreamZoom.constantOffset.amplitude",
                         &cfg->constantOffset.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("dreamZoom.constantOffset.motionSpeed",
                         &cfg->constantOffset.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("dreamZoom.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("dreamZoom.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("dreamZoom.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("dreamZoom.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("dreamZoom.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("dreamZoom.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

DreamZoomEffect *GetDreamZoomEffect(PostEffect *pe) {
  return (DreamZoomEffect *)pe->effectStates[TRANSFORM_DREAM_ZOOM_BLEND];
}

void SetupDreamZoom(PostEffect *pe) {
  DreamZoomEffectSetup(GetDreamZoomEffect(pe), &pe->effects.dreamZoom,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupDreamZoomBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.dreamZoom.blendIntensity,
                       pe->effects.dreamZoom.blendMode);
}

// === UI ===

static void DrawDreamZoomParams(EffectConfig *e, const ModSources *ms,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  DreamZoomConfig *cfg = &e->dreamZoom;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##dreamZoom", &cfg->baseFreq,
                    "dreamZoom.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##dreamZoom", &cfg->maxFreq,
                    "dreamZoom.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##dreamZoom", &cfg->gain, "dreamZoom.gain", "%.1f",
                    ms);
  ModulatableSlider("Contrast##dreamZoom", &cfg->curve, "dreamZoom.curve",
                    "%.2f", ms);
  ModulatableSlider("Base Bright##dreamZoom", &cfg->baseBright,
                    "dreamZoom.baseBright", "%.2f", ms);

  ImGui::SeparatorText("Variant");
  ImGui::Combo("Variant##dreamZoom", &cfg->variant, VARIANT_NAMES, 2);

  ImGui::SeparatorText("Iteration");
  ImGui::SliderInt("Iterations##dreamZoom", &cfg->iterations, 16, 256);
  ModulatableSlider("Formula Mix##dreamZoom", &cfg->formulaMix,
                    "dreamZoom.formulaMix", "%.2f", ms);

  ImGui::SeparatorText("Tiling");
  ModulatableSlider("Jacobi Repeats##dreamZoom", &cfg->jacobiRepeats,
                    "dreamZoom.jacobiRepeats", "%.2f", ms);

  ImGui::SeparatorText("Coloring");
  ModulatableSlider("Cmap Scale##dreamZoom", &cfg->cmapScale,
                    "dreamZoom.cmapScale", "%.1f", ms);
  ModulatableSlider("Cmap Offset##dreamZoom", &cfg->cmapOffset,
                    "dreamZoom.cmapOffset", "%.2f", ms);

  ImGui::SeparatorText("Animation");
  ModulatableSlider("Zoom Speed##dreamZoom", &cfg->zoomSpeed,
                    "dreamZoom.zoomSpeed", "%.2f", ms);
  ModulatableSliderSpeedDeg("Global Rotation##dreamZoom",
                            &cfg->globalRotationSpeed,
                            "dreamZoom.globalRotationSpeed", ms);
  ModulatableSliderSpeedDeg("Inner Rotation##dreamZoom", &cfg->rotationSpeed,
                            "dreamZoom.rotationSpeed", ms);

  ImGui::SeparatorText("Trap Offset");
  DrawLissajousControls(&cfg->trapOffset, "dreamZoom_trapOffset",
                        "dreamZoom.trapOffset", ms, 5.0f);

  ImGui::SeparatorText("Origin");
  DrawLissajousControls(&cfg->origin, "dreamZoom_origin", "dreamZoom.origin",
                        ms, 5.0f);

  ImGui::SeparatorText("Constant Offset");
  DrawLissajousControls(&cfg->constantOffset, "dreamZoom_constantOffset",
                        "dreamZoom.constantOffset", ms, 5.0f);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(dreamZoom)
REGISTER_GENERATOR(TRANSFORM_DREAM_ZOOM_BLEND, DreamZoom, dreamZoom, "Dream Zoom",
                   SetupDreamZoomBlend, SetupDreamZoom, 12,
                   DrawDreamZoomParams, DrawOutput_dreamZoom)
// clang-format on
