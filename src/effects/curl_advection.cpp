// Curl advection effect module implementation
// GPU curl-noise fluid advection with divergence feedback and gradient-colored
// trails

#include "curl_advection.h"
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
#include "rlgl.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

static void InitPingPong(CurlAdvectionEffect *e, int w, int h) {
  RenderUtilsInitTextureHDR(&e->statePingPong[0], w, h, "CURL_ADV_STATE");
  RenderUtilsInitTextureHDR(&e->statePingPong[1], w, h, "CURL_ADV_STATE");
  RenderUtilsInitTextureHDR(&e->pingPong[0], w, h, "CURL_ADV");
  RenderUtilsInitTextureHDR(&e->pingPong[1], w, h, "CURL_ADV");
}

static void UnloadPingPong(const CurlAdvectionEffect *e) {
  UnloadRenderTexture(e->statePingPong[0]);
  UnloadRenderTexture(e->statePingPong[1]);
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

static void InitializeStateWithNoise(const CurlAdvectionEffect *e, int w,
                                     int h) {
  const int count = w * h * 4;
  float *pixels = static_cast<float *>(malloc(count * sizeof(float)));
  if (pixels == NULL) {
    return;
  }

  for (int i = 0; i < w * h; i++) {
    // Random velocity in [-0.1, 0.1]
    // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded init
    pixels[i * 4 + 0] = ((float)rand() / RAND_MAX) * 0.2f - 0.1f;
    // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded init
    pixels[i * 4 + 1] = ((float)rand() / RAND_MAX) * 0.2f - 0.1f;
    // Divergence starts at zero
    pixels[i * 4 + 2] = 0.0f;
    pixels[i * 4 + 3] = 0.0f;
  }

  UpdateTexture(e->statePingPong[0].texture, pixels);
  UpdateTexture(e->statePingPong[1].texture, pixels);

  free(pixels);
}

static void CacheStateLocations(CurlAdvectionEffect *e) {
  e->stateResolutionLoc = GetShaderLocation(e->stateShader, "resolution");
  e->stateStepsLoc = GetShaderLocation(e->stateShader, "steps");
  e->stateAdvectionCurlLoc = GetShaderLocation(e->stateShader, "advectionCurl");
  e->stateCurlScaleLoc = GetShaderLocation(e->stateShader, "curlScale");
  e->stateLaplacianScaleLoc =
      GetShaderLocation(e->stateShader, "laplacianScale");
  e->statePressureScaleLoc = GetShaderLocation(e->stateShader, "pressureScale");
  e->stateDivergenceScaleLoc =
      GetShaderLocation(e->stateShader, "divergenceScale");
  e->stateDivergenceUpdateLoc =
      GetShaderLocation(e->stateShader, "divergenceUpdate");
  e->stateDivergenceSmoothingLoc =
      GetShaderLocation(e->stateShader, "divergenceSmoothing");
  e->stateSelfAmpLoc = GetShaderLocation(e->stateShader, "selfAmp");
  e->stateUpdateSmoothingLoc =
      GetShaderLocation(e->stateShader, "updateSmoothing");
  e->stateInjectionIntensityLoc =
      GetShaderLocation(e->stateShader, "injectionIntensity");
  e->stateInjectionThresholdLoc =
      GetShaderLocation(e->stateShader, "injectionThreshold");
  // texture0 is auto-bound by DrawTextureRec (previous state)
  e->stateAccumTextureLoc = GetShaderLocation(e->stateShader, "accumTexture");
}

static void CacheColorLocations(CurlAdvectionEffect *e) {
  e->colorStateTextureLoc = GetShaderLocation(e->shader, "stateTexture");
  // texture0 is auto-bound by DrawTextureRec (previous visual frame)
  e->colorDecayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->colorLUTLoc = GetShaderLocation(e->shader, "colorLUT");
  e->colorValueLoc = GetShaderLocation(e->shader, "value");
  e->colorResolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->colorDiffusionScaleLoc = GetShaderLocation(e->shader, "diffusionScale");
}

bool CurlAdvectionEffectInit(CurlAdvectionEffect *e,
                             const CurlAdvectionConfig *cfg, int width,
                             int height) {
  e->stateShader = LoadShader(NULL, "shaders/curl_advection_state.fs");
  if (e->stateShader.id == 0) {
    return false;
  }

  e->shader = LoadShader(NULL, "shaders/curl_advection_color.fs");
  if (e->shader.id == 0) {
    UnloadShader(e->stateShader);
    return false;
  }

  CacheStateLocations(e);
  CacheColorLocations(e);

  e->colorLUT = ColorLUTInit(&cfg->color);
  if (e->colorLUT == NULL) {
    UnloadShader(e->shader);
    UnloadShader(e->stateShader);
    return false;
  }

  InitPingPong(e, width, height);
  InitializeStateWithNoise(e, width, height);

  RenderUtilsClearTexture(&e->pingPong[0]);
  RenderUtilsClearTexture(&e->pingPong[1]);

  e->stateReadIdx = 0;
  e->readIdx = 0;

  return true;
}

static void BindStateUniforms(const CurlAdvectionEffect *e,
                              const CurlAdvectionConfig *cfg,
                              const float *resolution) {
  SetShaderValue(e->stateShader, e->stateResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->stateShader, e->stateStepsLoc, &cfg->steps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->stateShader, e->stateAdvectionCurlLoc, &cfg->advectionCurl,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateCurlScaleLoc, &cfg->curlScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateLaplacianScaleLoc,
                 &cfg->laplacianScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->statePressureScaleLoc, &cfg->pressureScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateDivergenceScaleLoc,
                 &cfg->divergenceScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateDivergenceUpdateLoc,
                 &cfg->divergenceUpdate, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateDivergenceSmoothingLoc,
                 &cfg->divergenceSmoothing, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateSelfAmpLoc, &cfg->selfAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateUpdateSmoothingLoc,
                 &cfg->updateSmoothing, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateInjectionIntensityLoc,
                 &cfg->injectionIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateInjectionThresholdLoc,
                 &cfg->injectionThreshold, SHADER_UNIFORM_FLOAT);
}

static void BindColorUniforms(const CurlAdvectionEffect *e,
                              const CurlAdvectionConfig *cfg, float deltaTime,
                              const float *resolution) {
  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->colorDecayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);

  float value;
  if (cfg->color.mode == COLOR_MODE_SOLID) {
    float h;
    float s;
    ColorConfigRGBToHSV(cfg->color.solid, &h, &s, &value);
  } else if (cfg->color.mode == COLOR_MODE_GRADIENT) {
    value = 1.0f;
  } else if (cfg->color.mode == COLOR_MODE_PALETTE) {
    float s;
    ColorConfigGetSV(&cfg->color, &s, &value);
  } else {
    value = cfg->color.rainbowVal;
  }
  SetShaderValue(e->shader, e->colorValueLoc, &value, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->colorDiffusionScaleLoc, &cfg->diffusionScale,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->colorResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
}

void CurlAdvectionEffectSetup(CurlAdvectionEffect *e,
                              const CurlAdvectionConfig *cfg, float deltaTime) {
  ColorLUTUpdate(e->colorLUT, &cfg->color);
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  BindStateUniforms(e, cfg, resolution);
  BindColorUniforms(e, cfg, deltaTime, resolution);
}

void CurlAdvectionEffectRender(CurlAdvectionEffect *e,
                               const CurlAdvectionConfig *cfg, float deltaTime,
                               int screenWidth, int screenHeight) {
  (void)cfg;
  (void)deltaTime;

  const int stateWriteIdx = 1 - e->stateReadIdx;
  const int writeIdx = 1 - e->readIdx;

  // Pass 1: State update (texture0 = previous state, auto-bound)
  BeginTextureMode(e->statePingPong[stateWriteIdx]);
  rlDisableColorBlend(); // State has alpha=0, must overwrite not blend
  BeginShaderMode(e->stateShader);
  SetShaderValueTexture(e->stateShader, e->stateAccumTextureLoc,
                        e->currentAccumTexture);
  RenderUtilsDrawFullscreenQuad(e->statePingPong[e->stateReadIdx].texture,
                                screenWidth, screenHeight);
  EndShaderMode();
  rlEnableColorBlend();
  EndTextureMode();

  // Pass 2: Color output (texture0 = previous visual frame, auto-bound)
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);
  SetShaderValueTexture(e->shader, e->colorStateTextureLoc,
                        e->statePingPong[stateWriteIdx].texture);
  SetShaderValueTexture(e->shader, e->colorLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));
  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  // Flip indices
  e->stateReadIdx = stateWriteIdx;
  e->readIdx = writeIdx;
}

