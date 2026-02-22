// Bit crush effect module implementation
// Lattice-based pixelation with FFT-driven glow and iterative folding

#include "bit_crush.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool BitCrushEffectInit(BitCrushEffect *e, const BitCrushConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/bit_crush.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->cellSizeLoc = GetShaderLocation(e->shader, "cellSize");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->walkModeLoc = GetShaderLocation(e->shader, "walkMode");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void BitCrushEffectSetup(BitCrushEffect *e, const BitCrushConfig *cfg,
                         float deltaTime, Texture2D fftTexture) {
  e->time += cfg->speed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  float center[2] = {0.5f, 0.5f};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

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
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellSizeLoc, &cfg->cellSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->walkModeLoc, &cfg->walkMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void BitCrushEffectUninit(BitCrushEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

BitCrushConfig BitCrushConfigDefault(void) { return BitCrushConfig{}; }

void BitCrushRegisterParams(BitCrushConfig *cfg) {
  ModEngineRegisterParam("bitCrush.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("bitCrush.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("bitCrush.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("bitCrush.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("bitCrush.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("bitCrush.scale", &cfg->scale, 0.05f, 1.0f);
  ModEngineRegisterParam("bitCrush.cellSize", &cfg->cellSize, 2.0f, 32.0f);
  ModEngineRegisterParam("bitCrush.speed", &cfg->speed, 0.1f, 5.0f);
  ModEngineRegisterParam("bitCrush.glowIntensity", &cfg->glowIntensity, 0.0f,
                         3.0f);
  ModEngineRegisterParam("bitCrush.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupBitCrush(PostEffect *pe) {
  BitCrushEffectSetup(&pe->bitCrush, &pe->effects.bitCrush,
                      pe->currentDeltaTime, pe->fftTexture);
}

void SetupBitCrushBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.bitCrush.blendIntensity,
                       pe->effects.bitCrush.blendMode);
}

// === UI ===

static const char *WALK_MODE_NAMES[] = {"Fixed Dir",       "Rotating Dir",
                                        "Offset Neighbor", "Alternating Snap",
                                        "Cross-Coupled",   "Asymmetric Hash"};
static const int WALK_MODE_COUNT = 6;

static void DrawBitCrushParams(EffectConfig *e, const ModSources *modSources,
                               ImU32 categoryGlow) {
  BitCrushConfig *cfg = &e->bitCrush;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##bitcrush", &cfg->baseFreq,
                    "bitCrush.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##bitcrush", &cfg->maxFreq,
                    "bitCrush.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##bitcrush", &cfg->gain, "bitCrush.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##bitcrush", &cfg->curve, "bitCrush.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##bitcrush", &cfg->baseBright,
                    "bitCrush.baseBright", "%.2f", modSources);

  // Lattice
  ImGui::SeparatorText("Lattice");
  ModulatableSlider("Scale##bitcrush", &cfg->scale, "bitCrush.scale", "%.2f",
                    modSources);
  ModulatableSlider("Cell Size##bitcrush", &cfg->cellSize, "bitCrush.cellSize",
                    "%.1f", modSources);
  ModulatableSlider("Speed##bitcrush", &cfg->speed, "bitCrush.speed", "%.2f",
                    modSources);
  ImGui::SliderInt("Iterations##bitcrush", &cfg->iterations, 4, 64);
  ImGui::Combo("Walk Mode##bitcrush", &cfg->walkMode, WALK_MODE_NAMES,
               WALK_MODE_COUNT);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##bitcrush", &cfg->glowIntensity,
                    "bitCrush.glowIntensity", "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(bitCrush)
REGISTER_GENERATOR(TRANSFORM_BIT_CRUSH_BLEND, BitCrush, bitCrush,
                   "Bit Crush", SetupBitCrushBlend,
                   SetupBitCrush, 12, DrawBitCrushParams, DrawOutput_bitCrush)
// clang-format on
