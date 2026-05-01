// Star trail effect module implementation
// Orbiting stars with persistent luminous trails and per-star FFT reactivity

#include "star_trail.h"
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

static void InitPingPong(StarTrailEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "STAR_TRAIL");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "STAR_TRAIL");
}

static void UnloadPingPong(const StarTrailEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool StarTrailEffectInit(StarTrailEffect *e, const StarTrailConfig *cfg,
                         int width, int height) {
  e->shader = LoadShader(NULL, "shaders/star_trail.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->variationTimeLoc = GetShaderLocation(e->shader, "variationTime");
  e->previousFrameLoc = GetShaderLocation(e->shader, "previousFrame");
  e->starCountLoc = GetShaderLocation(e->shader, "starCount");
  e->freqMapModeLoc = GetShaderLocation(e->shader, "freqMapMode");
  e->spreadRadiusLoc = GetShaderLocation(e->shader, "spreadRadius");
  e->speedWavinessLoc = GetShaderLocation(e->shader, "speedWaviness");
  e->dotSizeLoc = GetShaderLocation(e->shader, "spriteRadius");
  e->sharpnessLoc = GetShaderLocation(e->shader, "sharpness");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
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
  e->variationTime = 0.0f;

  return true;
}

void StarTrailEffectSetup(StarTrailEffect *e, const StarTrailConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture) {
  e->time += cfg->orbitSpeed * deltaTime;
  e->variationTime += cfg->speedVariation * deltaTime;
  e->currentFFTTexture = fftTexture;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->variationTimeLoc, &e->variationTime,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->starCountLoc, &cfg->starCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->freqMapModeLoc, &cfg->freqMapMode,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->spreadRadiusLoc, &cfg->spreadRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->speedWavinessLoc, &cfg->speedWaviness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dotSizeLoc, &cfg->dotSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sharpnessLoc, &cfg->sharpness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);

  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  const float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);

  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
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

void StarTrailEffectRender(StarTrailEffect *e, const StarTrailConfig *cfg,
                           float deltaTime, int screenWidth, int screenHeight) {
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

void StarTrailEffectResize(StarTrailEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void StarTrailEffectUninit(StarTrailEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

void StarTrailRegisterParams(StarTrailConfig *cfg) {
  ModEngineRegisterParam("starTrail.spreadRadius", &cfg->spreadRadius, 0.2f,
                         2.0f);
  ModEngineRegisterParam("starTrail.orbitSpeed", &cfg->orbitSpeed, -2.0f, 2.0f);
  ModEngineRegisterParam("starTrail.speedWaviness", &cfg->speedWaviness, 0.0f,
                         20.0f);
  ModEngineRegisterParam("starTrail.speedVariation", &cfg->speedVariation, 0.0f,
                         0.5f);
  ModEngineRegisterParam("starTrail.dotSize", &cfg->dotSize, 0.001f, 0.03f);
  ModEngineRegisterParam("starTrail.sharpness", &cfg->sharpness, 0.0f, 1.0f);
  ModEngineRegisterParam("starTrail.glowIntensity", &cfg->glowIntensity, 0.1f,
                         3.0f);
  ModEngineRegisterParam("starTrail.decayHalfLife", &cfg->decayHalfLife, 0.1f,
                         10.0f);
  ModEngineRegisterParam("starTrail.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("starTrail.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("starTrail.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("starTrail.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("starTrail.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("starTrail.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

StarTrailEffect *GetStarTrailEffect(PostEffect *pe) {
  return (StarTrailEffect *)pe->effectStates[TRANSFORM_STAR_TRAIL_BLEND];
}

void SetupStarTrail(PostEffect *pe) {
  StarTrailEffectSetup(GetStarTrailEffect(pe), &pe->effects.starTrail,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupStarTrailBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor,
      GetStarTrailEffect(pe)->pingPong[GetStarTrailEffect(pe)->readIdx].texture,
      pe->effects.starTrail.blendIntensity, pe->effects.starTrail.blendMode);
}

void RenderStarTrail(PostEffect *pe) {
  StarTrailEffectRender(GetStarTrailEffect(pe), &pe->effects.starTrail,
                        pe->currentDeltaTime, pe->screenWidth,
                        pe->screenHeight);
}

// === UI ===

static void DrawStarTrailParams(EffectConfig *e, const ModSources *modSources,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  StarTrailConfig *s = &e->starTrail;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##starTrail", &s->baseFreq,
                    "starTrail.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##starTrail", &s->maxFreq,
                    "starTrail.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##starTrail", &s->gain, "starTrail.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##starTrail", &s->curve, "starTrail.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##starTrail", &s->baseBright,
                    "starTrail.baseBright", "%.2f", modSources);

  // Stars
  ImGui::SeparatorText("Stars");
  ImGui::SliderInt("Stars##starTrail", &s->starCount, 50, 1000);
  const char *freqMapLabels[] = {"Radius", "Hash", "Angle"};
  ImGui::Combo("Freq Mapping##starTrail", &s->freqMapMode, freqMapLabels,
               IM_ARRAYSIZE(freqMapLabels));
  ModulatableSlider("Spread Radius##starTrail", &s->spreadRadius,
                    "starTrail.spreadRadius", "%.2f", modSources);

  // Orbit
  ImGui::SeparatorText("Orbit");
  ModulatableSlider("Orbit Speed##starTrail", &s->orbitSpeed,
                    "starTrail.orbitSpeed", "%.2f", modSources);
  ModulatableSlider("Speed Waviness##starTrail", &s->speedWaviness,
                    "starTrail.speedWaviness", "%.1f", modSources);
  ModulatableSlider("Speed Variation##starTrail", &s->speedVariation,
                    "starTrail.speedVariation", "%.3f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSliderLog("Dot Size##starTrail", &s->dotSize, "starTrail.dotSize",
                       "%.4f", modSources);
  ModulatableSlider("Sharpness##starTrail", &s->sharpness,
                    "starTrail.sharpness", "%.2f", modSources);
  ModulatableSlider("Glow Intensity##starTrail", &s->glowIntensity,
                    "starTrail.glowIntensity", "%.2f", modSources);
  ModulatableSlider("Decay Half-Life##starTrail", &s->decayHalfLife,
                    "starTrail.decayHalfLife", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(starTrail)
REGISTER_GENERATOR_FULL(TRANSFORM_STAR_TRAIL_BLEND, StarTrail, starTrail,
                        "Star Trail", SetupStarTrailBlend, SetupStarTrail,
                        RenderStarTrail, 10, DrawStarTrailParams,
                        DrawOutput_starTrail)
// clang-format on
