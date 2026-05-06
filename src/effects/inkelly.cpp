// Inkelly effect module implementation
// Two-pass ping-pong feedback ink simulation: lissajous-traced curve seeds a
// mask field that gets advected by procedural curl-noise and rendered with
// edge detection through a gradient LUT

#include "inkelly.h"
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
#include "render/render_utils.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"

#include <math.h>
#include <stddef.h>

static void CacheStateLocations(InkellyEffect *e) {
  e->stateResolutionLoc = GetShaderLocation(e->stateShader, "resolution");
  e->stateMorphPhaseLoc = GetShaderLocation(e->stateShader, "morphPhase");
  e->stateDeltaTimeLoc = GetShaderLocation(e->stateShader, "deltaTime");
  e->stateDecayRateLoc = GetShaderLocation(e->stateShader, "decayRate");
  e->stateSpawnDistortLoc = GetShaderLocation(e->stateShader, "spawnDistort");
  e->stateAdvectScaleLoc = GetShaderLocation(e->stateShader, "advectScale");
  e->stateLineThicknessLoc = GetShaderLocation(e->stateShader, "lineThickness");
  e->stateLissajousSamplesLoc =
      GetShaderLocation(e->stateShader, "lissajousSamples");
  e->stateLissAmplitudeLoc = GetShaderLocation(e->stateShader, "lissAmplitude");
  e->stateLissPhaseLoc = GetShaderLocation(e->stateShader, "lissPhase");
  e->stateLissFreqX1Loc = GetShaderLocation(e->stateShader, "lissFreqX1");
  e->stateLissFreqY1Loc = GetShaderLocation(e->stateShader, "lissFreqY1");
  e->stateLissFreqX2Loc = GetShaderLocation(e->stateShader, "lissFreqX2");
  e->stateLissFreqY2Loc = GetShaderLocation(e->stateShader, "lissFreqY2");
  e->stateLissOffsetX2Loc = GetShaderLocation(e->stateShader, "lissOffsetX2");
  e->stateLissOffsetY2Loc = GetShaderLocation(e->stateShader, "lissOffsetY2");
}

static void CacheColorLocations(InkellyEffect *e) {
  e->colorResolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->colorEdgeFeatherLoc = GetShaderLocation(e->shader, "edgeFeather");
  e->colorFftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->colorGradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->colorSampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->colorBaseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->colorMaxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->colorGainLoc = GetShaderLocation(e->shader, "gain");
  e->colorCurveLoc = GetShaderLocation(e->shader, "curve");
  e->colorBaseBrightLoc = GetShaderLocation(e->shader, "baseBright");
}

static void InitTextures(InkellyEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->maskPingPong[0], width, height, "INKELLY_MASK");
  RenderUtilsInitTextureHDR(&e->maskPingPong[1], width, height, "INKELLY_MASK");
  RenderUtilsInitTextureHDR(&e->colorRT, width, height, "INKELLY_COLOR");
  RenderUtilsClearTexture(&e->maskPingPong[0]);
  RenderUtilsClearTexture(&e->maskPingPong[1]);
  RenderUtilsClearTexture(&e->colorRT);
}

static void UnloadTextures(const InkellyEffect *e) {
  UnloadRenderTexture(e->maskPingPong[0]);
  UnloadRenderTexture(e->maskPingPong[1]);
  UnloadRenderTexture(e->colorRT);
}

bool InkellyEffectInit(InkellyEffect *e, const InkellyConfig *cfg, int width,
                       int height) {
  e->stateShader = LoadShader(NULL, "shaders/inkelly_state.fs");
  if (e->stateShader.id == 0) {
    return false;
  }

  e->shader = LoadShader(NULL, "shaders/inkelly_color.fs");
  if (e->shader.id == 0) {
    UnloadShader(e->stateShader);
    return false;
  }

  CacheStateLocations(e);
  CacheColorLocations(e);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    UnloadShader(e->stateShader);
    return false;
  }

  InitTextures(e, width, height);

  e->readIdx = 0;
  e->morphPhase = 0.0f;

  return true;
}

