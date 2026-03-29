// Synthwave effect module implementation

#include "synthwave.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool SynthwaveEffectInit(SynthwaveEffect *e) {
  e->shader = LoadShader(NULL, "shaders/synthwave.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->horizonYLoc = GetShaderLocation(e->shader, "horizonY");
  e->colorMixLoc = GetShaderLocation(e->shader, "colorMix");
  e->palettePhaseLoc = GetShaderLocation(e->shader, "palettePhase");
  e->gridSpacingLoc = GetShaderLocation(e->shader, "gridSpacing");
  e->gridThicknessLoc = GetShaderLocation(e->shader, "gridThickness");
  e->gridOpacityLoc = GetShaderLocation(e->shader, "gridOpacity");
  e->gridGlowLoc = GetShaderLocation(e->shader, "gridGlow");
  e->gridColorLoc = GetShaderLocation(e->shader, "gridColor");
  e->stripeCountLoc = GetShaderLocation(e->shader, "stripeCount");
  e->stripeSoftnessLoc = GetShaderLocation(e->shader, "stripeSoftness");
  e->stripeIntensityLoc = GetShaderLocation(e->shader, "stripeIntensity");
  e->sunColorLoc = GetShaderLocation(e->shader, "sunColor");
  e->horizonIntensityLoc = GetShaderLocation(e->shader, "horizonIntensity");
  e->horizonFalloffLoc = GetShaderLocation(e->shader, "horizonFalloff");
  e->horizonColorLoc = GetShaderLocation(e->shader, "horizonColor");
  e->gridTimeLoc = GetShaderLocation(e->shader, "gridTime");
  e->stripeTimeLoc = GetShaderLocation(e->shader, "stripeTime");

  e->gridTime = 0.0f;
  e->stripeTime = 0.0f;

  return true;
}

static void SetupGridUniforms(const SynthwaveEffect *e,
                              const SynthwaveConfig *cfg) {
  SetShaderValue(e->shader, e->gridSpacingLoc, &cfg->gridSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gridThicknessLoc, &cfg->gridThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gridOpacityLoc, &cfg->gridOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gridGlowLoc, &cfg->gridGlow,
                 SHADER_UNIFORM_FLOAT);
  const float gridColor[3] = {cfg->gridR, cfg->gridG, cfg->gridB};
  SetShaderValue(e->shader, e->gridColorLoc, gridColor, SHADER_UNIFORM_VEC3);
}

static void SetupHorizonUniforms(const SynthwaveEffect *e,
                                 const SynthwaveConfig *cfg) {
  SetShaderValue(e->shader, e->horizonIntensityLoc, &cfg->horizonIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->horizonFalloffLoc, &cfg->horizonFalloff,
                 SHADER_UNIFORM_FLOAT);
  const float horizonColor[3] = {cfg->horizonR, cfg->horizonG, cfg->horizonB};
  SetShaderValue(e->shader, e->horizonColorLoc, horizonColor,
                 SHADER_UNIFORM_VEC3);
}

void SynthwaveEffectSetup(SynthwaveEffect *e, const SynthwaveConfig *cfg,
                          float deltaTime) {
  e->gridTime += cfg->gridScrollSpeed * deltaTime;
  e->stripeTime += cfg->stripeScrollSpeed * deltaTime;

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->horizonYLoc, &cfg->horizonY,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorMixLoc, &cfg->colorMix,
                 SHADER_UNIFORM_FLOAT);
  const float palettePhase[3] = {cfg->palettePhaseR, cfg->palettePhaseG,
                                 cfg->palettePhaseB};
  SetShaderValue(e->shader, e->palettePhaseLoc, palettePhase,
                 SHADER_UNIFORM_VEC3);

  SetupGridUniforms(e, cfg);

  SetShaderValue(e->shader, e->stripeCountLoc, &cfg->stripeCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeSoftnessLoc, &cfg->stripeSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeIntensityLoc, &cfg->stripeIntensity,
                 SHADER_UNIFORM_FLOAT);
  const float sunColor[3] = {cfg->sunR, cfg->sunG, cfg->sunB};
  SetShaderValue(e->shader, e->sunColorLoc, sunColor, SHADER_UNIFORM_VEC3);

  SetupHorizonUniforms(e, cfg);

  SetShaderValue(e->shader, e->gridTimeLoc, &e->gridTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stripeTimeLoc, &e->stripeTime,
                 SHADER_UNIFORM_FLOAT);
}

void SynthwaveEffectUninit(const SynthwaveEffect *e) {
  UnloadShader(e->shader);
}

