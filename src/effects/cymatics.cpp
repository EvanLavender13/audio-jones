// Cymatics effect module implementation
// Simulated Chladni-plate standing-wave interference patterns

#include "cymatics.h"
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

static void InitPingPong(CymaticsEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "CYMATICS");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "CYMATICS");
}

static void UnloadPingPong(CymaticsEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool CymaticsEffectInit(CymaticsEffect *e, const CymaticsConfig *cfg, int width,
                        int height) {
  e->shader = LoadShader(NULL, "shaders/cymatics.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->aspectLoc = GetShaderLocation(e->shader, "aspect");
  e->waveScaleLoc = GetShaderLocation(e->shader, "waveScale");
  e->falloffLoc = GetShaderLocation(e->shader, "falloff");
  e->visualGainLoc = GetShaderLocation(e->shader, "visualGain");
  e->contourCountLoc = GetShaderLocation(e->shader, "contourCount");
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
  e->time = 0.0f;

  return true;
}

void CymaticsEffectSetup(CymaticsEffect *e, const CymaticsConfig *cfg,
                         float deltaTime, Texture2D waveformTexture,
                         int waveformWriteIndex) {
  e->time += deltaTime;
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

  int bufferSize = waveformTexture.width;
  SetShaderValue(e->shader, e->bufferSizeLoc, &bufferSize, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->writeIndexLoc, &waveformWriteIndex,
                 SHADER_UNIFORM_INT);

  // Compute source positions via circular Lissajous distribution
  float sources[16]; // 8 sources * 2 components
  const int count = cfg->sourceCount > 8 ? 8 : cfg->sourceCount;
  DualLissajousUpdateCircular(
      const_cast<DualLissajousConfig *>(&cfg->lissajous), deltaTime,
      cfg->baseRadius, 0.0f, 0.0f, count, sources);
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

void CymaticsEffectRender(CymaticsEffect *e, const CymaticsConfig *cfg,
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

void CymaticsEffectResize(CymaticsEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void CymaticsEffectUninit(CymaticsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
  UnloadPingPong(e);
}

CymaticsConfig CymaticsConfigDefault(void) { return CymaticsConfig{}; }

void CymaticsRegisterParams(CymaticsConfig *cfg) {
  ModEngineRegisterParam("cymatics.waveScale", &cfg->waveScale, 1.0f, 50.0f);
  ModEngineRegisterParam("cymatics.falloff", &cfg->falloff, 0.0f, 5.0f);
  ModEngineRegisterParam("cymatics.visualGain", &cfg->visualGain, 0.5f, 5.0f);
  ModEngineRegisterParam("cymatics.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
  ModEngineRegisterParam("cymatics.baseRadius", &cfg->baseRadius, 0.0f, 0.5f);
  ModEngineRegisterParam("cymatics.reflectionGain", &cfg->reflectionGain, 0.0f,
                         1.0f);
  ModEngineRegisterParam("cymatics.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("cymatics.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
}

void SetupCymatics(PostEffect *pe) {
  CymaticsEffectSetup(&pe->cymatics, &pe->effects.cymatics,
                      pe->currentDeltaTime, pe->waveformTexture,
                      pe->waveformWriteIndex);
}

void SetupCymaticsBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, pe->cymatics.pingPong[pe->cymatics.readIdx].texture,
      pe->effects.cymatics.blendIntensity, pe->effects.cymatics.blendMode);
}

void RenderCymatics(PostEffect *pe) {
  CymaticsEffectRender(&pe->cymatics, &pe->effects.cymatics,
                       pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
}

// === UI ===

static void DrawCymaticsParams(EffectConfig *e, const ModSources *ms,
                               ImU32 categoryGlow) {
  (void)categoryGlow;

  ImGui::SeparatorText("Wave");
  ModulatableSlider("Wave Scale##cym", &e->cymatics.waveScale,
                    "cymatics.waveScale", "%.1f", ms);
  ModulatableSlider("Falloff##cym", &e->cymatics.falloff, "cymatics.falloff",
                    "%.2f", ms);
  ModulatableSlider("Gain##cym", &e->cymatics.visualGain, "cymatics.visualGain",
                    "%.2f", ms);
  ImGui::SliderInt("Contours##cym", &e->cymatics.contourCount, 0, 10);

  ImGui::SeparatorText("Boundaries");
  ImGui::Checkbox("Boundaries##cym", &e->cymatics.boundaries);
  if (e->cymatics.boundaries) {
    ModulatableSlider("Reflection Gain##cym", &e->cymatics.reflectionGain,
                      "cymatics.reflectionGain", "%.2f", ms);
  }

  ImGui::SeparatorText("Sources");
  ImGui::SliderInt("Source Count##cym", &e->cymatics.sourceCount, 1, 8);
  ModulatableSlider("Base Radius##cym", &e->cymatics.baseRadius,
                    "cymatics.baseRadius", "%.2f", ms);
  DrawLissajousControls(&e->cymatics.lissajous, "cym_liss",
                        "cymatics.lissajous", ms, 0.2f);

  ImGui::SeparatorText("Trail");
  ImGui::SliderFloat("Decay##cym", &e->cymatics.decayHalfLife, 0.1f, 5.0f,
                     "%.2f s");
  ImGui::SliderInt("Diffusion##cym", &e->cymatics.diffusionScale, 0, 8);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(cymatics)
REGISTER_GENERATOR_FULL(TRANSFORM_CYMATICS, Cymatics, cymatics, "Cymatics",
                        SetupCymaticsBlend, SetupCymatics, RenderCymatics, 13,
                        DrawCymaticsParams, DrawOutput_cymatics)
// clang-format on
