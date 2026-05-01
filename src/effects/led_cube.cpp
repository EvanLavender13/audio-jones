// LED Cube effect module implementation
// Rotating 3D lattice of FFT-reactive LED points with tracer-driven highlights

#include "led_cube.h"
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

bool LedCubeEffectInit(LedCubeEffect *e, const LedCubeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/led_cube.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->gridSizeLoc = GetShaderLocation(e->shader, "gridSize");
  e->tracerPhaseLoc = GetShaderLocation(e->shader, "tracerPhase");
  e->rotPhaseALoc = GetShaderLocation(e->shader, "rotPhaseA");
  e->rotPhaseBLoc = GetShaderLocation(e->shader, "rotPhaseB");
  e->driftPhaseLoc = GetShaderLocation(e->shader, "driftPhase");
  e->glowFalloffLoc = GetShaderLocation(e->shader, "glowFalloff");
  e->displacementLoc = GetShaderLocation(e->shader, "displacement");
  e->cubeScaleLoc = GetShaderLocation(e->shader, "cubeScale");
  e->fovDivLoc = GetShaderLocation(e->shader, "fovDiv");
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

  e->tracerPhase = 0.0f;
  e->rotPhaseA = 0.0f;
  e->rotPhaseB = 0.0f;
  e->driftPhase = 0.0f;

  return true;
}

void LedCubeEffectSetup(LedCubeEffect *e, const LedCubeConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture) {
  e->tracerPhase += cfg->tracerSpeed * deltaTime;
  e->rotPhaseA += cfg->rotSpeedA * deltaTime;
  e->rotPhaseB += cfg->rotSpeedB * deltaTime;
  e->driftPhase += cfg->driftSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->gridSizeLoc, &cfg->gridSize, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->tracerPhaseLoc, &e->tracerPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotPhaseALoc, &e->rotPhaseA,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotPhaseBLoc, &e->rotPhaseB,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftPhaseLoc, &e->driftPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowFalloffLoc, &cfg->glowFalloff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->displacementLoc, &cfg->displacement,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cubeScaleLoc, &cfg->cubeScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fovDivLoc, &cfg->fovDiv, SHADER_UNIFORM_FLOAT);
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

void LedCubeEffectUninit(LedCubeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void LedCubeRegisterParams(LedCubeConfig *cfg) {
  ModEngineRegisterParam("ledCube.glowFalloff", &cfg->glowFalloff, 0.01f, 1.0f);
  ModEngineRegisterParam("ledCube.displacement", &cfg->displacement, 0.0f,
                         1.0f);
  ModEngineRegisterParam("ledCube.cubeScale", &cfg->cubeScale, 2.0f, 10.0f);
  ModEngineRegisterParam("ledCube.fovDiv", &cfg->fovDiv, 1.0f, 4.0f);
  ModEngineRegisterParam("ledCube.tracerSpeed", &cfg->tracerSpeed, 0.1f, 4.0f);
  ModEngineRegisterParam("ledCube.rotSpeedA", &cfg->rotSpeedA,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("ledCube.rotSpeedB", &cfg->rotSpeedB,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("ledCube.driftSpeed", &cfg->driftSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("ledCube.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("ledCube.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("ledCube.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("ledCube.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("ledCube.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("ledCube.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

LedCubeEffect *GetLedCubeEffect(PostEffect *pe) {
  return (LedCubeEffect *)pe->effectStates[TRANSFORM_LED_CUBE_BLEND];
}

void SetupLedCube(PostEffect *pe) {
  LedCubeEffectSetup(GetLedCubeEffect(pe), &pe->effects.ledCube,
                     pe->currentDeltaTime, pe->fftTexture);
}

void SetupLedCubeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.ledCube.blendIntensity,
                       pe->effects.ledCube.blendMode);
}

// === UI ===

static void DrawLedCubeParams(EffectConfig *e, const ModSources *modSources,
                              ImU32 categoryGlow) {
  (void)categoryGlow;
  LedCubeConfig *lc = &e->ledCube;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##ledCube", &lc->baseFreq,
                    "ledCube.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##ledCube", &lc->maxFreq, "ledCube.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##ledCube", &lc->gain, "ledCube.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##ledCube", &lc->curve, "ledCube.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##ledCube", &lc->baseBright,
                    "ledCube.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Grid Size##ledCube", &lc->gridSize, 3, 10);
  ModulatableSlider("Cube Scale##ledCube", &lc->cubeScale, "ledCube.cubeScale",
                    "%.2f", modSources);
  ModulatableSlider("FOV Divisor##ledCube", &lc->fovDiv, "ledCube.fovDiv",
                    "%.2f", modSources);
  ModulatableSliderLog("Glow Falloff##ledCube", &lc->glowFalloff,
                       "ledCube.glowFalloff", "%.3f", modSources);
  ModulatableSlider("Displacement##ledCube", &lc->displacement,
                    "ledCube.displacement", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Tracer Speed##ledCube", &lc->tracerSpeed,
                    "ledCube.tracerSpeed", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rot Speed A##ledCube", &lc->rotSpeedA,
                            "ledCube.rotSpeedA", modSources);
  ModulatableSliderSpeedDeg("Rot Speed B##ledCube", &lc->rotSpeedB,
                            "ledCube.rotSpeedB", modSources);
  ModulatableSlider("Drift Speed##ledCube", &lc->driftSpeed,
                    "ledCube.driftSpeed", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(ledCube)
REGISTER_GENERATOR(TRANSFORM_LED_CUBE_BLEND, LedCube, ledCube, "LED Cube",
                   SetupLedCubeBlend, SetupLedCube, 13,
                   DrawLedCubeParams, DrawOutput_ledCube)
// clang-format on
