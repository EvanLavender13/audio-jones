// Lichen effect module implementation
// Three-species cyclic reaction-diffusion with coordinate warp and
// gradient-colored output

#include "lichen.h"

#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "rlgl.h"

#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

static void InitTextures(LichenEffect *e, int w, int h) {
  RenderUtilsInitTextureHDR(&e->statePingPong0[0], w, h, "LICHEN_STATE0");
  RenderUtilsInitTextureHDR(&e->statePingPong0[1], w, h, "LICHEN_STATE0");
  RenderUtilsInitTextureHDR(&e->statePingPong1[0], w, h, "LICHEN_STATE1");
  RenderUtilsInitTextureHDR(&e->statePingPong1[1], w, h, "LICHEN_STATE1");
  RenderUtilsInitTextureHDR(&e->colorRT, w, h, "LICHEN_COLOR");
  SetTextureWrap(e->statePingPong0[0].texture, TEXTURE_WRAP_REPEAT);
  SetTextureWrap(e->statePingPong0[1].texture, TEXTURE_WRAP_REPEAT);
  SetTextureWrap(e->statePingPong1[0].texture, TEXTURE_WRAP_REPEAT);
  SetTextureWrap(e->statePingPong1[1].texture, TEXTURE_WRAP_REPEAT);
}

static void UnloadTextures(const LichenEffect *e) {
  UnloadRenderTexture(e->statePingPong0[0]);
  UnloadRenderTexture(e->statePingPong0[1]);
  UnloadRenderTexture(e->statePingPong1[0]);
  UnloadRenderTexture(e->statePingPong1[1]);
  UnloadRenderTexture(e->colorRT);
}

static void InitializeSeed(LichenEffect *e, int w, int h) {
  const int count = w * h * 4;
  float *pixels0 = static_cast<float *>(malloc(count * sizeof(float)));
  if (pixels0 == NULL) {
    return;
  }
  float *pixels1 = static_cast<float *>(malloc(count * sizeof(float)));
  if (pixels1 == NULL) {
    free(pixels0);
    return;
  }

  // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded init
  const float dateOffset = (float)rand() / (float)RAND_MAX;
  const float seedPos[3] = {0.16f, 0.5f, 0.84f};
  const float seedRadius = 28.5f;
  const float noiseAmp = 10.5f;

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      const float px = (float)x;
      const float py = (float)y;
      float v[3] = {0.0f, 0.0f, 0.0f};
      for (int k = 0; k < 3; k++) {
        const float cx = (float)w * seedPos[k];
        const float cy = (float)h * seedPos[k];
        const float dx = px - cx;
        const float dy = py - cy;
        const float dist = sqrtf(dx * dx + dy * dy);
        const float noise = sinf(px * py + dateOffset) * noiseAmp;
        if (dist + noise < seedRadius) {
          v[k] = 1.0f;
        }
      }
      const int idx = (y * w + x) * 4;
      pixels0[idx + 0] = 1.0f;
      pixels0[idx + 1] = v[0];
      pixels0[idx + 2] = 1.0f;
      pixels0[idx + 3] = v[1];
      pixels1[idx + 0] = 1.0f;
      pixels1[idx + 1] = v[2];
      pixels1[idx + 2] = 0.0f;
      pixels1[idx + 3] = 0.0f;
    }
  }

  UpdateTexture(e->statePingPong0[0].texture, pixels0);
  UpdateTexture(e->statePingPong0[1].texture, pixels0);
  UpdateTexture(e->statePingPong1[0].texture, pixels1);
  UpdateTexture(e->statePingPong1[1].texture, pixels1);

  free(pixels0);
  free(pixels1);
}