void CurlAdvectionEffectResize(CurlAdvectionEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  InitializeStateWithNoise(e, width, height);
  RenderUtilsClearTexture(&e->pingPong[0]);
  RenderUtilsClearTexture(&e->pingPong[1]);
  e->stateReadIdx = 0;
  e->readIdx = 0;
}

void CurlAdvectionEffectReset(CurlAdvectionEffect *e, int width, int height) {
  for (int i = 0; i < 2; i++) {
    BeginTextureMode(e->pingPong[i]);
    ClearBackground(BLACK);
    EndTextureMode();
  }
  InitializeStateWithNoise(e, width, height);
  e->stateReadIdx = 0;
  e->readIdx = 0;
}

void CurlAdvectionEffectUninit(CurlAdvectionEffect *e) {
  UnloadShader(e->stateShader);
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
  UnloadPingPong(e);
}

void CurlAdvectionRegisterParams(CurlAdvectionConfig *cfg) {
  ModEngineRegisterParam("curlAdvection.advectionCurl", &cfg->advectionCurl,
                         0.0f, 1.0f);
  ModEngineRegisterParam("curlAdvection.curlScale", &cfg->curlScale, -4.0f,
                         4.0f);
  ModEngineRegisterParam("curlAdvection.selfAmp", &cfg->selfAmp, 0.5f, 2.0f);
  ModEngineRegisterParam("curlAdvection.laplacianScale", &cfg->laplacianScale,
                         0.0f, 0.2f);
  ModEngineRegisterParam("curlAdvection.pressureScale", &cfg->pressureScale,
                         -4.0f, 4.0f);
  ModEngineRegisterParam("curlAdvection.divergenceScale", &cfg->divergenceScale,
                         -1.0f, 1.0f);
  ModEngineRegisterParam("curlAdvection.divergenceUpdate",
                         &cfg->divergenceUpdate, -0.1f, 0.1f);
  ModEngineRegisterParam("curlAdvection.divergenceSmoothing",
                         &cfg->divergenceSmoothing, 0.0f, 0.5f);
  ModEngineRegisterParam("curlAdvection.updateSmoothing", &cfg->updateSmoothing,
                         0.25f, 0.9f);
  ModEngineRegisterParam("curlAdvection.injectionIntensity",
                         &cfg->injectionIntensity, 0.0f, 1.0f);
  ModEngineRegisterParam("curlAdvection.injectionThreshold",
                         &cfg->injectionThreshold, 0.0f, 1.0f);
  ModEngineRegisterParam("curlAdvection.decayHalfLife", &cfg->decayHalfLife,
                         0.1f, 5.0f);
  ModEngineRegisterParam("curlAdvection.boostIntensity", &cfg->boostIntensity,
                         0.0f, 5.0f);
}