static void BindStateUniforms(const InkellyEffect *e, const InkellyConfig *cfg,
                              float deltaTime, const float *resolution) {
  SetShaderValue(e->stateShader, e->stateResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->stateShader, e->stateMorphPhaseLoc, &e->morphPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateDeltaTimeLoc, &deltaTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateDecayRateLoc, &cfg->decayRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateSpawnDistortLoc, &cfg->spawnDistort,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateAdvectScaleLoc, &cfg->advectScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLineThicknessLoc, &cfg->lineThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissajousSamplesLoc,
                 &cfg->lissajousSamples, SHADER_UNIFORM_INT);
  SetShaderValue(e->stateShader, e->stateLissAmplitudeLoc,
                 &cfg->lissajous.amplitude, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissPhaseLoc, &cfg->lissajous.phase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissFreqX1Loc, &cfg->lissajous.freqX1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissFreqY1Loc, &cfg->lissajous.freqY1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissFreqX2Loc, &cfg->lissajous.freqX2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissFreqY2Loc, &cfg->lissajous.freqY2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissOffsetX2Loc,
                 &cfg->lissajous.offsetX2, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLissOffsetY2Loc,
                 &cfg->lissajous.offsetY2, SHADER_UNIFORM_FLOAT);
}