static void CacheStateLocations(LichenEffect *e) {
  e->stateResolutionLoc = GetShaderLocation(e->stateShader, "resolution");
  e->stateTimeLoc = GetShaderLocation(e->stateShader, "time");
  e->statePassIndexLoc = GetShaderLocation(e->stateShader, "passIndex");
  e->stateFeedRateLoc = GetShaderLocation(e->stateShader, "feedRate");
  e->stateKillRateBaseLoc = GetShaderLocation(e->stateShader, "killRateBase");
  e->stateCouplingStrengthLoc =
      GetShaderLocation(e->stateShader, "couplingStrength");
  e->statePredatorAdvantageLoc =
      GetShaderLocation(e->stateShader, "predatorAdvantage");
  e->stateWarpIntensityLoc = GetShaderLocation(e->stateShader, "warpIntensity");
  e->stateWarpSpeedLoc = GetShaderLocation(e->stateShader, "warpSpeed");
  e->stateActivatorRadiusLoc =
      GetShaderLocation(e->stateShader, "activatorRadius");
  e->stateInhibitorRadiusLoc =
      GetShaderLocation(e->stateShader, "inhibitorRadius");
  e->stateReactionStepsLoc = GetShaderLocation(e->stateShader, "reactionSteps");
  e->stateReactionRateLoc = GetShaderLocation(e->stateShader, "reactionRate");
  e->stateTex1Loc = GetShaderLocation(e->stateShader, "stateTex1");
}

static void CacheColorLocations(LichenEffect *e) {
  e->colorResolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->colorBrightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->colorScatterLoc = GetShaderLocation(e->shader, "colorScatter");
  e->colorStateTex0Loc = GetShaderLocation(e->shader, "stateTex0");
  e->colorStateTex1Loc = GetShaderLocation(e->shader, "stateTex1");
  e->colorGradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->colorFftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->colorSampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->colorBaseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->colorMaxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->colorGainLoc = GetShaderLocation(e->shader, "gain");
  e->colorCurveLoc = GetShaderLocation(e->shader, "curve");
  e->colorBaseBrightLoc = GetShaderLocation(e->shader, "baseBright");
}

static void BindStateUniforms(const LichenEffect *e, const LichenConfig *cfg,
                              const float *resolution) {
  SetShaderValue(e->stateShader, e->stateResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->stateShader, e->stateFeedRateLoc, &cfg->feedRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateKillRateBaseLoc, &cfg->killRateBase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateCouplingStrengthLoc,
                 &cfg->couplingStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->statePredatorAdvantageLoc,
                 &cfg->predatorAdvantage, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateWarpIntensityLoc, &cfg->warpIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateWarpSpeedLoc, &cfg->warpSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateActivatorRadiusLoc,
                 &cfg->activatorRadius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateInhibitorRadiusLoc,
                 &cfg->inhibitorRadius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->stateShader, e->stateReactionStepsLoc, &cfg->reactionSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->stateShader, e->stateReactionRateLoc, &cfg->reactionRate,
                 SHADER_UNIFORM_FLOAT);
}

static void BindColorUniforms(const LichenEffect *e, const LichenConfig *cfg,
                              const float *resolution) {
  const float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->colorResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->colorBrightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorScatterLoc, &cfg->colorScatter,
                 SHADER_UNIFORM_FLOAT);
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

bool LichenEffectInit(LichenEffect *e, const LichenConfig *cfg, int width,
                      int height) {
  e->stateShader = LoadShader(NULL, "shaders/lichen_state.fs");
  if (e->stateShader.id == 0) {
    return false;
  }

  e->shader = LoadShader(NULL, "shaders/lichen.fs");
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
  InitializeSeed(e, width, height);
  RenderUtilsClearTexture(&e->colorRT);

  e->readIdx0 = 0;
  e->readIdx1 = 0;
  e->time = 0.0f;

  return true;
}

void LichenEffectSetup(LichenEffect *e, const LichenConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture) {
  e->time += deltaTime;
  const float WRAP = 1000.0f;
  if (e->time > WRAP) {
    e->time -= WRAP;
  }

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};

  BindStateUniforms(e, cfg, resolution);
  SetShaderValue(e->stateShader, e->stateTimeLoc, &e->time,
                 SHADER_UNIFORM_FLOAT);

  BindColorUniforms(e, cfg, resolution);
  e->fftTexture = fftTexture;
}

