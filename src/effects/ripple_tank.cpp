// Ripple Tank effect module implementation
// Audio-reactive wave interference from virtual point sources

#include "ripple_tank.h"
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
  e->falloffLoc = GetShaderLocation(e->shader, "falloff");
  e->visualGainLoc = GetShaderLocation(e->shader, "visualGain");
  e->contourCountLoc = GetShaderLocation(e->shader, "contourCount");
  e->contourModeLoc = GetShaderLocation(e->shader, "contourMode");
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

void RippleTankEffectSetup(RippleTankEffect *e, RippleTankConfig *cfg,
                           float deltaTime, Texture2D waveformTexture,
                           int waveformWriteIndex) {
  e->currentWaveformTexture = waveformTexture;

  ColorLUTUpdate(e->colorLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  float aspect = resolution[0] / resolution[1];
  SetShaderValue(e->shader, e->aspectLoc, &aspect, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->waveScaleLoc, &cfg->waveScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->falloffLoc, &cfg->falloff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->visualGainLoc, &cfg->visualGain,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->contourCountLoc, &cfg->contourCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->contourModeLoc, &cfg->contourMode,
                 SHADER_UNIFORM_INT);

  int bufferSize = waveformTexture.width;
  SetShaderValue(e->shader, e->bufferSizeLoc, &bufferSize, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->writeIndexLoc, &waveformWriteIndex,
                 SHADER_UNIFORM_INT);

  // Compute source positions via circular Lissajous distribution
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
  // Compute brightness value from color mode
  float value;
  if (cfg->gradient.mode == COLOR_MODE_SOLID) {
    float h, s;
    ColorConfigRGBToHSV(cfg->gradient.solid, &h, &s, &value);
  } else if (cfg->gradient.mode == COLOR_MODE_GRADIENT) {
    value = 1.0f;
  } else if (cfg->gradient.mode == COLOR_MODE_PALETTE) {
    float h, s;
    ColorConfigGetSV(&cfg->gradient, &s, &value);
  } else {
    value = cfg->gradient.rainbowVal;
  }
  SetShaderValue(e->shader, e->valueLoc, &value, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->diffusionScaleLoc, &cfg->diffusionScale,
                 SHADER_UNIFORM_INT);

  // Compute exponential decay factor from half-life
  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
}

void RippleTankEffectRender(RippleTankEffect *e, const RippleTankConfig *cfg,
                            float deltaTime, int screenWidth, int screenHeight) {
  (void)cfg;
  (void)deltaTime;

  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  SetShaderValueTexture(e->shader, e->waveformTextureLoc,
                        e->currentWaveformTexture);
  SetShaderValueTexture(e->shader, e->colorLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));

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
  ModEngineRegisterParam("rippleTank.falloff", &cfg->falloff, 0.0f, 5.0f);
  ModEngineRegisterParam("rippleTank.visualGain", &cfg->visualGain, 0.5f, 5.0f);
  ModEngineRegisterParam("rippleTank.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
  ModEngineRegisterParam("rippleTank.baseRadius", &cfg->baseRadius, 0.0f, 0.5f);
  ModEngineRegisterParam("rippleTank.reflectionGain", &cfg->reflectionGain, 0.0f,
                         1.0f);
  ModEngineRegisterParam("rippleTank.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("rippleTank.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
}

void SetupRippleTank(PostEffect *pe) {
  RippleTankEffectSetup(&pe->rippleTank, &pe->effects.rippleTank,
                        pe->currentDeltaTime, pe->waveformTexture,
                        pe->waveformWriteIndex);
}

void SetupRippleTankBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, pe->rippleTank.pingPong[pe->rippleTank.readIdx].texture,
      pe->effects.rippleTank.blendIntensity, pe->effects.rippleTank.blendMode);
}

void RenderRippleTank(PostEffect *pe) {
  RippleTankEffectRender(&pe->rippleTank, &pe->effects.rippleTank,
                         pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
}

// === UI ===

static void DrawRippleTankParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;

  ImGui::SeparatorText("Wave");
  ModulatableSlider("Wave Scale##rt", &e->rippleTank.waveScale,
                    "rippleTank.waveScale", "%.1f", ms);
  ModulatableSlider("Falloff##rt", &e->rippleTank.falloff, "rippleTank.falloff",
                    "%.2f", ms);
  ModulatableSlider("Gain##rt", &e->rippleTank.visualGain, "rippleTank.visualGain",
                    "%.2f", ms);
  ImGui::Combo("Contour Mode##rt", &e->rippleTank.contourMode,
               "Off\0Bands\0Lines\0");
  if (e->rippleTank.contourMode > 0) {
    ImGui::SliderInt("Contours##rt", &e->rippleTank.contourCount, 1, 20);
  }

  ImGui::SeparatorText("Boundaries");
  ImGui::Checkbox("Boundaries##rt", &e->rippleTank.boundaries);
  if (e->rippleTank.boundaries) {
    ModulatableSlider("Reflection Gain##rt", &e->rippleTank.reflectionGain,
                      "rippleTank.reflectionGain", "%.2f", ms);
  }

  ImGui::SeparatorText("Sources");
  ImGui::SliderInt("Source Count##rt", &e->rippleTank.sourceCount, 1, 8);
  ModulatableSlider("Base Radius##rt", &e->rippleTank.baseRadius,
                    "rippleTank.baseRadius", "%.2f", ms);
  DrawLissajousControls(&e->rippleTank.lissajous, "rt_liss",
                        "rippleTank.lissajous", ms, 0.2f);

  ImGui::SeparatorText("Trail");
  ImGui::SliderFloat("Decay##rt", &e->rippleTank.decayHalfLife, 0.1f, 5.0f,
                     "%.2f s");
  ImGui::SliderInt("Diffusion##rt", &e->rippleTank.diffusionScale, 0, 8);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(rippleTank)
REGISTER_GENERATOR_FULL(TRANSFORM_RIPPLE_TANK, RippleTank, rippleTank, "Ripple Tank",
                        SetupRippleTankBlend, SetupRippleTank, RenderRippleTank, 13,
                        DrawRippleTankParams, DrawOutput_rippleTank)
// clang-format on
