// Rainbow Road effect module implementation
// Receding frequency bars with sway, curvature, and glow

#include "rainbow_road.h"
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

bool RainbowRoadEffectInit(RainbowRoadEffect *e, const RainbowRoadConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/rainbow_road.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->scrollLoc = GetShaderLocation(e->shader, "scroll");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->directionLoc = GetShaderLocation(e->shader, "direction");
  e->widthLoc = GetShaderLocation(e->shader, "width");
  e->swayLoc = GetShaderLocation(e->shader, "sway");
  e->curvatureLoc = GetShaderLocation(e->shader, "curvature");
  e->phaseSpreadLoc = GetShaderLocation(e->shader, "phaseSpread");
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

  e->time = 0.0f;
  e->scroll = 0.0f;

  return true;
}

void RainbowRoadEffectSetup(RainbowRoadEffect *e, const RainbowRoadConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture) {
  e->time += deltaTime;
  e->scroll += deltaTime * cfg->speed;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  float scrollFrac = e->scroll - (float)(int)e->scroll;
  if (scrollFrac < 0.0f) {
    scrollFrac += 1.0f;
  }
  SetShaderValue(e->shader, e->scrollLoc, &scrollFrac, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->directionLoc, &cfg->direction,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->widthLoc, &cfg->width, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->swayLoc, &cfg->sway, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curvatureLoc, &cfg->curvature,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseSpreadLoc, &cfg->phaseSpread,
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

void RainbowRoadEffectUninit(RainbowRoadEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void RainbowRoadRegisterParams(RainbowRoadConfig *cfg) {
  ModEngineRegisterParam("rainbowRoad.width", &cfg->width, 0.5f, 8.0f);
  ModEngineRegisterParam("rainbowRoad.sway", &cfg->sway, 0.0f, 3.0f);
  ModEngineRegisterParam("rainbowRoad.curvature", &cfg->curvature, 0.0f, 1.0f);
  ModEngineRegisterParam("rainbowRoad.phaseSpread", &cfg->phaseSpread, 0.1f,
                         3.0f);
  ModEngineRegisterParam("rainbowRoad.speed", &cfg->speed, -3.0f, 3.0f);
  ModEngineRegisterParam("rainbowRoad.glowIntensity", &cfg->glowIntensity, 0.1f,
                         5.0f);
  ModEngineRegisterParam("rainbowRoad.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("rainbowRoad.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("rainbowRoad.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("rainbowRoad.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("rainbowRoad.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("rainbowRoad.blendIntensity", &cfg->blendIntensity,
                         0.0f, 1.0f);
}

RainbowRoadEffect *GetRainbowRoadEffect(PostEffect *pe) {
  return (RainbowRoadEffect *)pe->effectStates[TRANSFORM_RAINBOW_ROAD_BLEND];
}

void SetupRainbowRoad(PostEffect *pe) {
  RainbowRoadEffectSetup(GetRainbowRoadEffect(pe), &pe->effects.rainbowRoad,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupRainbowRoadBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.rainbowRoad.blendIntensity,
                       pe->effects.rainbowRoad.blendMode);
}

// === UI ===

static void DrawRainbowRoadParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  RainbowRoadConfig *cfg = &e->rainbowRoad;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##rainbowRoad", &cfg->baseFreq,
                    "rainbowRoad.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##rainbowRoad", &cfg->maxFreq,
                    "rainbowRoad.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##rainbowRoad", &cfg->gain, "rainbowRoad.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##rainbowRoad", &cfg->curve, "rainbowRoad.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##rainbowRoad", &cfg->baseBright,
                    "rainbowRoad.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Layers##rainbowRoad", &cfg->layers, 4, 64);
  ImGui::Combo("Direction##rainbowRoad", &cfg->direction, "Up\0Down\0");
  ModulatableSlider("Width##rainbowRoad", &cfg->width, "rainbowRoad.width",
                    "%.2f", modSources);
  ModulatableSlider("Sway##rainbowRoad", &cfg->sway, "rainbowRoad.sway", "%.2f",
                    modSources);
  ModulatableSlider("Curvature##rainbowRoad", &cfg->curvature,
                    "rainbowRoad.curvature", "%.2f", modSources);
  ModulatableSlider("Phase Spread##rainbowRoad", &cfg->phaseSpread,
                    "rainbowRoad.phaseSpread", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Speed##rainbowRoad", &cfg->speed, "rainbowRoad.speed",
                    "%.2f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##rainbowRoad", &cfg->glowIntensity,
                    "rainbowRoad.glowIntensity", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(rainbowRoad)
REGISTER_GENERATOR(TRANSFORM_RAINBOW_ROAD_BLEND, RainbowRoad, rainbowRoad,
                   "Rainbow Road", SetupRainbowRoadBlend, SetupRainbowRoad, 11,
                   DrawRainbowRoadParams, DrawOutput_rainbowRoad)
// clang-format on
