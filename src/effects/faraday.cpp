// Faraday effect module implementation
// FFT-driven standing wave interference pattern (Faraday instability)

#include "faraday.h"
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

static void InitPingPong(FaradayEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "FARADAY");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "FARADAY");
}

static void UnloadPingPong(FaradayEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool FaradayEffectInit(FaradayEffect *e, const FaradayConfig *cfg, int width,
                       int height) {
  e->shader = LoadShader(NULL, "shaders/faraday.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->waveCountLoc = GetShaderLocation(e->shader, "waveCount");
  e->spatialScaleLoc = GetShaderLocation(e->shader, "spatialScale");
  e->visualGainLoc = GetShaderLocation(e->shader, "visualGain");
  e->rotationOffsetLoc = GetShaderLocation(e->shader, "rotationOffset");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->diffusionScaleLoc = GetShaderLocation(e->shader, "diffusionScale");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->waveSourceLoc = GetShaderLocation(e->shader, "waveSource");
  e->waveShapeLoc = GetShaderLocation(e->shader, "waveShape");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");

  e->colorLUT = ColorLUTInit(&cfg->gradient);
  if (e->colorLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  InitPingPong(e, width, height);
  RenderUtilsClearTexture(&e->pingPong[0]);
  RenderUtilsClearTexture(&e->pingPong[1]);
  e->readIdx = 0;
  e->time = 0.0f;
  e->rotationAccum = 0.0f;

  return true;
}

void FaradayEffectSetup(FaradayEffect *e, const FaradayConfig *cfg,
                        float deltaTime, Texture2D fftTexture) {
  e->currentFftTexture = fftTexture;
  ColorLUTUpdate(e->colorLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  if (cfg->waveSource == 0) {
    e->time += deltaTime;
  } else {
    e->time += cfg->waveSpeed * deltaTime;
  }
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->waveSourceLoc, &cfg->waveSource,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->waveShapeLoc, &cfg->waveShape,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->waveCountLoc, &cfg->waveCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->spatialScaleLoc, &cfg->spatialScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->visualGainLoc, &cfg->visualGain,
                 SHADER_UNIFORM_FLOAT);

  e->rotationAccum += cfg->rotationSpeed * deltaTime;
  float rotationOffset = cfg->rotationAngle + e->rotationAccum;
  SetShaderValue(e->shader, e->rotationOffsetLoc, &rotationOffset,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->diffusionScaleLoc, &cfg->diffusionScale,
                 SHADER_UNIFORM_INT);

  // Compute exponential decay factor from half-life
  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
}

void FaradayEffectRender(FaradayEffect *e, const FaradayConfig *cfg,
                         float deltaTime, int screenWidth, int screenHeight) {
  (void)cfg;
  (void)deltaTime;

  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFftTexture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));

  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void FaradayEffectResize(FaradayEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void FaradayEffectUninit(FaradayEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
  UnloadPingPong(e);
}

FaradayConfig FaradayConfigDefault(void) { return FaradayConfig{}; }

void FaradayRegisterParams(FaradayConfig *cfg) {
  ModEngineRegisterParam("faraday.waveFreq", &cfg->waveFreq, 5.0f, 100.0f);
  ModEngineRegisterParam("faraday.waveSpeed", &cfg->waveSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("faraday.spatialScale", &cfg->spatialScale, 0.01f,
                         1.0f);
  ModEngineRegisterParam("faraday.visualGain", &cfg->visualGain, 0.5f, 5.0f);
  ModEngineRegisterParam("faraday.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("faraday.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("faraday.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("faraday.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("faraday.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("faraday.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("faraday.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupFaraday(PostEffect *pe) {
  FaradayEffectSetup(&pe->faraday, &pe->effects.faraday, pe->currentDeltaTime,
                     pe->fftTexture);
}

void SetupFaradayBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, pe->faraday.pingPong[pe->faraday.readIdx].texture,
      pe->effects.faraday.blendIntensity, pe->effects.faraday.blendMode);
}

void RenderFaraday(PostEffect *pe) {
  FaradayEffectRender(&pe->faraday, &pe->effects.faraday, pe->currentDeltaTime,
                      pe->screenWidth, pe->screenHeight);
}

// === UI ===

static void DrawFaradayParams(EffectConfig *e, const ModSources *ms,
                              ImU32 categoryGlow) {
  (void)categoryGlow;

  ImGui::SeparatorText("Wave");
  ImGui::Combo("Wave Source##faraday", &e->faraday.waveSource,
               "Audio\0Parametric\0");
  if (e->faraday.waveSource == 1) {
    ImGui::Combo("Wave Shape##faraday", &e->faraday.waveShape,
                 "Sine\0Triangle\0Sawtooth\0Square\0");
    ModulatableSlider("Wave Freq##faraday", &e->faraday.waveFreq,
                      "faraday.waveFreq", "%.1f", ms);
    ModulatableSlider("Wave Speed##faraday", &e->faraday.waveSpeed,
                      "faraday.waveSpeed", "%.1f", ms);
  }
  ImGui::SliderInt("Wave Count##faraday", &e->faraday.waveCount, 1, 6);
  ModulatableSliderLog("Spatial Scale##faraday", &e->faraday.spatialScale,
                       "faraday.spatialScale", "%.3f", ms);
  ModulatableSlider("Vis Gain##faraday", &e->faraday.visualGain,
                    "faraday.visualGain", "%.2f", ms);
  ModulatableSliderSpeedDeg("Rotation##faraday", &e->faraday.rotationSpeed,
                            "faraday.rotationSpeed", ms);
  ModulatableSliderAngleDeg("Offset##faraday", &e->faraday.rotationAngle,
                            "faraday.rotationAngle", ms);

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Layers##faraday", &e->faraday.layers, 1, 16);

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##faraday", &e->faraday.baseFreq,
                    "faraday.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##faraday", &e->faraday.maxFreq,
                    "faraday.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##faraday", &e->faraday.gain, "faraday.gain", "%.1f",
                    ms);
  ModulatableSlider("Contrast##faraday", &e->faraday.curve, "faraday.curve",
                    "%.2f", ms);

  ImGui::SeparatorText("Trail");
  ImGui::SliderFloat("Decay##faraday", &e->faraday.decayHalfLife, 0.05f, 5.0f,
                     "%.2f s");
  ImGui::SliderInt("Diffusion##faraday", &e->faraday.diffusionScale, 0, 8);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(faraday)
REGISTER_GENERATOR_FULL(TRANSFORM_FARADAY_BLEND, Faraday, faraday, "Faraday Waves",
                        SetupFaradayBlend, SetupFaraday, RenderFaraday, 16,
                        DrawFaradayParams, DrawOutput_faraday)
// clang-format on
