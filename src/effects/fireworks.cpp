// Fireworks effect module implementation
// Burst particles with gravity, drag, and trail persistence via ping-pong decay

#include "fireworks.h"
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

static void CacheLocations(FireworksEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->previousFrameLoc = GetShaderLocation(e->shader, "previousFrame");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->maxBurstsLoc = GetShaderLocation(e->shader, "maxBursts");
  e->particlesLoc = GetShaderLocation(e->shader, "particles");
  e->spreadAreaLoc = GetShaderLocation(e->shader, "spreadArea");
  e->yBiasLoc = GetShaderLocation(e->shader, "yBias");
  e->rocketTimeLoc = GetShaderLocation(e->shader, "rocketTime");
  e->explodeTimeLoc = GetShaderLocation(e->shader, "explodeTime");
  e->pauseTimeLoc = GetShaderLocation(e->shader, "pauseTime");
  e->gravityLoc = GetShaderLocation(e->shader, "gravity");
  e->burstSpeedLoc = GetShaderLocation(e->shader, "burstSpeed");
  e->rocketSpeedLoc = GetShaderLocation(e->shader, "rocketSpeed");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->particleSizeLoc = GetShaderLocation(e->shader, "particleSize");
  e->glowSharpnessLoc = GetShaderLocation(e->shader, "glowSharpness");
  e->sparkleSpeedLoc = GetShaderLocation(e->shader, "sparkleSpeed");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
}

static void InitPingPong(FireworksEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "FIREWORKS");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "FIREWORKS");
}

