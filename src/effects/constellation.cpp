// Constellation effect module implementation
// Procedural star field with wandering points and distance-based connection
// lines

#include "constellation.h"
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
#include "ui/ui_units.h"
#include <stddef.h>

bool ConstellationEffectInit(ConstellationEffect *e,
                             const ConstellationConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/constellation.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->gridScaleLoc = GetShaderLocation(e->shader, "gridScale");
  e->wanderAmpLoc = GetShaderLocation(e->shader, "wanderAmp");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->waveAmpLoc = GetShaderLocation(e->shader, "waveAmp");
  e->pointSizeLoc = GetShaderLocation(e->shader, "pointSize");
  e->pointBrightnessLoc = GetShaderLocation(e->shader, "pointBrightness");
  e->lineThicknessLoc = GetShaderLocation(e->shader, "lineThickness");
  e->maxLineLenLoc = GetShaderLocation(e->shader, "maxLineLen");
  e->lineOpacityLoc = GetShaderLocation(e->shader, "lineOpacity");
  e->interpolateLineColorLoc =
      GetShaderLocation(e->shader, "interpolateLineColor");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->wavePhaseLoc = GetShaderLocation(e->shader, "wavePhase");
  e->pointLUTLoc = GetShaderLocation(e->shader, "pointLUT");
  e->lineLUTLoc = GetShaderLocation(e->shader, "lineLUT");
  e->fillEnabledLoc = GetShaderLocation(e->shader, "fillEnabled");
  e->fillOpacityLoc = GetShaderLocation(e->shader, "fillOpacity");
  e->fillThresholdLoc = GetShaderLocation(e->shader, "fillThreshold");
  e->waveCenterLoc = GetShaderLocation(e->shader, "waveCenter");
  e->waveInfluenceLoc = GetShaderLocation(e->shader, "waveInfluence");
  e->pointOpacityLoc = GetShaderLocation(e->shader, "pointOpacity");
  e->depthLayersLoc = GetShaderLocation(e->shader, "depthLayers");

  e->pointLUT = ColorLUTInit(&cfg->gradient);
  if (e->pointLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->lineLUT = ColorLUTInit(&cfg->lineGradient);
  if (e->lineLUT == NULL) {
    ColorLUTUninit(e->pointLUT);
    UnloadShader(e->shader);
    return false;
  }

  e->animPhase = 0.0f;
  e->wavePhase = 0.0f;

  return true;
}

void ConstellationEffectSetup(ConstellationEffect *e,
                              const ConstellationConfig *cfg, float deltaTime) {
  e->animPhase += cfg->animSpeed * deltaTime;
  e->animPhase = fmodf(e->animPhase, 6.2831853f);
  e->wavePhase += cfg->waveSpeed * deltaTime;
  e->wavePhase = fmodf(e->wavePhase, 6.2831853f);

  ColorLUTUpdate(e->pointLUT, &cfg->gradient);
  ColorLUTUpdate(e->lineLUT, &cfg->lineGradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->waveInfluenceLoc, &cfg->waveInfluence,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pointOpacityLoc, &cfg->pointOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->depthLayersLoc, &cfg->depthLayers,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->animPhaseLoc, &e->animPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wavePhaseLoc, &e->wavePhase,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->gridScaleLoc, &cfg->gridScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->wanderAmpLoc, &cfg->wanderAmp,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveAmpLoc, &cfg->waveAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pointSizeLoc, &cfg->pointSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pointBrightnessLoc, &cfg->pointBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineThicknessLoc, &cfg->lineThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxLineLenLoc, &cfg->maxLineLen,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineOpacityLoc, &cfg->lineOpacity,
                 SHADER_UNIFORM_FLOAT);

  int interp = cfg->interpolateLineColor ? 1 : 0;
  SetShaderValue(e->shader, e->interpolateLineColorLoc, &interp,
                 SHADER_UNIFORM_INT);

  int fillEn = cfg->fillEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->fillEnabledLoc, &fillEn, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->fillOpacityLoc, &cfg->fillOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fillThresholdLoc, &cfg->fillThreshold,
                 SHADER_UNIFORM_FLOAT);

  float waveCenter[2] = {
      (cfg->waveCenterX - 0.5f) * cfg->gridScale *
          ((float)GetScreenWidth() / (float)GetScreenHeight()),
      (cfg->waveCenterY - 0.5f) * cfg->gridScale};
  SetShaderValue(e->shader, e->waveCenterLoc, waveCenter, SHADER_UNIFORM_VEC2);

  SetShaderValueTexture(e->shader, e->pointLUTLoc,
                        ColorLUTGetTexture(e->pointLUT));
  SetShaderValueTexture(e->shader, e->lineLUTLoc,
                        ColorLUTGetTexture(e->lineLUT));
}

void ConstellationEffectUninit(ConstellationEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->pointLUT);
  ColorLUTUninit(e->lineLUT);
}

ConstellationConfig ConstellationConfigDefault(void) {
  return ConstellationConfig{};
}

