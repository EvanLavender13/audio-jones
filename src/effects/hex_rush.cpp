// Hex rush effect module implementation
// FFT-driven concentric polygon walls rushing inward with gap patterns,
// perspective distortion, and gradient coloring

#include "hex_rush.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

static float Fract(float x) { return x - floorf(x); }

bool HexRushEffectInit(HexRushEffect *e, const HexRushConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/hex_rush.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->sidesLoc = GetShaderLocation(e->shader, "sides");
  e->centerSizeLoc = GetShaderLocation(e->shader, "centerSize");
  e->wallThicknessLoc = GetShaderLocation(e->shader, "wallThickness");
  e->wallSpacingLoc = GetShaderLocation(e->shader, "wallSpacing");
  e->ringBufferLoc = GetShaderLocation(e->shader, "ringBuffer");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->pulseAmountLoc = GetShaderLocation(e->shader, "pulseAmount");
  e->pulseAccumLoc = GetShaderLocation(e->shader, "pulseAccum");
  e->perspectiveLoc = GetShaderLocation(e->shader, "perspective");
  e->bgContrastLoc = GetShaderLocation(e->shader, "bgContrast");
  e->colorAccumLoc = GetShaderLocation(e->shader, "colorAccum");
  e->wallGlowLoc = GetShaderLocation(e->shader, "wallGlow");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->wallAccumLoc = GetShaderLocation(e->shader, "wallAccum");
  e->wobbleTimeLoc = GetShaderLocation(e->shader, "wobbleTime");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  for (int i = 0; i < 256; i++) {
    const int idx = i * 4;
    e->ringBuffer[idx] = cfg->gapChance;
    e->ringBuffer[idx + 1] = cfg->patternSeed;
    e->ringBuffer[idx + 2] = 0.0f;
    e->ringBuffer[idx + 3] = 0.0f;
  }
  const Image img = {e->ringBuffer, 256, 1, 1,
                     PIXELFORMAT_UNCOMPRESSED_R32G32B32A32};
  e->ringBufferTex = LoadTextureFromImage(img);
  if (e->ringBufferTex.id == 0) {
    ColorLUTUninit(e->gradientLUT);
    UnloadShader(e->shader);
    return false;
  }
  SetTextureFilter(e->ringBufferTex, TEXTURE_FILTER_POINT);
  e->lastFilledRing = -1;

  e->rotationAccum = 0.0f;
  e->flipAccum = 0.0f;
  e->rotationDir = 1.0f;
  e->pulseAccum = 0.0f;
  e->wallAccum = 0.0f;
  e->colorAccum = 0.0f;
  e->wobbleTime = 0.0f;

  return true;
}

