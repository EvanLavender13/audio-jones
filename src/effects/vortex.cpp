// Vortex effect module implementation
// Raymarched turbulent sphere with volumetric distortion and trail persistence

#include "vortex.h"
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

static void InitPingPong(VortexEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "VORTEX");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "VORTEX");
}

static void UnloadPingPong(VortexEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool VortexEffectInit(VortexEffect *e, const VortexConfig *cfg, int width,
                      int height) {
  e->shader = LoadShader(NULL, "shaders/vortex.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->turbulenceOctavesLoc = GetShaderLocation(e->shader, "turbulenceOctaves");
  e->turbulenceGrowthLoc = GetShaderLocation(e->shader, "turbulenceGrowth");
  e->sphereRadiusLoc = GetShaderLocation(e->shader, "sphereRadius");
  e->surfaceDetailLoc = GetShaderLocation(e->shader, "surfaceDetail");
  e->cameraDistanceLoc = GetShaderLocation(e->shader, "cameraDistance");
  e->rotationAngleLoc = GetShaderLocation(e->shader, "rotationAngle");
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
  e->rotationAngle = 0.0f;

  return true;
}

void VortexEffectSetup(VortexEffect *e, const VortexConfig *cfg,
                       float deltaTime, Texture2D fftTexture) {
  e->time += deltaTime;
  e->colorPhase += cfg->colorSpeed * deltaTime;
  e->rotationAngle += cfg->rotationSpeed * deltaTime;
  e->currentFFTTexture = fftTexture;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->turbulenceOctavesLoc, &cfg->turbulenceOctaves,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->turbulenceGrowthLoc, &cfg->turbulenceGrowth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereRadiusLoc, &cfg->sphereRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->surfaceDetailLoc, &cfg->surfaceDetail,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraDistanceLoc, &cfg->cameraDistance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAngleLoc, &e->rotationAngle,
                 SHADER_UNIFORM_FLOAT);
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

void VortexEffectRender(VortexEffect *e, const VortexConfig *cfg,
                        float deltaTime, int screenWidth, int screenHeight) {
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

void VortexEffectResize(VortexEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void VortexEffectUninit(VortexEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

VortexConfig VortexConfigDefault(void) { return VortexConfig{}; }

void VortexRegisterParams(VortexConfig *cfg) {
  ModEngineRegisterParam("vortex.turbulenceGrowth", &cfg->turbulenceGrowth,
                         0.5f, 0.95f);
  ModEngineRegisterParam("vortex.sphereRadius", &cfg->sphereRadius, 0.1f, 3.0f);
  ModEngineRegisterParam("vortex.surfaceDetail", &cfg->surfaceDetail, 5.0f,
                         100.0f);
  ModEngineRegisterParam("vortex.cameraDistance", &cfg->cameraDistance, 3.0f,
                         20.0f);
  ModEngineRegisterParam("vortex.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("vortex.decayHalfLife", &cfg->decayHalfLife, 0.1f,
                         10.0f);
  ModEngineRegisterParam("vortex.trailBlur", &cfg->trailBlur, 0.0f, 1.0f);
  ModEngineRegisterParam("vortex.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("vortex.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("vortex.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("vortex.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("vortex.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("vortex.colorSpeed", &cfg->colorSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("vortex.colorStretch", &cfg->colorStretch, 0.1f, 5.0f);
  ModEngineRegisterParam("vortex.brightness", &cfg->brightness, 0.1f, 5.0f);
  ModEngineRegisterParam("vortex.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupVortex(PostEffect *pe) {
  VortexEffectSetup(&pe->vortex, &pe->effects.vortex, pe->currentDeltaTime,
                    pe->fftTexture);
}

void SetupVortexBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, pe->vortex.pingPong[pe->vortex.readIdx].texture,
      pe->effects.vortex.blendIntensity, pe->effects.vortex.blendMode);
}

void RenderVortex(PostEffect *pe) {
  VortexEffectRender(&pe->vortex, &pe->effects.vortex, pe->currentDeltaTime,
                     pe->screenWidth, pe->screenHeight);
}

// === UI ===

static void DrawVortexParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  (void)categoryGlow;
  VortexConfig *v = &e->vortex;

  // Raymarching
  ImGui::SeparatorText("Raymarching");
  ImGui::SliderInt("March Steps##vortex", &v->marchSteps, 4, 200);
  ImGui::SliderInt("Octaves##vortex", &v->turbulenceOctaves, 2, 12);
  ModulatableSlider("Growth##vortex", &v->turbulenceGrowth,
                    "vortex.turbulenceGrowth", "%.2f", modSources);
  ModulatableSlider("Sphere Radius##vortex", &v->sphereRadius,
                    "vortex.sphereRadius", "%.2f", modSources);
  ModulatableSlider("Surface Detail##vortex", &v->surfaceDetail,
                    "vortex.surfaceDetail", "%.1f", modSources);
  ModulatableSlider("Camera Distance##vortex", &v->cameraDistance,
                    "vortex.cameraDistance", "%.1f", modSources);
  ModulatableSliderSpeedDeg("Rotation Speed##vortex", &v->rotationSpeed,
                            "vortex.rotationSpeed", modSources);

  // Trails
  ImGui::SeparatorText("Trails");
  ModulatableSlider("Decay Half-Life##vortex", &v->decayHalfLife,
                    "vortex.decayHalfLife", "%.1f", modSources);
  ModulatableSlider("Trail Blur##vortex", &v->trailBlur, "vortex.trailBlur",
                    "%.2f", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##vortex", &v->baseFreq, "vortex.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##vortex", &v->maxFreq, "vortex.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##vortex", &v->gain, "vortex.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##vortex", &v->curve, "vortex.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##vortex", &v->baseBright, "vortex.baseBright",
                    "%.2f", modSources);

  // Color
  ImGui::SeparatorText("Color");
  ModulatableSlider("Color Speed##vortex", &v->colorSpeed, "vortex.colorSpeed",
                    "%.2f", modSources);
  ModulatableSlider("Color Stretch##vortex", &v->colorStretch,
                    "vortex.colorStretch", "%.2f", modSources);

  // Tonemap
  ImGui::SeparatorText("Tonemap");
  ModulatableSlider("Brightness##vortex", &v->brightness, "vortex.brightness",
                    "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(vortex)
REGISTER_GENERATOR_FULL(TRANSFORM_VORTEX_BLEND, Vortex, vortex, "Vortex",
                        SetupVortexBlend, SetupVortex, RenderVortex, 11,
                        DrawVortexParams, DrawOutput_vortex)
// clang-format on
