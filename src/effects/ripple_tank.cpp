// Ripple Tank effect module implementation
// Audio-reactive wave interference from virtual point sources

#include "ripple_tank.h"
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

static void InitPingPong(RippleTankEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "RIPPLE_TANK");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "RIPPLE_TANK");
}

static void UnloadPingPong(RippleTankEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool RippleTankEffectInit(RippleTankEffect *e, const RippleTankConfig *cfg,
                          int width, int height) {
  e->shader = LoadShader(NULL, "shaders/ripple_tank.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->aspectLoc = GetShaderLocation(e->shader, "aspect");
  e->waveScaleLoc = GetShaderLocation(e->shader, "waveScale");
  e->falloffStrengthLoc = GetShaderLocation(e->shader, "falloffStrength");
  e->visualGainLoc = GetShaderLocation(e->shader, "visualGain");
  e->contourCountLoc = GetShaderLocation(e->shader, "contourCount");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->waveSourceLoc = GetShaderLocation(e->shader, "waveSource");
  e->waveShapeLoc = GetShaderLocation(e->shader, "waveShape");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->falloffTypeLoc = GetShaderLocation(e->shader, "falloffType");
  e->visualModeLoc = GetShaderLocation(e->shader, "visualMode");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->chromaSpreadLoc = GetShaderLocation(e->shader, "chromaSpread");
  e->phasesLoc = GetShaderLocation(e->shader, "phases");
  e->bufferSizeLoc = GetShaderLocation(e->shader, "bufferSize");
  e->writeIndexLoc = GetShaderLocation(e->shader, "writeIndex");
  e->valueLoc = GetShaderLocation(e->shader, "value");
  e->sourcesLoc = GetShaderLocation(e->shader, "sources");
  e->sourceCountLoc = GetShaderLocation(e->shader, "sourceCount");
  e->boundariesLoc = GetShaderLocation(e->shader, "boundaries");
  e->reflectionGainLoc = GetShaderLocation(e->shader, "reflectionGain");
  e->waveformTextureLoc = GetShaderLocation(e->shader, "waveformTexture");
  e->diffusionScaleLoc = GetShaderLocation(e->shader, "diffusionScale");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->colorLUTLoc = GetShaderLocation(e->shader, "colorLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->spatialScaleLoc = GetShaderLocation(e->shader, "spatialScale");

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

  return true;
}

static void BindTimeAndPhases(RippleTankEffect *e, const RippleTankConfig *cfg,
                              float deltaTime) {
  e->time += cfg->waveSpeed * deltaTime;
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  const int count = cfg->sourceCount < 1   ? 1
                    : cfg->sourceCount > 8 ? 8
                                           : cfg->sourceCount;
  float phases[8];
  for (int i = 0; i < count; i++) {
    phases[i] = (float)i / (float)count * TWO_PI_F;
  }
  SetShaderValueV(e->shader, e->phasesLoc, phases, SHADER_UNIFORM_FLOAT, count);
}

static void BindWaveSource(RippleTankEffect *e, const RippleTankConfig *cfg,
                           float deltaTime, Texture2D waveformTexture,
                           int waveformWriteIndex, Texture2D fftTexture) {
  SetShaderValue(e->shader, e->waveSourceLoc, &cfg->waveSource,
                 SHADER_UNIFORM_INT);
  // Always bind waveFreq (chromatic mode uses it in both audio and sine paths)
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);

  if (cfg->waveSource == 0) {
    SetShaderValue(e->shader, e->waveScaleLoc, &cfg->waveScale,
                   SHADER_UNIFORM_FLOAT);
    int bufferSize = waveformTexture.width;
    SetShaderValue(e->shader, e->bufferSizeLoc, &bufferSize,
                   SHADER_UNIFORM_INT);
    SetShaderValue(e->shader, e->writeIndexLoc, &waveformWriteIndex,
                   SHADER_UNIFORM_INT);
  } else if (cfg->waveSource == 1) {
    BindTimeAndPhases(e, cfg, deltaTime);
    SetShaderValue(e->shader, e->waveShapeLoc, &cfg->waveShape,
                   SHADER_UNIFORM_INT);
  } else {
    BindTimeAndPhases(e, cfg, deltaTime);
    float sr = (float)AUDIO_SAMPLE_RATE;
    SetShaderValue(e->shader, e->sampleRateLoc, &sr, SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
    SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->spatialScaleLoc, &cfg->spatialScale,
                   SHADER_UNIFORM_FLOAT);
    e->currentFftTexture = fftTexture;
  }
}

static float ComputeBrightness(const RippleTankConfig *cfg) {
  if (cfg->waveSource >= 1)
    return 1.0f;
  if (cfg->gradient.mode == COLOR_MODE_SOLID) {
    float h, s, v;
    ColorConfigRGBToHSV(cfg->gradient.solid, &h, &s, &v);
    return v;
  }
  if (cfg->gradient.mode == COLOR_MODE_GRADIENT)
    return 1.0f;
  if (cfg->gradient.mode == COLOR_MODE_PALETTE) {
    float s, v;
    ColorConfigGetSV(&cfg->gradient, &s, &v);
    return v;
  }
  return cfg->gradient.rainbowVal;
}

void RippleTankEffectSetup(RippleTankEffect *e, RippleTankConfig *cfg,
                           float deltaTime, Texture2D waveformTexture,
                           int waveformWriteIndex, Texture2D fftTexture) {
  e->currentWaveformTexture = waveformTexture;
  ColorLUTUpdate(e->colorLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  float aspect = resolution[0] / resolution[1];
  SetShaderValue(e->shader, e->aspectLoc, &aspect, SHADER_UNIFORM_FLOAT);

  BindWaveSource(e, cfg, deltaTime, waveformTexture, waveformWriteIndex,
                 fftTexture);

  SetShaderValue(e->shader, e->falloffTypeLoc, &cfg->falloffType,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffStrengthLoc, &cfg->falloffStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->visualModeLoc, &cfg->visualMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->contourCountLoc, &cfg->contourCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->visualGainLoc, &cfg->visualGain,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->chromaSpreadLoc, &cfg->chromaSpread,
                 SHADER_UNIFORM_FLOAT);

  // Source positions
  float sources[16]; // 8 sources * 2 components
  const int count =
      cfg->sourceCount < 1 ? 1 : (cfg->sourceCount > 8 ? 8 : cfg->sourceCount);
  DualLissajousUpdateCircular(&cfg->lissajous, deltaTime, cfg->baseRadius, 0.0f,
                              0.0f, count, sources);
  SetShaderValueV(e->shader, e->sourcesLoc, sources, SHADER_UNIFORM_VEC2,
                  count);
  SetShaderValue(e->shader, e->sourceCountLoc, &count, SHADER_UNIFORM_INT);

  int boundariesInt = cfg->boundaries ? 1 : 0;
  SetShaderValue(e->shader, e->boundariesLoc, &boundariesInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->reflectionGainLoc, &cfg->reflectionGain,
                 SHADER_UNIFORM_FLOAT);

  float value = ComputeBrightness(cfg);
  SetShaderValue(e->shader, e->valueLoc, &value, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->diffusionScaleLoc, &cfg->diffusionScale,
                 SHADER_UNIFORM_INT);

  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
}

void RippleTankEffectRender(RippleTankEffect *e, const RippleTankConfig *cfg,
                            float deltaTime, int screenWidth,
                            int screenHeight) {
  (void)deltaTime;

  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  SetShaderValueTexture(e->shader, e->waveformTextureLoc,
                        e->currentWaveformTexture);
  SetShaderValueTexture(e->shader, e->colorLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));
  if (cfg->waveSource == 2) {
    SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFftTexture);
  }

  // Ping-pong read buffer drawn as fullscreen quad becomes texture0
  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void RippleTankEffectResize(RippleTankEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void RippleTankEffectUninit(RippleTankEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
  UnloadPingPong(e);
}

RippleTankConfig RippleTankConfigDefault(void) { return RippleTankConfig{}; }

void RippleTankRegisterParams(RippleTankConfig *cfg) {
  ModEngineRegisterParam("rippleTank.waveScale", &cfg->waveScale, 1.0f, 50.0f);
  ModEngineRegisterParam("rippleTank.waveFreq", &cfg->waveFreq, 5.0f, 100.0f);
  ModEngineRegisterParam("rippleTank.waveSpeed", &cfg->waveSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("rippleTank.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("rippleTank.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("rippleTank.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("rippleTank.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("rippleTank.spatialScale", &cfg->spatialScale, 0.001f,
                         0.1f);
  ModEngineRegisterParam("rippleTank.falloffStrength", &cfg->falloffStrength,
                         0.0f, 5.0f);
  ModEngineRegisterParam("rippleTank.visualGain", &cfg->visualGain, 0.5f, 5.0f);
  ModEngineRegisterParam("rippleTank.chromaSpread", &cfg->chromaSpread, 0.0f,
                         0.1f);
  ModEngineRegisterParam("rippleTank.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
  ModEngineRegisterParam("rippleTank.baseRadius", &cfg->baseRadius, 0.0f, 1.0f);
  ModEngineRegisterParam("rippleTank.reflectionGain", &cfg->reflectionGain,
                         0.0f, 1.0f);
  ModEngineRegisterParam("rippleTank.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("rippleTank.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
}

void SetupRippleTank(PostEffect *pe) {
  RippleTankEffectSetup(&pe->rippleTank, &pe->effects.rippleTank,
                        pe->currentDeltaTime, pe->waveformTexture,
                        pe->waveformWriteIndex, pe->fftTexture);
}

void SetupRippleTankBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor,
                       pe->rippleTank.pingPong[pe->rippleTank.readIdx].texture,
                       pe->effects.rippleTank.blendIntensity,
                       pe->effects.rippleTank.blendMode);
}

void RenderRippleTank(PostEffect *pe) {
  RippleTankEffectRender(&pe->rippleTank, &pe->effects.rippleTank,
                         pe->currentDeltaTime, pe->screenWidth,
                         pe->screenHeight);
}

// === UI ===

static void DrawRippleTankParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;

  ImGui::SeparatorText("Wave");
  ImGui::Combo("Wave Source##rt", &e->rippleTank.waveSource,
               "Audio\0Parametric\0Spectral\0");
  if (e->rippleTank.waveSource == 0) {
    ModulatableSlider("Wave Scale##rt", &e->rippleTank.waveScale,
                      "rippleTank.waveScale", "%.1f", ms);
  } else if (e->rippleTank.waveSource == 1) {
    ImGui::Combo("Wave Shape##rt", &e->rippleTank.waveShape,
                 "Sine\0Triangle\0Sawtooth\0Square\0");
    ModulatableSlider("Wave Freq##rt", &e->rippleTank.waveFreq,
                      "rippleTank.waveFreq", "%.1f", ms);
    ModulatableSlider("Wave Speed##rt", &e->rippleTank.waveSpeed,
                      "rippleTank.waveSpeed", "%.1f", ms);
  } else {
    ImGui::SliderInt("Layers##rt", &e->rippleTank.layers, 1, 16);
    ModulatableSlider("Wave Speed##rt", &e->rippleTank.waveSpeed,
                      "rippleTank.waveSpeed", "%.1f", ms);
    ModulatableSliderLog("Spatial Scale##rt", &e->rippleTank.spatialScale,
                         "rippleTank.spatialScale", "%.4f", ms);
    ImGui::SeparatorText("Audio");
    ModulatableSlider("Base Freq (Hz)##rt", &e->rippleTank.baseFreq,
                      "rippleTank.baseFreq", "%.1f", ms);
    ModulatableSlider("Max Freq (Hz)##rt", &e->rippleTank.maxFreq,
                      "rippleTank.maxFreq", "%.0f", ms);
    ModulatableSlider("Gain##rt_spec", &e->rippleTank.gain, "rippleTank.gain",
                      "%.1f", ms);
    ModulatableSlider("Contrast##rt", &e->rippleTank.curve, "rippleTank.curve",
                      "%.2f", ms);
  }
  ImGui::Combo("Falloff##rt", &e->rippleTank.falloffType,
               "None\0Inverse\0Inv-Square\0Gaussian\0");
  ModulatableSlider("Falloff Strength##rt", &e->rippleTank.falloffStrength,
                    "rippleTank.falloffStrength", "%.2f", ms);

  ImGui::SeparatorText("Visualization");
  ImGui::Combo("Visual Mode##rt", &e->rippleTank.visualMode,
               "Raw\0Absolute\0Bands\0Lines\0");
  if (e->rippleTank.visualMode >= 2) {
    ImGui::SliderInt("Contours##rt", &e->rippleTank.contourCount, 1, 20);
  }
  ModulatableSlider("Gain##rt", &e->rippleTank.visualGain,
                    "rippleTank.visualGain", "%.2f", ms);

  ImGui::SeparatorText("Color");
  ImGui::Combo("Color Mode##rt", &e->rippleTank.colorMode,
               "Intensity\0PerSource\0Chromatic\0");
  if (e->rippleTank.colorMode == 2) {
    ModulatableSlider("Chroma Spread##rt", &e->rippleTank.chromaSpread,
                      "rippleTank.chromaSpread", "%.3f", ms);
  }

  ImGui::SeparatorText("Sources");
  ImGui::SliderInt("Source Count##rt", &e->rippleTank.sourceCount, 1, 8);
  ModulatableSlider("Base Radius##rt", &e->rippleTank.baseRadius,
                    "rippleTank.baseRadius", "%.2f", ms);
  DrawLissajousControls(&e->rippleTank.lissajous, "rt_liss",
                        "rippleTank.lissajous", ms, 0.2f);

  ImGui::SeparatorText("Boundaries");
  ImGui::Checkbox("Boundaries##rt", &e->rippleTank.boundaries);
  if (e->rippleTank.boundaries) {
    ModulatableSlider("Reflection Gain##rt", &e->rippleTank.reflectionGain,
                      "rippleTank.reflectionGain", "%.2f", ms);
  }

  ImGui::SeparatorText("Trail");
  ImGui::SliderFloat("Decay##rt", &e->rippleTank.decayHalfLife, 0.1f, 5.0f,
                     "%.2f s");
  ImGui::SliderInt("Diffusion##rt", &e->rippleTank.diffusionScale, 0, 8);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(rippleTank)
REGISTER_GENERATOR_FULL(TRANSFORM_RIPPLE_TANK, RippleTank, rippleTank, "Ripple Tank",
                        SetupRippleTankBlend, SetupRippleTank, RenderRippleTank, 16,
                        DrawRippleTankParams, DrawOutput_rippleTank)
// clang-format on