void HexRushEffectSetup(HexRushEffect *e, const HexRushConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture) {
  // Flip logic: toggle rotation direction on cycle boundary
  const float prevFlipAccum = e->flipAccum;
  e->flipAccum += cfg->flipRate * deltaTime;
  if (cfg->flipRate > 0.0f && Fract(e->flipAccum) < Fract(prevFlipAccum)) {
    e->rotationDir *= -1.0f;
  }

  e->rotationAccum += cfg->rotationSpeed * e->rotationDir * deltaTime;

  // Pulse and time accumulators
  e->pulseAccum += cfg->pulseSpeed * 6.283185f * deltaTime;
  e->wallAccum += cfg->wallSpeed * deltaTime;

  const int maxRing =
      (int)floorf((1.5f * 10.0f + e->wallAccum) / cfg->wallSpacing);
  int startRing = e->lastFilledRing + 1;
  if (maxRing - startRing > 256) {
    startRing = maxRing - 256;
  }
  for (int ring = startRing; ring <= maxRing; ring++) {
    const int idx = (ring & 255) * 4;
    e->ringBuffer[idx] = cfg->gapChance;
    e->ringBuffer[idx + 1] = cfg->patternSeed;
  }
  if (maxRing > e->lastFilledRing) {
    e->lastFilledRing = maxRing;
    UpdateTexture(e->ringBufferTex, e->ringBuffer);
  }

  e->colorAccum += cfg->colorSpeed * deltaTime;
  e->wobbleTime += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

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
  SetShaderValue(e->shader, e->sidesLoc, &cfg->sides, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->centerSizeLoc, &cfg->centerSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wallThicknessLoc, &cfg->wallThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wallSpacingLoc, &cfg->wallSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulseAmountLoc, &cfg->pulseAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulseAccumLoc, &e->pulseAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->perspectiveLoc, &cfg->perspective,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bgContrastLoc, &cfg->bgContrast,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorAccumLoc, &e->colorAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wallGlowLoc, &cfg->wallGlow,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wallAccumLoc, &e->wallAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wobbleTimeLoc, &e->wobbleTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->ringBufferLoc, e->ringBufferTex);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void HexRushEffectUninit(HexRushEffect *e) {
  UnloadTexture(e->ringBufferTex);
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void HexRushRegisterParams(HexRushConfig *cfg) {
  ModEngineRegisterParam("hexRush.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("hexRush.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("hexRush.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("hexRush.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("hexRush.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("hexRush.wallSpeed", &cfg->wallSpeed, 0.5f, 10.0f);
  ModEngineRegisterParam("hexRush.wallSpacing", &cfg->wallSpacing, 0.2f, 2.0f);
  ModEngineRegisterParam("hexRush.wallThickness", &cfg->wallThickness, 0.02f,
                         0.6f);
  ModEngineRegisterParam("hexRush.wallGlow", &cfg->wallGlow, 0.0f, 2.0f);
  ModEngineRegisterParam("hexRush.gapChance", &cfg->gapChance, 0.1f, 0.99f);
  ModEngineRegisterParam("hexRush.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("hexRush.pulseSpeed", &cfg->pulseSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("hexRush.pulseAmount", &cfg->pulseAmount, 0.0f, 0.5f);
  ModEngineRegisterParam("hexRush.patternSeed", &cfg->patternSeed, 0.0f,
                         100.0f);
  ModEngineRegisterParam("hexRush.perspective", &cfg->perspective, 0.0f, 1.0f);
  ModEngineRegisterParam("hexRush.colorSpeed", &cfg->colorSpeed, 0.0f, 0.5f);
  ModEngineRegisterParam("hexRush.glowIntensity", &cfg->glowIntensity, 0.1f,
                         3.0f);
  ModEngineRegisterParam("hexRush.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupHexRush(PostEffect *pe) {
  HexRushEffectSetup(&pe->hexRush, &pe->effects.hexRush, pe->currentDeltaTime,
                     pe->fftTexture);
}

void SetupHexRushBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.hexRush.blendIntensity,
                       pe->effects.hexRush.blendMode);
}

// === UI ===

static void DrawHexRushParams(EffectConfig *e, const ModSources *modSources,
                              ImU32 categoryGlow) {
  (void)categoryGlow;
  HexRushConfig *cfg = &e->hexRush;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##hexrush", &cfg->baseFreq,
                    "hexRush.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##hexrush", &cfg->maxFreq, "hexRush.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##hexrush", &cfg->gain, "hexRush.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##hexrush", &cfg->curve, "hexRush.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##hexrush", &cfg->baseBright,
                    "hexRush.baseBright", "%.2f", modSources);
  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Sides##hexrush", &cfg->sides, 3, 12);
  ImGui::SliderFloat("Center Size##hexrush", &cfg->centerSize, 0.05f, 0.5f,
                     "%.2f");
  ModulatableSlider("Wall Thickness##hexrush", &cfg->wallThickness,
                    "hexRush.wallThickness", "%.2f", modSources);
  ModulatableSlider("Wall Spacing##hexrush", &cfg->wallSpacing,
                    "hexRush.wallSpacing", "%.2f", modSources);

  // Dynamics
  ImGui::SeparatorText("Dynamics");
  ModulatableSlider("Wall Speed##hexrush", &cfg->wallSpeed, "hexRush.wallSpeed",
                    "%.1f", modSources);
  ModulatableSlider("Gap Chance##hexrush", &cfg->gapChance, "hexRush.gapChance",
                    "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rotation Speed##hexrush", &cfg->rotationSpeed,
                            "hexRush.rotationSpeed", modSources);
  ImGui::SliderFloat("Flip Rate##hexrush", &cfg->flipRate, 0.0f, 1.0f, "%.2f");
  ModulatableSlider("Pulse Speed##hexrush", &cfg->pulseSpeed,
                    "hexRush.pulseSpeed", "%.1f", modSources);
  ModulatableSlider("Pulse Amount##hexrush", &cfg->pulseAmount,
                    "hexRush.pulseAmount", "%.2f", modSources);
  ModulatableSlider("Pattern Seed##hexrush", &cfg->patternSeed,
                    "hexRush.patternSeed", "%.1f", modSources);

  // Visual
  ImGui::SeparatorText("Visual");
  ModulatableSlider("Perspective##hexrush", &cfg->perspective,
                    "hexRush.perspective", "%.2f", modSources);
  ImGui::SliderFloat("BG Contrast##hexrush", &cfg->bgContrast, 0.0f, 1.0f,
                     "%.2f");
  ModulatableSlider("Color Speed##hexrush", &cfg->colorSpeed,
                    "hexRush.colorSpeed", "%.3f", modSources);
  ModulatableSlider("Wall Glow##hexrush", &cfg->wallGlow, "hexRush.wallGlow",
                    "%.2f", modSources);
  ModulatableSlider("Glow Intensity##hexrush", &cfg->glowIntensity,
                    "hexRush.glowIntensity", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(hexRush)
REGISTER_GENERATOR(TRANSFORM_HEX_RUSH_BLEND, HexRush, hexRush,
                   "Hex Rush", SetupHexRushBlend,
                   SetupHexRush, 10, DrawHexRushParams, DrawOutput_hexRush)
// clang-format on
