// Interference effect module implementation
// Overlapping wave emitters create ripple patterns via constructive/destructive
// interference

#include "interference.h"
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

bool InterferenceEffectInit(InterferenceEffect *e,
                            const InterferenceConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/interference.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->sourcesLoc = GetShaderLocation(e->shader, "sources");
  e->phasesLoc = GetShaderLocation(e->shader, "phases");
  e->sourceCountLoc = GetShaderLocation(e->shader, "sourceCount");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->falloffTypeLoc = GetShaderLocation(e->shader, "falloffType");
  e->falloffStrengthLoc = GetShaderLocation(e->shader, "falloffStrength");
  e->boundariesLoc = GetShaderLocation(e->shader, "boundaries");
  e->reflectionGainLoc = GetShaderLocation(e->shader, "reflectionGain");
  e->visualModeLoc = GetShaderLocation(e->shader, "visualMode");
  e->contourCountLoc = GetShaderLocation(e->shader, "contourCount");
  e->visualGainLoc = GetShaderLocation(e->shader, "visualGain");
  e->chromaSpreadLoc = GetShaderLocation(e->shader, "chromaSpread");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->colorLUTLoc = GetShaderLocation(e->shader, "colorLUT");

  e->colorLUT = ColorLUTInit(&cfg->gradient);
  if (e->colorLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

static void SetupWaveParams(InterferenceEffect *e,
                            const InterferenceConfig *cfg) {
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->falloffTypeLoc, &cfg->falloffType,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffStrengthLoc, &cfg->falloffStrength,
                 SHADER_UNIFORM_FLOAT);
  int boundariesInt = cfg->boundaries ? 1 : 0;
  SetShaderValue(e->shader, e->boundariesLoc, &boundariesInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->reflectionGainLoc, &cfg->reflectionGain,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupVisualParams(InterferenceEffect *e,
                              const InterferenceConfig *cfg) {
  SetShaderValue(e->shader, e->visualModeLoc, &cfg->visualMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->contourCountLoc, &cfg->contourCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->visualGainLoc, &cfg->visualGain,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->chromaSpreadLoc, &cfg->chromaSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);
}

void InterferenceEffectSetup(InterferenceEffect *e, InterferenceConfig *cfg,
                             float deltaTime) {
  e->time += cfg->waveSpeed * deltaTime;

  float sources[16];
  float phases[8];
  const int count = cfg->sourceCount > 8 ? 8 : cfg->sourceCount;
  DualLissajousUpdateCircular(&cfg->lissajous, deltaTime, cfg->baseRadius, 0.0f,
                              0.0f, count, sources);

  for (int i = 0; i < count; i++) {
    phases[i] = (float)i / (float)count * TWO_PI_F;
  }

  ColorLUTUpdate(e->colorLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValueV(e->shader, e->sourcesLoc, sources, SHADER_UNIFORM_VEC2,
                  count);
  SetShaderValueV(e->shader, e->phasesLoc, phases, SHADER_UNIFORM_FLOAT, count);
  SetShaderValue(e->shader, e->sourceCountLoc, &count, SHADER_UNIFORM_INT);

  SetupWaveParams(e, cfg);
  SetupVisualParams(e, cfg);

  SetShaderValueTexture(e->shader, e->colorLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));
}

void InterferenceEffectUninit(InterferenceEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
}

InterferenceConfig InterferenceConfigDefault(void) {
  return InterferenceConfig{};
}

void InterferenceRegisterParams(InterferenceConfig *cfg) {
  ModEngineRegisterParam("interference.baseRadius", &cfg->baseRadius, 0.0f,
                         1.0f);
  ModEngineRegisterParam("interference.chromaSpread", &cfg->chromaSpread, 0.0f,
                         0.1f);
  ModEngineRegisterParam("interference.falloffStrength", &cfg->falloffStrength,
                         0.0f, 5.0f);
  ModEngineRegisterParam("interference.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("interference.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("interference.reflectionGain", &cfg->reflectionGain,
                         0.0f, 1.0f);
  ModEngineRegisterParam("interference.visualGain", &cfg->visualGain, 0.5f,
                         5.0f);
  ModEngineRegisterParam("interference.waveFreq", &cfg->waveFreq, 5.0f, 100.0f);
  ModEngineRegisterParam("interference.waveSpeed", &cfg->waveSpeed, 0.0f,
                         10.0f);
  ModEngineRegisterParam("interference.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupInterference(PostEffect *pe) {
  InterferenceEffectSetup(&pe->interference, &pe->effects.interference,
                          pe->currentDeltaTime);
}

void SetupInterferenceBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.interference.blendIntensity,
                       pe->effects.interference.blendMode);
}

// === UI ===

static void DrawInterferenceParams(EffectConfig *e,
                                   const ModSources *modSources,
                                   ImU32 categoryGlow) {
  InterferenceConfig *i = &e->interference;

  // Sources
  ImGui::SeparatorText("Sources");
  ImGui::SliderInt("Sources##interference", &i->sourceCount, 1, 8);
  ModulatableSlider("Radius##interference", &i->baseRadius,
                    "interference.baseRadius", "%.2f", modSources);

  // Motion (DualLissajous)
  ImGui::SeparatorText("Motion");
  DrawLissajousControls(&i->lissajous, "interference", "interference.lissajous",
                        modSources, 1.0f);

  // Waves
  ImGui::SeparatorText("Waves");
  ModulatableSlider("Wave Freq##interference", &i->waveFreq,
                    "interference.waveFreq", "%.1f", modSources);
  ModulatableSlider("Wave Speed##interference", &i->waveSpeed,
                    "interference.waveSpeed", "%.2f", modSources);

  // Falloff
  ImGui::SeparatorText("Falloff");
  ImGui::Combo("Falloff##interference", &i->falloffType,
               "None\0Inverse\0InvSquare\0Gaussian\0");
  ModulatableSlider("Falloff Strength##interference", &i->falloffStrength,
                    "interference.falloffStrength", "%.2f", modSources);

  // Boundaries
  ImGui::SeparatorText("Boundaries");
  ImGui::Checkbox("Boundaries##interference", &i->boundaries);
  if (i->boundaries) {
    ModulatableSlider("Reflection##interference", &i->reflectionGain,
                      "interference.reflectionGain", "%.2f", modSources);
  }

  // Visualization
  ImGui::SeparatorText("Visualization");
  ImGui::Combo("Visual Mode##interference", &i->visualMode,
               "Raw\0Absolute\0Contour\0");
  if (i->visualMode == 2) {
    ImGui::SliderInt("Contours##interference", &i->contourCount, 2, 20);
  }
  ModulatableSlider("Intensity##interference", &i->visualGain,
                    "interference.visualGain", "%.2f", modSources);

  // Color
  ImGui::SeparatorText("Color");
  ImGui::Combo("Color Mode##interference", &i->colorMode,
               "Intensity\0PerSource\0Chromatic\0");
  if (i->colorMode == 2) {
    ModulatableSlider("Chroma Spread##interference", &i->chromaSpread,
                      "interference.chromaSpread", "%.3f", modSources);
  }
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(interference)
REGISTER_GENERATOR(TRANSFORM_INTERFERENCE_BLEND, Interference, interference,
                   "Interference", SetupInterferenceBlend,
                   SetupInterference, 12, DrawInterferenceParams, DrawOutput_interference)
// clang-format on
