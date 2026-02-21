// Muons effect module implementation
// Raymarched turbulent ring trails through a volumetric noise field

#include "muons.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include <math.h>
#include <stddef.h>

static void InitPingPong(MuonsEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "MUONS");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "MUONS");
}

static void UnloadPingPong(MuonsEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool MuonsEffectInit(MuonsEffect *e, const MuonsConfig *cfg, int width,
                     int height) {
  e->shader = LoadShader(NULL, "shaders/muons.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->turbulenceOctavesLoc = GetShaderLocation(e->shader, "turbulenceOctaves");
  e->turbulenceStrengthLoc = GetShaderLocation(e->shader, "turbulenceStrength");
  e->ringThicknessLoc = GetShaderLocation(e->shader, "ringThickness");
  e->cameraDistanceLoc = GetShaderLocation(e->shader, "cameraDistance");
  e->colorFreqLoc = GetShaderLocation(e->shader, "colorFreq");
  e->colorSpeedLoc = GetShaderLocation(e->shader, "colorSpeed");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->exposureLoc = GetShaderLocation(e->shader, "exposure");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->previousFrameLoc = GetShaderLocation(e->shader, "previousFrame");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
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

  return true;
}

void MuonsEffectSetup(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime,
                      Texture2D fftTexture) {
  e->time += deltaTime;
  e->currentFFTTexture = fftTexture;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->turbulenceOctavesLoc, &cfg->turbulenceOctaves,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->turbulenceStrengthLoc, &cfg->turbulenceStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ringThicknessLoc, &cfg->ringThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraDistanceLoc, &cfg->cameraDistance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorFreqLoc, &cfg->colorFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorSpeedLoc, &cfg->colorSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->exposureLoc, &cfg->exposure,
                 SHADER_UNIFORM_FLOAT);

  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
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

void MuonsEffectRender(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime,
                       int screenWidth, int screenHeight) {
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

void MuonsEffectResize(MuonsEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void MuonsEffectUninit(MuonsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

MuonsConfig MuonsConfigDefault(void) { return MuonsConfig{}; }

void MuonsRegisterParams(MuonsConfig *cfg) {
  ModEngineRegisterParam("muons.turbulenceStrength", &cfg->turbulenceStrength,
                         0.0f, 2.0f);
  ModEngineRegisterParam("muons.ringThickness", &cfg->ringThickness, 0.005f,
                         0.1f);
  ModEngineRegisterParam("muons.cameraDistance", &cfg->cameraDistance, 3.0f,
                         20.0f);
  ModEngineRegisterParam("muons.decayHalfLife", &cfg->decayHalfLife, 0.1f,
                         10.0f);
  ModEngineRegisterParam("muons.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("muons.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("muons.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("muons.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("muons.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("muons.colorFreq", &cfg->colorFreq, 0.5f, 50.0f);
  ModEngineRegisterParam("muons.colorSpeed", &cfg->colorSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("muons.brightness", &cfg->brightness, 0.1f, 5.0f);
  ModEngineRegisterParam("muons.exposure", &cfg->exposure, 500.0f, 10000.0f);
  ModEngineRegisterParam("muons.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupMuons(PostEffect *pe) {
  MuonsEffectSetup(&pe->muons, &pe->effects.muons, pe->currentDeltaTime,
                   pe->fftTexture);
}

void SetupMuonsBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, pe->muons.pingPong[pe->muons.readIdx].texture,
      pe->effects.muons.blendIntensity, pe->effects.muons.blendMode);
}

void RenderMuons(PostEffect *pe) {
  MuonsEffectRender(&pe->muons, &pe->effects.muons, pe->currentDeltaTime,
                    pe->screenWidth, pe->screenHeight);
}

// clang-format off
REGISTER_GENERATOR_FULL(TRANSFORM_MUONS_BLEND, Muons, muons, "Muons Blend",
                        SetupMuonsBlend, SetupMuons, RenderMuons)
// clang-format on