static void UnloadPingPong(FireworksEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool FireworksEffectInit(FireworksEffect *e, const FireworksConfig *cfg,
                         int width, int height) {
  e->shader = LoadShader(NULL, "shaders/fireworks.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  InitPingPong(e, width, height);
  e->readIdx = 0;
  e->time = 0.0f;

  return true;
}

void FireworksEffectSetup(FireworksEffect *e, const FireworksConfig *cfg,
                          float deltaTime, int screenWidth, int screenHeight) {
  e->time += deltaTime;

  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  float decayFactor = expf(-0.693147f * deltaTime / cfg->decayHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);

  int maxBursts = cfg->maxBursts;
  SetShaderValue(e->shader, e->maxBurstsLoc, &maxBursts, SHADER_UNIFORM_INT);
  int particles = cfg->particles;
  SetShaderValue(e->shader, e->particlesLoc, &particles, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->spreadAreaLoc, &cfg->spreadArea,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->yBiasLoc, &cfg->yBias, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->rocketTimeLoc, &cfg->rocketTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->explodeTimeLoc, &cfg->explodeTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pauseTimeLoc, &cfg->pauseTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gravityLoc, &cfg->gravity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->burstSpeedLoc, &cfg->burstSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rocketSpeedLoc, &cfg->rocketSpeed,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->particleSizeLoc, &cfg->particleSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowSharpnessLoc, &cfg->glowSharpness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sparkleSpeedLoc, &cfg->sparkleSpeed,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
}

void FireworksEffectRender(FireworksEffect *e, const FireworksConfig *cfg,
                           float deltaTime, int screenWidth, int screenHeight,
                           Texture2D fftTexture) {
  (void)cfg;
  (void)deltaTime;

  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  // Texture bindings use raylib's activeTextureId[] which resets on every batch
  // flush. They MUST be set after BeginTextureMode/BeginShaderMode (both
  // flush).
  SetShaderValueTexture(e->shader, e->previousFrameLoc,
                        e->pingPong[e->readIdx].texture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void FireworksEffectResize(FireworksEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void FireworksEffectUninit(FireworksEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

FireworksConfig FireworksConfigDefault(void) { return FireworksConfig{}; }

void FireworksRegisterParams(FireworksConfig *cfg) {
  ModEngineRegisterParam("fireworks.spreadArea", &cfg->spreadArea, 0.1f, 1.0f);
  ModEngineRegisterParam("fireworks.yBias", &cfg->yBias, -0.5f, 0.5f);
  ModEngineRegisterParam("fireworks.rocketTime", &cfg->rocketTime, 0.3f, 2.0f);
  ModEngineRegisterParam("fireworks.explodeTime", &cfg->explodeTime, 0.3f,
                         2.0f);
  ModEngineRegisterParam("fireworks.pauseTime", &cfg->pauseTime, 0.0f, 2.0f);
  ModEngineRegisterParam("fireworks.gravity", &cfg->gravity, 0.0f, 20.0f);
  ModEngineRegisterParam("fireworks.burstSpeed", &cfg->burstSpeed, 5.0f, 20.0f);
  ModEngineRegisterParam("fireworks.rocketSpeed", &cfg->rocketSpeed, 2.0f,
                         12.0f);
  ModEngineRegisterParam("fireworks.glowIntensity", &cfg->glowIntensity, 0.1f,
                         3.0f);
  ModEngineRegisterParam("fireworks.particleSize", &cfg->particleSize, 0.01f,
                         0.1f);
  ModEngineRegisterParam("fireworks.decayHalfLife", &cfg->decayHalfLife, 0.05f,
                         2.0f);
  ModEngineRegisterParam("fireworks.glowSharpness", &cfg->glowSharpness, 1.0f,
                         3.0f);
  ModEngineRegisterParam("fireworks.sparkleSpeed", &cfg->sparkleSpeed, 5.0f,
                         40.0f);
  ModEngineRegisterParam("fireworks.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("fireworks.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("fireworks.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("fireworks.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("fireworks.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("fireworks.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupFireworks(PostEffect *pe) {
  FireworksEffectSetup(&pe->fireworks, &pe->effects.fireworks,
                       pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
}

void SetupFireworksBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor,
                       pe->fireworks.pingPong[pe->fireworks.readIdx].texture,
                       pe->effects.fireworks.blendIntensity,
                       pe->effects.fireworks.blendMode);
}

void RenderFireworks(PostEffect *pe) {
  FireworksEffectRender(&pe->fireworks, &pe->effects.fireworks,
                        pe->currentDeltaTime, pe->screenWidth, pe->screenHeight,
                        pe->fftTexture);
}

// === UI ===

static void DrawFireworksParams(EffectConfig *e, const ModSources *modSources,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  FireworksConfig *fw = &e->fireworks;

  // Burst
  ImGui::SeparatorText("Burst");
  ImGui::SliderInt("Bursts##fireworks", &fw->maxBursts, 1, 8);
  ImGui::SliderInt("Particles##fireworks", &fw->particles, 10, 60);
  ModulatableSlider("Spread##fireworks", &fw->spreadArea,
                    "fireworks.spreadArea", "%.2f", modSources);
  ModulatableSlider("Y Bias##fireworks", &fw->yBias, "fireworks.yBias", "%.2f",
                    modSources);

  // Timing
  ImGui::SeparatorText("Timing");
  ModulatableSlider("Rocket Time##fireworks", &fw->rocketTime,
                    "fireworks.rocketTime", "%.2f", modSources);
  ModulatableSlider("Explode Time##fireworks", &fw->explodeTime,
                    "fireworks.explodeTime", "%.2f", modSources);
  ModulatableSlider("Pause Time##fireworks", &fw->pauseTime,
                    "fireworks.pauseTime", "%.2f", modSources);

  // Physics
  ImGui::SeparatorText("Physics");
  ModulatableSlider("Gravity##fireworks", &fw->gravity, "fireworks.gravity",
                    "%.1f", modSources);
  ModulatableSlider("Burst Speed##fireworks", &fw->burstSpeed,
                    "fireworks.burstSpeed", "%.1f", modSources);
  ModulatableSlider("Rocket Speed##fireworks", &fw->rocketSpeed,
                    "fireworks.rocketSpeed", "%.1f", modSources);

  // Visual
  ImGui::SeparatorText("Visual");
  ModulatableSlider("Glow Intensity##fireworks", &fw->glowIntensity,
                    "fireworks.glowIntensity", "%.2f", modSources);
  ModulatableSlider("Particle Size##fireworks", &fw->particleSize,
                    "fireworks.particleSize", "%.3f", modSources);
  ModulatableSlider("Sharpness##fireworks", &fw->glowSharpness,
                    "fireworks.glowSharpness", "%.2f", modSources);
  ModulatableSlider("Sparkle Speed##fireworks", &fw->sparkleSpeed,
                    "fireworks.sparkleSpeed", "%.1f", modSources);
  ModulatableSlider("Decay##fireworks", &fw->decayHalfLife,
                    "fireworks.decayHalfLife", "%.2f", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##fireworks", &fw->baseFreq,
                    "fireworks.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##fireworks", &fw->maxFreq,
                    "fireworks.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##fireworks", &fw->gain, "fireworks.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##fireworks", &fw->curve, "fireworks.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##fireworks", &fw->baseBright,
                    "fireworks.baseBright", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(fireworks)
REGISTER_GENERATOR_FULL(TRANSFORM_FIREWORKS_BLEND, Fireworks,
                        fireworks, "Fireworks",
                        SetupFireworksBlend, SetupFireworks,
                        RenderFireworks, 15, DrawFireworksParams, DrawOutput_fireworks)
// clang-format on
