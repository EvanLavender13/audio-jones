// SpiralNest effect module implementation
// Nested spiraling fractal arms with FFT-driven glow and configurable zoom

#include "spiral_nest.h"
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
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

bool SpiralNestEffectInit(SpiralNestEffect *e, const SpiralNestConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/spiral_nest.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
  e->timeAccumLoc = GetShaderLocation(e->shader, "timeAccum");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->timeAccum = 0.0f;

  return true;
}

void SpiralNestEffectSetup(SpiralNestEffect *e, const SpiralNestConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture) {
  e->timeAccum += cfg->animSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeAccumLoc, &e->timeAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SpiralNestEffectUninit(SpiralNestEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void SpiralNestRegisterParams(SpiralNestConfig *cfg) {
  ModEngineRegisterParam("spiralNest.zoom", &cfg->zoom, 10.0f, 400.0f);
  ModEngineRegisterParam("spiralNest.animSpeed", &cfg->animSpeed, 0.01f, 1.0f);
  ModEngineRegisterParam("spiralNest.glowIntensity", &cfg->glowIntensity, 0.5f,
                         10.0f);
  ModEngineRegisterParam("spiralNest.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("spiralNest.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("spiralNest.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("spiralNest.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("spiralNest.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("spiralNest.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSpiralNest(PostEffect *pe) {
  SpiralNestEffectSetup(&pe->spiralNest, &pe->effects.spiralNest,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupSpiralNestBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.spiralNest.blendIntensity,
                       pe->effects.spiralNest.blendMode);
}

// === UI ===

static void DrawSpiralNestParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  SpiralNestConfig *cfg = &e->spiralNest;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##spiralNest", &cfg->baseFreq,
                    "spiralNest.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##spiralNest", &cfg->maxFreq,
                    "spiralNest.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##spiralNest", &cfg->gain, "spiralNest.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##spiralNest", &cfg->curve, "spiralNest.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##spiralNest", &cfg->baseBright,
                    "spiralNest.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Zoom##spiralNest", &cfg->zoom, "spiralNest.zoom", "%.0f",
                    modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Anim Speed##spiralNest", &cfg->animSpeed,
                    "spiralNest.animSpeed", "%.3f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##spiralNest", &cfg->glowIntensity,
                    "spiralNest.glowIntensity", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(spiralNest)
REGISTER_GENERATOR(TRANSFORM_SPIRAL_NEST_BLEND, SpiralNest, spiralNest,
                   "Spiral Nest", SetupSpiralNestBlend, SetupSpiralNest, 10,
                   DrawSpiralNestParams, DrawOutput_spiralNest)
// clang-format on