CurlAdvectionEffect *GetCurlAdvectionEffect(PostEffect *pe) {
  return (CurlAdvectionEffect *)pe->effectStates[TRANSFORM_CURL_ADVECTION];
}

void SetupCurlAdvection(PostEffect *pe) {
  CurlAdvectionEffect *e = GetCurlAdvectionEffect(pe);
  const CurlAdvectionConfig *cfg = &pe->effects.curlAdvection;
  e->currentAccumTexture = pe->accumTexture.texture;
  CurlAdvectionEffectSetup(e, cfg, GetFrameTime());
}

void SetupCurlAdvectionBlend(PostEffect *pe) {
  CurlAdvectionEffect *e = GetCurlAdvectionEffect(pe);
  BlendCompositorApply(pe->blendCompositor, e->pingPong[e->readIdx].texture,
                       pe->effects.curlAdvection.boostIntensity,
                       pe->effects.curlAdvection.blendMode);
}

void RenderCurlAdvection(PostEffect *pe) {
  CurlAdvectionEffect *e = GetCurlAdvectionEffect(pe);
  const CurlAdvectionConfig *cfg = &pe->effects.curlAdvection;
  CurlAdvectionEffectRender(e, cfg, GetFrameTime(), pe->screenWidth,
                            pe->screenHeight);
}