void ConstellationRegisterParams(ConstellationConfig *cfg) {
  ModEngineRegisterParam("constellation.animSpeed", &cfg->animSpeed, 0.0f,
                         5.0f);
  ModEngineRegisterParam("constellation.gridScale", &cfg->gridScale, 5.0f,
                         50.0f);
  ModEngineRegisterParam("constellation.lineOpacity", &cfg->lineOpacity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("constellation.maxLineLen", &cfg->maxLineLen, 0.5f,
                         2.0f);
  ModEngineRegisterParam("constellation.pointBrightness", &cfg->pointBrightness,
                         0.0f, 2.0f);
  ModEngineRegisterParam("constellation.pointSize", &cfg->pointSize, 0.3f,
                         3.0f);
  ModEngineRegisterParam("constellation.waveAmp", &cfg->waveAmp, 0.0f, 4.0f);
  ModEngineRegisterParam("constellation.waveSpeed", &cfg->waveSpeed, 0.0f,
                         5.0f);
  ModEngineRegisterParam("constellation.fillOpacity", &cfg->fillOpacity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("constellation.wanderAmp", &cfg->wanderAmp, 0.0f,
                         0.5f);
  ModEngineRegisterParam("constellation.pointOpacity", &cfg->pointOpacity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("constellation.waveInfluence", &cfg->waveInfluence,
                         0.0f, 1.0f);
  ModEngineRegisterParam("constellation.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupConstellation(PostEffect *pe) {
  ConstellationEffectSetup(&pe->constellation, &pe->effects.constellation,
                           pe->currentDeltaTime);
}

void SetupConstellationBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.constellation.blendIntensity,
                       pe->effects.constellation.blendMode);
}

// === UI ===

static void DrawConstellationParams(EffectConfig *e,
                                    const ModSources *modSources,
                                    ImU32 categoryGlow) {
  (void)categoryGlow;
  ConstellationConfig *c = &e->constellation;

  // Grid and animation
  ModulatableSlider("Grid Scale##constellation", &c->gridScale,
                    "constellation.gridScale", "%.1f", modSources);
  ModulatableSlider("Anim Speed##constellation", &c->animSpeed,
                    "constellation.animSpeed", "%.2f", modSources);
  ModulatableSlider("Wander##constellation", &c->wanderAmp,
                    "constellation.wanderAmp", "%.2f", modSources);

  // Wave overlay
  ImGui::SeparatorText("Wave");
  ImGui::SliderFloat("Wave Freq##constellation", &c->waveFreq, 0.1f, 2.0f,
                     "%.2f");
  ModulatableSlider("Wave Amp##constellation", &c->waveAmp,
                    "constellation.waveAmp", "%.2f", modSources);
  ModulatableSlider("Wave Speed##constellation", &c->waveSpeed,
                    "constellation.waveSpeed", "%.2f", modSources);
  ImGui::SliderFloat("Wave Center X##constellation", &c->waveCenterX, -2.0f,
                     3.0f, "%.2f");
  ImGui::SliderFloat("Wave Center Y##constellation", &c->waveCenterY, -2.0f,
                     3.0f, "%.2f");

  ModulatableSlider("Wave Influence##constellation", &c->waveInfluence,
                    "constellation.waveInfluence", "%.2f", modSources);

  // Depth
  ImGui::SliderInt("Depth Layers##constellation", &c->depthLayers, 1, 3);

  // Point rendering
  ImGui::SeparatorText("Points");
  ModulatableSlider("Point Size##constellation", &c->pointSize,
                    "constellation.pointSize", "%.2f", modSources);
  ModulatableSlider("Point Bright##constellation", &c->pointBrightness,
                    "constellation.pointBrightness", "%.2f", modSources);
  ModulatableSlider("Point Opacity##constellation", &c->pointOpacity,
                    "constellation.pointOpacity", "%.2f", modSources);

  // Line rendering
  ImGui::SeparatorText("Lines");
  ImGui::SliderFloat("Line Width##constellation", &c->lineThickness, 0.01f,
                     0.1f, "%.3f");
  ModulatableSlider("Max Line Len##constellation", &c->maxLineLen,
                    "constellation.maxLineLen", "%.2f", modSources);
  ModulatableSlider("Line Opacity##constellation", &c->lineOpacity,
                    "constellation.lineOpacity", "%.2f", modSources);
  ImGui::Checkbox("Interpolate Line Color##constellation",
                  &c->interpolateLineColor);
  ImGuiDrawColorMode(&c->lineGradient);

  // Triangle fill
  ImGui::SeparatorText("Triangles");
  ImGui::Checkbox("Fill Triangles##constellation", &c->fillEnabled);
  if (c->fillEnabled) {
    ModulatableSlider("Fill Opacity##constellation", &c->fillOpacity,
                      "constellation.fillOpacity", "%.2f", modSources);
    ImGui::SliderFloat("Fill Threshold##constellation", &c->fillThreshold, 1.0f,
                       4.0f, "%.1f");
  }
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(constellation)
REGISTER_GENERATOR(TRANSFORM_CONSTELLATION_BLEND, Constellation, constellation,
                   "Constellation", SetupConstellationBlend,
                   SetupConstellation, 11, DrawConstellationParams,
                   DrawOutput_constellation)
// clang-format on