static void BindColorUniforms(const InkellyEffect *e, const InkellyConfig *cfg,
                              const float *resolution,
                              const Texture2D &fftTexture) {
  SetShaderValue(e->shader, e->colorResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->colorEdgeFeatherLoc, &cfg->edgeFeather,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->colorFftTextureLoc, fftTexture);
  SetShaderValueTexture(e->shader, e->colorGradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  const float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->colorSampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorBaseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorMaxFreqLoc, &cfg->maxFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorGainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorCurveLoc, &cfg->curve,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorBaseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void InkellyEffectSetup(InkellyEffect *e, InkellyConfig *cfg, float deltaTime,
                        const Texture2D &fftTexture) {
  e->morphPhase += cfg->morphSpeed * deltaTime;
  cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  BindStateUniforms(e, cfg, deltaTime, resolution);
  BindColorUniforms(e, cfg, resolution, fftTexture);
}

void InkellyEffectRender(InkellyEffect *e, const InkellyConfig *cfg,
                         int screenWidth, int screenHeight) {
  (void)cfg;
  const int writeIdx = 1 - e->readIdx;

  BeginTextureMode(e->maskPingPong[writeIdx]);
  BeginShaderMode(e->stateShader);
  RenderUtilsDrawFullscreenQuad(e->maskPingPong[e->readIdx].texture,
                                screenWidth, screenHeight);
  EndShaderMode();
  EndTextureMode();

  BeginTextureMode(e->colorRT);
  BeginShaderMode(e->shader);
  RenderUtilsDrawFullscreenQuad(e->maskPingPong[writeIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void InkellyEffectResize(InkellyEffect *e, int width, int height) {
  UnloadTextures(e);
  InitTextures(e, width, height);
  e->readIdx = 0;
}

void InkellyEffectUninit(InkellyEffect *e) {
  UnloadShader(e->stateShader);
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadTextures(e);
}

void InkellyRegisterParams(InkellyConfig *cfg) {
  ModEngineRegisterParam("inkelly.morphSpeed", &cfg->morphSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("inkelly.decayRate", &cfg->decayRate, 0.05f, 5.0f);
  ModEngineRegisterParam("inkelly.spawnDistort", &cfg->spawnDistort, 0.0f,
                         0.2f);
  ModEngineRegisterParam("inkelly.advectScale", &cfg->advectScale, 0.0f, 0.02f);
  ModEngineRegisterParam("inkelly.lineThickness", &cfg->lineThickness, 0.005f,
                         0.1f);
  ModEngineRegisterParam("inkelly.edgeFeather", &cfg->edgeFeather, 0.005f,
                         0.1f);
  ModEngineRegisterParam("inkelly.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.05f, 2.0f);
  ModEngineRegisterParam("inkelly.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("inkelly.lissajous.offsetX2", &cfg->lissajous.offsetX2,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("inkelly.lissajous.offsetY2", &cfg->lissajous.offsetY2,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("inkelly.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("inkelly.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("inkelly.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("inkelly.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("inkelly.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("inkelly.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

InkellyEffect *GetInkellyEffect(PostEffect *pe) {
  return (InkellyEffect *)pe->effectStates[TRANSFORM_INKELLY];
}

void SetupInkelly(PostEffect *pe) {
  InkellyEffect *e = GetInkellyEffect(pe);
  InkellyEffectSetup(e, &pe->effects.inkelly, GetFrameTime(), pe->fftTexture);
}

void SetupInkellyBlend(PostEffect *pe) {
  InkellyEffect *e = GetInkellyEffect(pe);
  BlendCompositorApply(pe->blendCompositor, e->colorRT.texture,
                       pe->effects.inkelly.blendIntensity,
                       pe->effects.inkelly.blendMode);
}

void RenderInkelly(PostEffect *pe) {
  InkellyEffect *e = GetInkellyEffect(pe);
  InkellyEffectRender(e, &pe->effects.inkelly, pe->screenWidth,
                      pe->screenHeight);
}

// === UI ===

static void DrawInkellyParams(EffectConfig *e, const ModSources *ms,
                              ImU32 glow) {
  (void)glow;
  InkellyConfig *cfg = &e->inkelly;

  ImGui::SeparatorText("Field");
  ModulatableSlider("Morph Speed##inkelly", &cfg->morphSpeed,
                    "inkelly.morphSpeed", "%.2f", ms);
  ModulatableSlider("Decay##inkelly", &cfg->decayRate, "inkelly.decayRate",
                    "%.2f s", ms);
  ModulatableSlider("Spawn Distort##inkelly", &cfg->spawnDistort,
                    "inkelly.spawnDistort", "%.2f", ms);
  ModulatableSlider("Advect##inkelly", &cfg->advectScale, "inkelly.advectScale",
                    "%.4f", ms);

  ImGui::SeparatorText("Spawn");
  ModulatableSlider("Thickness##inkelly", &cfg->lineThickness,
                    "inkelly.lineThickness", "%.3f", ms);
  ImGui::SliderInt("Samples##inkelly", &cfg->lissajousSamples, 16, 256);
  DrawLissajousControls(&cfg->lissajous, "inkelly_liss", "inkelly.lissajous",
                        ms, 5.0f);

  ImGui::SeparatorText("Render");
  ModulatableSlider("Edge Feather##inkelly", &cfg->edgeFeather,
                    "inkelly.edgeFeather", "%.3f", ms);

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##inkelly", &cfg->baseFreq,
                    "inkelly.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##inkelly", &cfg->maxFreq, "inkelly.maxFreq",
                    "%.0f", ms);
  ModulatableSlider("Gain##inkelly", &cfg->gain, "inkelly.gain", "%.1f", ms);
  ModulatableSlider("Contrast##inkelly", &cfg->curve, "inkelly.curve", "%.2f",
                    ms);
  ModulatableSlider("Base Bright##inkelly", &cfg->baseBright,
                    "inkelly.baseBright", "%.2f", ms);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(inkelly)
REGISTER_GENERATOR_FULL(TRANSFORM_INKELLY, Inkelly, inkelly,
                        "Inkelly", SetupInkellyBlend,
                        SetupInkelly, RenderInkelly, 12,
                        DrawInkellyParams, DrawOutput_inkelly)
// clang-format on