// === UI ===

static void DrawCurlAdvectionParams(EffectConfig *e, const ModSources *ms,
                                    ImU32) {
  ImGui::SeparatorText("Field");
  ImGui::SliderInt("Steps##curlAdv", &e->curlAdvection.steps, 10, 80);
  ModulatableSlider("Advection Curl##curlAdv", &e->curlAdvection.advectionCurl,
                    "curlAdvection.advectionCurl", "%.2f", ms);
  ModulatableSlider("Curl Scale##curlAdv", &e->curlAdvection.curlScale,
                    "curlAdvection.curlScale", "%.2f", ms);
  ModulatableSlider("Self Amp##curlAdv", &e->curlAdvection.selfAmp,
                    "curlAdvection.selfAmp", "%.2f", ms);

  ImGui::SeparatorText("Pressure");
  ModulatableSlider("Laplacian##curlAdv", &e->curlAdvection.laplacianScale,
                    "curlAdvection.laplacianScale", "%.3f", ms);
  ModulatableSlider("Pressure##curlAdv", &e->curlAdvection.pressureScale,
                    "curlAdvection.pressureScale", "%.2f", ms);
  ModulatableSlider("Div Scale##curlAdv", &e->curlAdvection.divergenceScale,
                    "curlAdvection.divergenceScale", "%.2f", ms);
  ModulatableSlider("Div Update##curlAdv", &e->curlAdvection.divergenceUpdate,
                    "curlAdvection.divergenceUpdate", "%.3f", ms);
  ModulatableSlider("Div Smooth##curlAdv",
                    &e->curlAdvection.divergenceSmoothing,
                    "curlAdvection.divergenceSmoothing", "%.2f", ms);
  ModulatableSlider("Update Smooth##curlAdv", &e->curlAdvection.updateSmoothing,
                    "curlAdvection.updateSmoothing", "%.2f", ms);

  ImGui::SeparatorText("Injection");
  ModulatableSlider("Injection##curlAdv", &e->curlAdvection.injectionIntensity,
                    "curlAdvection.injectionIntensity", "%.2f", ms);
  ModulatableSlider("Inj Threshold##curlAdv",
                    &e->curlAdvection.injectionThreshold,
                    "curlAdvection.injectionThreshold", "%.2f", ms);

  ImGui::SeparatorText("Trail");
  ModulatableSlider("Decay##curlAdv", &e->curlAdvection.decayHalfLife,
                    "curlAdvection.decayHalfLife", "%.2f s", ms);
  ImGui::SliderInt("Diffusion##curlAdv", &e->curlAdvection.diffusionScale, 0,
                   4);
}

static void DrawOutput_curlAdvection(EffectConfig *e, const ModSources *ms) {
  ImGuiDrawColorMode(&e->curlAdvection.color);
  ImGui::SeparatorText("Output");
  ModulatableSlider("Boost Intensity##curlAdvection",
                    &e->curlAdvection.boostIntensity,
                    "curlAdvection.boostIntensity", "%.2f", ms);
  int blendModeInt = (int)e->curlAdvection.blendMode;
  if (ImGui::Combo("Blend Mode##curlAdvection", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    e->curlAdvection.blendMode = (EffectBlendMode)blendModeInt;
  }
}

// clang-format off
REGISTER_GENERATOR_FULL(TRANSFORM_CURL_ADVECTION, CurlAdvection, curlAdvection,
                        "Curl Advection", SetupCurlAdvectionBlend,
                        SetupCurlAdvection, RenderCurlAdvection, 12,
                        DrawCurlAdvectionParams, DrawOutput_curlAdvection)
// clang-format on
