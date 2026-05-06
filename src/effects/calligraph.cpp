// Calligraph effect module implementation
// Two-pass ping-pong feedback ink simulation: lissajous-traced curve seeds a
// mask field that gets advected by procedural curl-noise and rendered with
// edge detection through a gradient LUT

#include "calligraph.h"
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

static void CacheStateLocations(CalligraphEffect *e) {
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

static void CacheColorLocations(CalligraphEffect *e) {
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

static void InitTextures(CalligraphEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->maskPingPong[0], width, height, "CALLIGRAPH_MASK");
  RenderUtilsInitTextureHDR(&e->maskPingPong[1], width, height, "CALLIGRAPH_MASK");
  RenderUtilsInitTextureHDR(&e->colorRT, width, height, "CALLIGRAPH_COLOR");
  RenderUtilsClearTexture(&e->maskPingPong[0]);
  RenderUtilsClearTexture(&e->maskPingPong[1]);
  RenderUtilsClearTexture(&e->colorRT);
}

static void UnloadTextures(const CalligraphEffect *e) {
  UnloadRenderTexture(e->maskPingPong[0]);
  UnloadRenderTexture(e->maskPingPong[1]);
  UnloadRenderTexture(e->colorRT);
}

bool CalligraphEffectInit(CalligraphEffect *e, const CalligraphConfig *cfg, int width,
                       int height) {
  e->stateShader = LoadShader(NULL, "shaders/calligraph_state.fs");
  if (e->stateShader.id == 0) {
    return false;
  }

  e->shader = LoadShader(NULL, "shaders/calligraph_color.fs");
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

static void BindStateUniforms(const CalligraphEffect *e, const CalligraphConfig *cfg,
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

static void BindColorUniforms(const CalligraphEffect *e, const CalligraphConfig *cfg,
                              const float *resolution) {
  SetShaderValue(e->shader, e->colorResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->colorEdgeFeatherLoc, &cfg->edgeFeather,
                 SHADER_UNIFORM_FLOAT);

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

void CalligraphEffectSetup(CalligraphEffect *e, CalligraphConfig *cfg, float deltaTime,
                        const Texture2D &fftTexture) {
  e->morphPhase += cfg->morphSpeed * deltaTime;
  cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  BindStateUniforms(e, cfg, deltaTime, resolution);
  BindColorUniforms(e, cfg, resolution);
  e->fftTexture = fftTexture;
}

void CalligraphEffectRender(CalligraphEffect *e, const CalligraphConfig *cfg,
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
  SetShaderValueTexture(e->shader, e->colorGradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->colorFftTextureLoc, e->fftTexture);
  RenderUtilsDrawFullscreenQuad(e->maskPingPong[writeIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void CalligraphEffectResize(CalligraphEffect *e, int width, int height) {
  UnloadTextures(e);
  InitTextures(e, width, height);
  e->readIdx = 0;
}

void CalligraphEffectUninit(CalligraphEffect *e) {
  UnloadShader(e->stateShader);
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadTextures(e);
}

void CalligraphRegisterParams(CalligraphConfig *cfg) {
  ModEngineRegisterParam("calligraph.morphSpeed", &cfg->morphSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("calligraph.decayRate", &cfg->decayRate, 0.05f, 5.0f);
  ModEngineRegisterParam("calligraph.spawnDistort", &cfg->spawnDistort, 0.0f,
                         0.2f);
  ModEngineRegisterParam("calligraph.advectScale", &cfg->advectScale, 0.0f, 0.02f);
  ModEngineRegisterParam("calligraph.lineThickness", &cfg->lineThickness, 0.005f,
                         0.1f);
  ModEngineRegisterParam("calligraph.edgeFeather", &cfg->edgeFeather, 0.005f,
                         0.1f);
  ModEngineRegisterParam("calligraph.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.05f, 2.0f);
  ModEngineRegisterParam("calligraph.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("calligraph.lissajous.offsetX2", &cfg->lissajous.offsetX2,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("calligraph.lissajous.offsetY2", &cfg->lissajous.offsetY2,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("calligraph.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("calligraph.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("calligraph.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("calligraph.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("calligraph.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("calligraph.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

CalligraphEffect *GetCalligraphEffect(PostEffect *pe) {
  return (CalligraphEffect *)pe->effectStates[TRANSFORM_CALLIGRAPH];
}

void SetupCalligraph(PostEffect *pe) {
  CalligraphEffect *e = GetCalligraphEffect(pe);
  CalligraphEffectSetup(e, &pe->effects.calligraph, GetFrameTime(), pe->fftTexture);
}

void SetupCalligraphBlend(PostEffect *pe) {
  CalligraphEffect *e = GetCalligraphEffect(pe);
  BlendCompositorApply(pe->blendCompositor, e->colorRT.texture,
                       pe->effects.calligraph.blendIntensity,
                       pe->effects.calligraph.blendMode);
}

void RenderCalligraph(PostEffect *pe) {
  CalligraphEffect *e = GetCalligraphEffect(pe);
  CalligraphEffectRender(e, &pe->effects.calligraph, pe->screenWidth,
                      pe->screenHeight);
}

// === UI ===

static void DrawCalligraphParams(EffectConfig *e, const ModSources *ms,
                              ImU32 glow) {
  (void)glow;
  CalligraphConfig *cfg = &e->calligraph;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##calligraph", &cfg->baseFreq,
                    "calligraph.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##calligraph", &cfg->maxFreq, "calligraph.maxFreq",
                    "%.0f", ms);
  ModulatableSlider("Gain##calligraph", &cfg->gain, "calligraph.gain", "%.1f", ms);
  ModulatableSlider("Contrast##calligraph", &cfg->curve, "calligraph.curve", "%.2f",
                    ms);
  ModulatableSlider("Base Bright##calligraph", &cfg->baseBright,
                    "calligraph.baseBright", "%.2f", ms);

  ImGui::SeparatorText("Field");
  ModulatableSlider("Morph Speed##calligraph", &cfg->morphSpeed,
                    "calligraph.morphSpeed", "%.2f", ms);
  ModulatableSlider("Decay##calligraph", &cfg->decayRate, "calligraph.decayRate",
                    "%.2f s", ms);
  ModulatableSlider("Spawn Distort##calligraph", &cfg->spawnDistort,
                    "calligraph.spawnDistort", "%.2f", ms);
  ModulatableSlider("Advect##calligraph", &cfg->advectScale, "calligraph.advectScale",
                    "%.4f", ms);

  ImGui::SeparatorText("Spawn");
  ModulatableSlider("Thickness##calligraph", &cfg->lineThickness,
                    "calligraph.lineThickness", "%.3f", ms);
  ImGui::SliderInt("Samples##calligraph", &cfg->lissajousSamples, 16, 256);
  DrawLissajousControls(&cfg->lissajous, "calligraph_liss", "calligraph.lissajous",
                        ms, 5.0f);

  ImGui::SeparatorText("Glow");
  ModulatableSlider("Edge Feather##calligraph", &cfg->edgeFeather,
                    "calligraph.edgeFeather", "%.3f", ms);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(calligraph)
REGISTER_GENERATOR_FULL(TRANSFORM_CALLIGRAPH, Calligraph, calligraph,
                        "Calligraph", SetupCalligraphBlend,
                        SetupCalligraph, RenderCalligraph, 12,
                        DrawCalligraphParams, DrawOutput_calligraph)
// clang-format on