void LichenEffectRender(LichenEffect *e, const LichenConfig *cfg,
                        int screenWidth, int screenHeight) {
  int writeIdx0 = 1 - e->readIdx0;
  int writeIdx1 = 1 - e->readIdx1;

  const int steps = cfg->simSteps < 1 ? 1 : cfg->simSteps;
  for (int s = 0; s < steps; s++) {
    // Pass 0: write species 0+1 to statePingPong0[writeIdx0]
    // Both passes share the same readIdx0/readIdx1 source so coupling is
    // consistent within a sim step
    BeginTextureMode(e->statePingPong0[writeIdx0]);
    rlDisableColorBlend();
    BeginShaderMode(e->stateShader);
    SetShaderValueTexture(e->stateShader, e->stateTex1Loc,
                          e->statePingPong1[e->readIdx1].texture);
    const int passIdx0 = 0;
    SetShaderValue(e->stateShader, e->statePassIndexLoc, &passIdx0,
                   SHADER_UNIFORM_INT);
    RenderUtilsDrawFullscreenQuad(e->statePingPong0[e->readIdx0].texture,
                                  screenWidth, screenHeight);
    EndShaderMode();
    rlEnableColorBlend();
    EndTextureMode();

    // Pass 1: write species 2 to statePingPong1[writeIdx1]
    BeginTextureMode(e->statePingPong1[writeIdx1]);
    rlDisableColorBlend();
    BeginShaderMode(e->stateShader);
    SetShaderValueTexture(e->stateShader, e->stateTex1Loc,
                          e->statePingPong1[e->readIdx1].texture);
    const int passIdx1 = 1;
    SetShaderValue(e->stateShader, e->statePassIndexLoc, &passIdx1,
                   SHADER_UNIFORM_INT);
    RenderUtilsDrawFullscreenQuad(e->statePingPong0[e->readIdx0].texture,
                                  screenWidth, screenHeight);
    EndShaderMode();
    rlEnableColorBlend();
    EndTextureMode();

    e->readIdx0 = writeIdx0;
    e->readIdx1 = writeIdx1;
    writeIdx0 = 1 - e->readIdx0;
    writeIdx1 = 1 - e->readIdx1;
  }

  // Color pass: read latest state textures, write to colorRT
  BeginTextureMode(e->colorRT);
  BeginShaderMode(e->shader);
  SetShaderValueTexture(e->shader, e->colorStateTex0Loc,
                        e->statePingPong0[e->readIdx0].texture);
  SetShaderValueTexture(e->shader, e->colorStateTex1Loc,
                        e->statePingPong1[e->readIdx1].texture);
  SetShaderValueTexture(e->shader, e->colorGradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->colorFftTextureLoc, e->fftTexture);
  RenderUtilsDrawFullscreenQuad(e->statePingPong0[e->readIdx0].texture,
                                screenWidth, screenHeight);
  EndShaderMode();
  EndTextureMode();
}

void LichenEffectResize(LichenEffect *e, int width, int height) {
  UnloadTextures(e);
  InitTextures(e, width, height);
  InitializeSeed(e, width, height);
  RenderUtilsClearTexture(&e->colorRT);
  e->readIdx0 = 0;
  e->readIdx1 = 0;
}

void LichenEffectReset(LichenEffect *e, int width, int height) {
  RenderUtilsClearTexture(&e->colorRT);
  InitializeSeed(e, width, height);
  e->readIdx0 = 0;
  e->readIdx1 = 0;
}

void LichenEffectUninit(LichenEffect *e) {
  UnloadShader(e->stateShader);
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadTextures(e);
}