void SynthwaveRegisterParams(SynthwaveConfig *cfg) {
  ModEngineRegisterParam("synthwave.horizonY", &cfg->horizonY, 0.3f, 0.7f);
  ModEngineRegisterParam("synthwave.colorMix", &cfg->colorMix, 0.0f, 1.0f);
  ModEngineRegisterParam("synthwave.gridOpacity", &cfg->gridOpacity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("synthwave.gridGlow", &cfg->gridGlow, 1.0f, 3.0f);
  ModEngineRegisterParam("synthwave.stripeIntensity", &cfg->stripeIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("synthwave.horizonIntensity", &cfg->horizonIntensity,
                         0.0f, 1.0f);
}

// === UI ===

static void DrawSynthwaveParams(EffectConfig *e, const ModSources *ms,
                                ImU32 glow) {
  (void)glow;
  SynthwaveConfig *sw = &e->synthwave;

  ModulatableSlider("Horizon##synthwave", &sw->horizonY, "synthwave.horizonY",
                    "%.2f", ms);
  ModulatableSlider("Color Mix##synthwave", &sw->colorMix, "synthwave.colorMix",
                    "%.2f", ms);

  ImGui::SeparatorText("Palette");
  ImGui::SliderFloat("Phase R##synthwave", &sw->palettePhaseR, 0.0f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Phase G##synthwave", &sw->palettePhaseG, 0.0f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Phase B##synthwave", &sw->palettePhaseB, 0.0f, 1.0f,
                     "%.2f");

  ImGui::SeparatorText("Grid");
  ImGui::SliderFloat("Spacing##synthwave", &sw->gridSpacing, 2.0f, 20.0f,
                     "%.1f");
  ImGui::SliderFloat("Line Width##synthwave", &sw->gridThickness, 0.01f, 0.1f,
                     "%.3f");
  ModulatableSlider("Opacity##synthwave_grid", &sw->gridOpacity,
                    "synthwave.gridOpacity", "%.2f", ms);
  ModulatableSlider("Glow##synthwave", &sw->gridGlow, "synthwave.gridGlow",
                    "%.2f", ms);
  float gridCol[3] = {sw->gridR, sw->gridG, sw->gridB};
  if (ImGui::ColorEdit3("Color##synthwave_grid", gridCol)) {
    sw->gridR = gridCol[0];
    sw->gridG = gridCol[1];
    sw->gridB = gridCol[2];
  }

  ImGui::SeparatorText("Sun Stripes");
  ImGui::SliderFloat("Count##synthwave", &sw->stripeCount, 4.0f, 20.0f, "%.0f");
  ImGui::SliderFloat("Softness##synthwave", &sw->stripeSoftness, 0.0f, 0.3f,
                     "%.2f");
  ModulatableSlider("Intensity##synthwave_stripe", &sw->stripeIntensity,
                    "synthwave.stripeIntensity", "%.2f", ms);
  float sunCol[3] = {sw->sunR, sw->sunG, sw->sunB};
  if (ImGui::ColorEdit3("Color##synthwave_sun", sunCol)) {
    sw->sunR = sunCol[0];
    sw->sunG = sunCol[1];
    sw->sunB = sunCol[2];
  }

  ImGui::SeparatorText("Horizon Glow");
  ModulatableSlider("Intensity##synthwave_horizon", &sw->horizonIntensity,
                    "synthwave.horizonIntensity", "%.2f", ms);
  ImGui::SliderFloat("Falloff##synthwave", &sw->horizonFalloff, 5.0f, 30.0f,
                     "%.1f");
  float horizonCol[3] = {sw->horizonR, sw->horizonG, sw->horizonB};
  if (ImGui::ColorEdit3("Color##synthwave_horizon", horizonCol)) {
    sw->horizonR = horizonCol[0];
    sw->horizonG = horizonCol[1];
    sw->horizonB = horizonCol[2];
  }

  ImGui::SeparatorText("Animation");
  ImGui::SliderFloat("Grid Scroll##synthwave", &sw->gridScrollSpeed, 0.0f, 2.0f,
                     "%.2f");
  ImGui::SliderFloat("Stripe Scroll##synthwave", &sw->stripeScrollSpeed, 0.0f,
                     0.5f, "%.3f");
}

void SetupSynthwave(PostEffect *pe) {
  SynthwaveEffectSetup(&pe->synthwave, &pe->effects.synthwave,
                       pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_SYNTHWAVE, Synthwave, synthwave, "Synthwave", "RET",
                6, EFFECT_FLAG_NONE, SetupSynthwave, NULL,
                DrawSynthwaveParams)
// clang-format on
