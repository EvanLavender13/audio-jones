// Chladni effect module implementation
// FFT-driven resonant plate eigenmode visualization

#include "chladni.h"
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

static void InitPingPong(ChladniEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "CHLADNI");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "CHLADNI");
}

static void UnloadPingPong(const ChladniEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool ChladniEffectInit(ChladniEffect *e, const ChladniConfig *cfg, int width,
                       int height) {
  e->shader = LoadShader(NULL, "shaders/chladni.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->plateSizeLoc = GetShaderLocation(e->shader, "plateSize");
  e->coherenceLoc = GetShaderLocation(e->shader, "coherence");
  e->visualGainLoc = GetShaderLocation(e->shader, "visualGain");
  e->nodalEmphasisLoc = GetShaderLocation(e->shader, "nodalEmphasis");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->diffusionScaleLoc = GetShaderLocation(e->shader, "diffusionScale");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->plateShapeLoc = GetShaderLocation(e->shader, "plateShape");
  e->fullscreenLoc = GetShaderLocation(e->shader, "fullscreen");

  e->colorLUT = ColorLUTInit(&cfg->gradient);
  if (e->colorLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  InitPingPong(e, width, height);
  RenderUtilsClearTexture(&e->pingPong[0]);
  RenderUtilsClearTexture(&e->pingPong[1]);
  e->readIdx = 0;

  return true;
}

void ChladniEffectSetup(ChladniEffect *e, const ChladniConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture) {
  e->currentFftTexture = fftTexture;
  ColorLUTUpdate(e->colorLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->plateSizeLoc, &cfg->plateSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->coherenceLoc, &cfg->coherence,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->visualGainLoc, &cfg->visualGain,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->nodalEmphasisLoc, &cfg->nodalEmphasis,
                 SHADER_UNIFORM_FLOAT);

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

  SetShaderValue(e->shader, e->plateShapeLoc, &cfg->plateShape,
                 SHADER_UNIFORM_INT);
  int fs = cfg->fullscreen ? 1 : 0;
  SetShaderValue(e->shader, e->fullscreenLoc, &fs, SHADER_UNIFORM_INT);

  // Compute exponential decay factor from half-life
  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
}

void ChladniEffectRender(ChladniEffect *e, const ChladniConfig *cfg,
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

void ChladniEffectResize(ChladniEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void ChladniEffectUninit(ChladniEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
  UnloadPingPong(e);
}

void ChladniRegisterParams(ChladniConfig *cfg) {
  ModEngineRegisterParam("chladni.plateSize", &cfg->plateSize, 0.5f, 3.0f);
  ModEngineRegisterParam("chladni.coherence", &cfg->coherence, 0.0f, 1.0f);
  ModEngineRegisterParam("chladni.visualGain", &cfg->visualGain, 0.5f, 5.0f);
  ModEngineRegisterParam("chladni.nodalEmphasis", &cfg->nodalEmphasis, 0.0f,
                         1.0f);
  ModEngineRegisterParam("chladni.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("chladni.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("chladni.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("chladni.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("chladni.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupChladni(PostEffect *pe) {
  ChladniEffectSetup(&pe->chladni, &pe->effects.chladni, pe->currentDeltaTime,
                     pe->fftTexture);
}

void SetupChladniBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, pe->chladni.pingPong[pe->chladni.readIdx].texture,
      pe->effects.chladni.blendIntensity, pe->effects.chladni.blendMode);
}

void RenderChladni(PostEffect *pe) {
  ChladniEffectRender(&pe->chladni, &pe->effects.chladni, pe->currentDeltaTime,
                      pe->screenWidth, pe->screenHeight);
}

// === UI ===

static void DrawChladniParams(EffectConfig *e, const ModSources *ms,
                              ImU32 categoryGlow) {
  (void)categoryGlow;

  ImGui::SeparatorText("Wave");
  ImGui::Combo("Plate Shape##chladni", &e->chladni.plateShape,
               "Rectangular\0Circular\0");
  ImGui::Checkbox("Fullscreen##chladni", &e->chladni.fullscreen);
  ModulatableSlider("Plate Size##chladni", &e->chladni.plateSize,
                    "chladni.plateSize", "%.2f", ms);
  ModulatableSlider("Coherence##chladni", &e->chladni.coherence,
                    "chladni.coherence", "%.2f", ms);
  ModulatableSlider("Vis Gain##chladni", &e->chladni.visualGain,
                    "chladni.visualGain", "%.2f", ms);
  ModulatableSlider("Nodal Emphasis##chladni", &e->chladni.nodalEmphasis,
                    "chladni.nodalEmphasis", "%.2f", ms);

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##chladni", &e->chladni.baseFreq,
                    "chladni.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##chladni", &e->chladni.maxFreq,
                    "chladni.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##chladni", &e->chladni.gain, "chladni.gain", "%.1f",
                    ms);
  ModulatableSlider("Contrast##chladni", &e->chladni.curve, "chladni.curve",
                    "%.2f", ms);

  ImGui::SeparatorText("Trail");
  ImGui::SliderFloat("Decay##chladni", &e->chladni.decayHalfLife, 0.05f, 5.0f,
                     "%.2f s");
  ImGui::SliderInt("Diffusion##chladni", &e->chladni.diffusionScale, 0, 8);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(chladni)
REGISTER_GENERATOR_FULL(TRANSFORM_CHLADNI_BLEND, Chladni, chladni, "Chladni",
                        SetupChladniBlend, SetupChladni, RenderChladni, 16,
                        DrawChladniParams, DrawOutput_chladni)
// clang-format on