void LichenRegisterParams(LichenConfig *cfg) {
  ModEngineRegisterParam("lichen.feedRate", &cfg->feedRate, 0.005f, 0.08f);
  ModEngineRegisterParam("lichen.killRateBase", &cfg->killRateBase, 0.01f,
                         0.12f);
  ModEngineRegisterParam("lichen.couplingStrength", &cfg->couplingStrength,
                         0.0f, 0.2f);
  ModEngineRegisterParam("lichen.predatorAdvantage", &cfg->predatorAdvantage,
                         0.8f, 1.5f);
  ModEngineRegisterParam("lichen.warpIntensity", &cfg->warpIntensity, 0.0f,
                         0.5f);
  ModEngineRegisterParam("lichen.warpSpeed", &cfg->warpSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("lichen.activatorRadius", &cfg->activatorRadius, 0.5f,
                         5.0f);
  ModEngineRegisterParam("lichen.inhibitorRadius", &cfg->inhibitorRadius, 0.5f,
                         3.0f);
  ModEngineRegisterParam("lichen.reactionRate", &cfg->reactionRate, 0.1f, 0.8f);
  ModEngineRegisterParam("lichen.brightness", &cfg->brightness, 0.5f, 4.0f);
  ModEngineRegisterParam("lichen.colorScatter", &cfg->colorScatter, 1.0f,
                         120.0f);
  ModEngineRegisterParam("lichen.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("lichen.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("lichen.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("lichen.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("lichen.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("lichen.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

LichenEffect *GetLichenEffect(PostEffect *pe) {
  return (LichenEffect *)pe->effectStates[TRANSFORM_LICHEN];
}

void SetupLichen(PostEffect *pe) {
  LichenEffectSetup(GetLichenEffect(pe), &pe->effects.lichen,
                    pe->currentDeltaTime, pe->fftTexture);
}

void SetupLichenBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, GetLichenEffect(pe)->colorRT.texture,
      pe->effects.lichen.blendIntensity, pe->effects.lichen.blendMode);
}

void RenderLichen(PostEffect *pe) {
  LichenEffectRender(GetLichenEffect(pe), &pe->effects.lichen, pe->screenWidth,
                     pe->screenHeight);
}

// === UI ===

static void DrawLichenParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  (void)categoryGlow;
  LichenConfig *cfg = &e->lichen;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##lichen", &cfg->baseFreq, "lichen.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##lichen", &cfg->maxFreq, "lichen.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##lichen", &cfg->gain, "lichen.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##lichen", &cfg->curve, "lichen.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##lichen", &cfg->baseBright,
                    "lichen.baseBright", "%.2f", modSources);

  ImGui::SeparatorText("Reaction");
  ModulatableSlider("Feed Rate##lichen", &cfg->feedRate, "lichen.feedRate",
                    "%.3f", modSources);
  ModulatableSlider("Kill Base##lichen", &cfg->killRateBase,
                    "lichen.killRateBase", "%.3f", modSources);
  ModulatableSlider("Coupling##lichen", &cfg->couplingStrength,
                    "lichen.couplingStrength", "%.3f", modSources);
  ModulatableSlider("Pred Advantage##lichen", &cfg->predatorAdvantage,
                    "lichen.predatorAdvantage", "%.2f", modSources);
  ModulatableSlider("Reaction Rate##lichen", &cfg->reactionRate,
                    "lichen.reactionRate", "%.2f", modSources);
  ImGui::SliderInt("Reaction Steps##lichen", &cfg->reactionSteps, 5, 50);
  ImGui::SliderInt("Sim Steps##lichen", &cfg->simSteps, 1, 8);

  ImGui::SeparatorText("Diffusion");
  ModulatableSlider("Activator Radius##lichen", &cfg->activatorRadius,
                    "lichen.activatorRadius", "%.2f", modSources);
  ModulatableSlider("Inhibitor Radius##lichen", &cfg->inhibitorRadius,
                    "lichen.inhibitorRadius", "%.2f", modSources);

  ImGui::SeparatorText("Warp");
  ModulatableSlider("Warp Intensity##lichen", &cfg->warpIntensity,
                    "lichen.warpIntensity", "%.2f", modSources);
  ModulatableSlider("Warp Speed##lichen", &cfg->warpSpeed, "lichen.warpSpeed",
                    "%.2f", modSources);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Brightness##lichen", &cfg->brightness, "lichen.brightness",
                    "%.2f", modSources);
  ModulatableSlider("Color Scatter##lichen", &cfg->colorScatter,
                    "lichen.colorScatter", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(lichen)
REGISTER_GENERATOR_FULL(TRANSFORM_LICHEN, Lichen, lichen,
                        "Lichen", SetupLichenBlend,
                        SetupLichen, RenderLichen, 13,
                        DrawLichenParams, DrawOutput_lichen)
// clang-format on
