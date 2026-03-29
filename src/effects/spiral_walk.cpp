// SpiralWalk effect module implementation
// Spiraling chain of glowing segments with per-segment FFT brightness

#include "spiral_walk.h"
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
#include <stddef.h>

bool SpiralWalkEffectInit(SpiralWalkEffect *e, const SpiralWalkConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/spiral_walk.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->segmentsLoc = GetShaderLocation(e->shader, "segments");
  e->growthRateLoc = GetShaderLocation(e->shader, "growthRate");
  e->angleOffsetLoc = GetShaderLocation(e->shader, "angleOffset");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
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

  e->rotationAccum = 0.0f;

  return true;
}

void SpiralWalkEffectSetup(SpiralWalkEffect *e, const SpiralWalkConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->segmentsLoc, &cfg->segments, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->growthRateLoc, &cfg->growthRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->angleOffsetLoc, &cfg->angleOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
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

void SpiralWalkEffectUninit(SpiralWalkEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void SpiralWalkRegisterParams(SpiralWalkConfig *cfg) {
  ModEngineRegisterParam("spiralWalk.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("spiralWalk.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("spiralWalk.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("spiralWalk.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("spiralWalk.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("spiralWalk.growthRate", &cfg->growthRate, 0.005f,
                         0.1f);
  ModEngineRegisterParam("spiralWalk.angleOffset", &cfg->angleOffset,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("spiralWalk.glowIntensity", &cfg->glowIntensity, 0.5f,
                         10.0f);
  ModEngineRegisterParam("spiralWalk.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("spiralWalk.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSpiralWalk(PostEffect *pe) {
  SpiralWalkEffectSetup(&pe->spiralWalk, &pe->effects.spiralWalk,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupSpiralWalkBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.spiralWalk.blendIntensity,
                       pe->effects.spiralWalk.blendMode);
}

// === UI ===

static void DrawSpiralWalkParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  SpiralWalkConfig *cfg = &e->spiralWalk;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##spiralWalk", &cfg->baseFreq,
                    "spiralWalk.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##spiralWalk", &cfg->maxFreq,
                    "spiralWalk.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##spiralWalk", &cfg->gain, "spiralWalk.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##spiralWalk", &cfg->curve, "spiralWalk.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##spiralWalk", &cfg->baseBright,
                    "spiralWalk.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Segments##spiralWalk", &cfg->segments, 10, 200);
  ModulatableSlider("Growth Rate##spiralWalk", &cfg->growthRate,
                    "spiralWalk.growthRate", "%.3f", modSources);
  ModulatableSliderAngleDeg("Angle Offset##spiralWalk", &cfg->angleOffset,
                            "spiralWalk.angleOffset", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##spiralWalk", &cfg->rotationSpeed,
                            "spiralWalk.rotationSpeed", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##spiralWalk", &cfg->glowIntensity,
                    "spiralWalk.glowIntensity", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(spiralWalk)
REGISTER_GENERATOR(TRANSFORM_SPIRAL_WALK_BLEND, SpiralWalk, spiralWalk,
                   "Spiral Walk", SetupSpiralWalkBlend, SetupSpiralWalk, 10,
                   DrawSpiralWalkParams, DrawOutput_spiralWalk)
// clang-format on
