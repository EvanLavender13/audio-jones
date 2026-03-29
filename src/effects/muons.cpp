// Muons effect module implementation
// Raymarched turbulent ring trails through a volumetric noise field

#include "muons.h"
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
#include "render/render_utils.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

static void InitPingPong(MuonsEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "MUONS");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "MUONS");
}

static void UnloadPingPong(const MuonsEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool MuonsEffectInit(MuonsEffect *e, const MuonsConfig *cfg, int width,
                     int height) {
  e->shader = LoadShader(NULL, "shaders/muons.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->turbulenceOctavesLoc = GetShaderLocation(e->shader, "turbulenceOctaves");
  e->turbulenceStrengthLoc = GetShaderLocation(e->shader, "turbulenceStrength");
  e->ringThicknessLoc = GetShaderLocation(e->shader, "ringThickness");
  e->cameraDistanceLoc = GetShaderLocation(e->shader, "cameraDistance");
  e->phaseLoc = GetShaderLocation(e->shader, "phase");
  e->driftLoc = GetShaderLocation(e->shader, "drift");
  e->axisFeedbackLoc = GetShaderLocation(e->shader, "axisFeedback");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->colorPhaseLoc = GetShaderLocation(e->shader, "colorPhase");
  e->colorStretchLoc = GetShaderLocation(e->shader, "colorStretch");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->previousFrameLoc = GetShaderLocation(e->shader, "previousFrame");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->trailBlurLoc = GetShaderLocation(e->shader, "trailBlur");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->modeLoc = GetShaderLocation(e->shader, "mode");
  e->turbulenceModeLoc = GetShaderLocation(e->shader, "turbulenceMode");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  InitPingPong(e, width, height);
  RenderUtilsClearTexture(&e->pingPong[0]);
  RenderUtilsClearTexture(&e->pingPong[1]);
  e->readIdx = 0;
  e->time = 0.0f;
  e->colorPhase = 0.0f;

  return true;
}

void MuonsEffectSetup(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime,
                      const Texture2D &fftTexture) {
  e->time += deltaTime;
  e->colorPhase += cfg->colorSpeed * deltaTime;
  e->currentFFTTexture = fftTexture;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->turbulenceModeLoc, &cfg->turbulenceMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->turbulenceOctavesLoc, &cfg->turbulenceOctaves,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->turbulenceStrengthLoc, &cfg->turbulenceStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ringThicknessLoc, &cfg->ringThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraDistanceLoc, &cfg->cameraDistance,
                 SHADER_UNIFORM_FLOAT);
  const float phase[3] = {cfg->phaseX, cfg->phaseY, cfg->phaseZ};
  SetShaderValue(e->shader, e->phaseLoc, phase, SHADER_UNIFORM_VEC3);
  SetShaderValue(e->shader, e->driftLoc, &cfg->drift, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->axisFeedbackLoc, &cfg->axisFeedback,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->colorPhaseLoc, &e->colorPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorStretchLoc, &cfg->colorStretch,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);

  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->trailBlurLoc, &cfg->trailBlur,
                 SHADER_UNIFORM_FLOAT);

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
}

void MuonsEffectRender(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime,
                       int screenWidth, int screenHeight) {
  (void)cfg;
  (void)deltaTime;

  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  SetShaderValueTexture(e->shader, e->previousFrameLoc,
                        e->pingPong[e->readIdx].texture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFFTTexture);

  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void MuonsEffectResize(MuonsEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void MuonsEffectUninit(MuonsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

void MuonsRegisterParams(MuonsConfig *cfg) {
  ModEngineRegisterParam("muons.turbulenceStrength", &cfg->turbulenceStrength,
                         0.0f, 2.0f);
  ModEngineRegisterParam("muons.ringThickness", &cfg->ringThickness, 0.005f,
                         0.1f);
  ModEngineRegisterParam("muons.cameraDistance", &cfg->cameraDistance, 3.0f,
                         20.0f);
  ModEngineRegisterParam("muons.phaseX", &cfg->phaseX, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("muons.phaseY", &cfg->phaseY, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("muons.phaseZ", &cfg->phaseZ, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("muons.drift", &cfg->drift, 0.0f, 0.5f);
  ModEngineRegisterParam("muons.axisFeedback", &cfg->axisFeedback, 0.0f, 2.0f);
  ModEngineRegisterParam("muons.decayHalfLife", &cfg->decayHalfLife, 0.1f,
                         10.0f);
  ModEngineRegisterParam("muons.trailBlur", &cfg->trailBlur, 0.0f, 1.0f);
  ModEngineRegisterParam("muons.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("muons.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("muons.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("muons.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("muons.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("muons.colorSpeed", &cfg->colorSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("muons.colorStretch", &cfg->colorStretch, 0.1f, 5.0f);
  ModEngineRegisterParam("muons.brightness", &cfg->brightness, 0.1f, 5.0f);
  ModEngineRegisterParam("muons.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupMuons(PostEffect *pe) {
  MuonsEffectSetup(&pe->muons, &pe->effects.muons, pe->currentDeltaTime,
                   pe->fftTexture);
}

void SetupMuonsBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, pe->muons.pingPong[pe->muons.readIdx].texture,
      pe->effects.muons.blendIntensity, pe->effects.muons.blendMode);
}

void RenderMuons(PostEffect *pe) {
  MuonsEffectRender(&pe->muons, &pe->effects.muons, pe->currentDeltaTime,
                    pe->screenWidth, pe->screenHeight);
}

// === UI ===

static void DrawMuonsParams(EffectConfig *e, const ModSources *modSources,
                            ImU32 categoryGlow) {
  (void)categoryGlow;
  MuonsConfig *m = &e->muons;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##muons", &m->baseFreq, "muons.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##muons", &m->maxFreq, "muons.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##muons", &m->gain, "muons.gain", "%.1f", modSources);
  ModulatableSlider("Contrast##muons", &m->curve, "muons.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##muons", &m->baseBright, "muons.baseBright",
                    "%.2f", modSources);

  // Raymarching
  ImGui::SeparatorText("Raymarching");
  const char *modeLabels[] = {
      "Sine Shells",      "L1 Norm",     "Axis Distance", "Dot Product",
      "Chebyshev Spread", "Cone Metric", "Triple Product"};
  ImGui::Combo("Mode##muons", &m->mode, modeLabels, IM_ARRAYSIZE(modeLabels));
  ImGui::Combo("Turbulence Mode##muons", &m->turbulenceMode,
               "Sine\0Fract Fold\0Abs-Sin\0Triangle\0Squared Sin\0"
               "Square Wave\0Quantized\0Cosine\0");
  ModulatableSliderAngleDeg("Phase X##muons", &m->phaseX, "muons.phaseX",
                            modSources);
  ModulatableSliderAngleDeg("Phase Y##muons", &m->phaseY, "muons.phaseY",
                            modSources);
  ModulatableSliderAngleDeg("Phase Z##muons", &m->phaseZ, "muons.phaseZ",
                            modSources);
  ModulatableSlider("Drift##muons", &m->drift, "muons.drift", "%.3f",
                    modSources);
  ModulatableSlider("Axis Feedback##muons", &m->axisFeedback,
                    "muons.axisFeedback", "%.2f", modSources);
  ImGui::SliderInt("March Steps##muons", &m->marchSteps, 4, 200);
  ImGui::SliderInt("Octaves##muons", &m->turbulenceOctaves, 2, 12);
  ModulatableSlider("Turbulence##muons", &m->turbulenceStrength,
                    "muons.turbulenceStrength", "%.2f", modSources);
  ModulatableSlider("Ring Thickness##muons", &m->ringThickness,
                    "muons.ringThickness", "%.3f", modSources);
  ModulatableSlider("Camera Distance##muons", &m->cameraDistance,
                    "muons.cameraDistance", "%.1f", modSources);

  // Trail
  ImGui::SeparatorText("Trail");
  ModulatableSlider("Decay Half-Life##muons", &m->decayHalfLife,
                    "muons.decayHalfLife", "%.1f", modSources);
  ModulatableSlider("Trail Blur##muons", &m->trailBlur, "muons.trailBlur",
                    "%.2f", modSources);

  // Color
  ImGui::SeparatorText("Color");
  const char *colorModeLabels[] = {"Winner Takes All", "Additive Volume"};
  ImGui::Combo("Color Mode##muons", &m->colorMode, colorModeLabels,
               IM_ARRAYSIZE(colorModeLabels));
  ModulatableSlider("Color Speed##muons", &m->colorSpeed, "muons.colorSpeed",
                    "%.2f", modSources);
  ModulatableSlider("Color Stretch##muons", &m->colorStretch,
                    "muons.colorStretch", "%.2f", modSources);

  ModulatableSlider("Brightness##muons", &m->brightness, "muons.brightness",
                    "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(muons)
REGISTER_GENERATOR_FULL(TRANSFORM_MUONS_BLEND, Muons, muons, "Muons",
                        SetupMuonsBlend, SetupMuons, RenderMuons, 11,
                        DrawMuonsParams, DrawOutput_muons)
// clang-format on
