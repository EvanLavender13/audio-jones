// Scrawl effect module implementation
// IFS fractal fold with thick marker strokes and gradient LUT coloring

#include "scrawl.h"
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

bool ScrawlEffectInit(ScrawlEffect *e, const ScrawlConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/scrawl.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->foldOffsetLoc = GetShaderLocation(e->shader, "foldOffset");
  e->tileScaleLoc = GetShaderLocation(e->shader, "tileScale");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
  e->warpFreqLoc = GetShaderLocation(e->shader, "warpFreq");
  e->warpAmpLoc = GetShaderLocation(e->shader, "warpAmp");
  e->thicknessLoc = GetShaderLocation(e->shader, "thickness");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->scrollAccumLoc = GetShaderLocation(e->shader, "scrollAccum");
  e->evolveAccumLoc = GetShaderLocation(e->shader, "evolveAccum");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->scrollAccum = 0.0f;
  e->evolveAccum = 0.0f;
  e->rotationAccum = 0.0f;

  return true;
}

void ScrawlEffectSetup(ScrawlEffect *e, const ScrawlConfig *cfg,
                       float deltaTime) {
  e->scrollAccum += cfg->scrollSpeed * deltaTime;
  e->evolveAccum += cfg->evolveSpeed * deltaTime;
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->foldOffsetLoc, &cfg->foldOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tileScaleLoc, &cfg->tileScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpFreqLoc, &cfg->warpFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpAmpLoc, &cfg->warpAmp, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thicknessLoc, &cfg->thickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollAccumLoc, &e->scrollAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->evolveAccumLoc, &e->evolveAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void ScrawlEffectUninit(ScrawlEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

ScrawlConfig ScrawlConfigDefault(void) { return ScrawlConfig{}; }

void ScrawlRegisterParams(ScrawlConfig *cfg) {
  ModEngineRegisterParam("scrawl.foldOffset", &cfg->foldOffset, 0.0f, 1.5f);
  ModEngineRegisterParam("scrawl.zoom", &cfg->zoom, 0.1f, 1.0f);
  ModEngineRegisterParam("scrawl.warpFreq", &cfg->warpFreq, 5.0f, 40.0f);
  ModEngineRegisterParam("scrawl.warpAmp", &cfg->warpAmp, 0.0f, 0.5f);
  ModEngineRegisterParam("scrawl.thickness", &cfg->thickness, 0.005f, 0.05f);
  ModEngineRegisterParam("scrawl.glowIntensity", &cfg->glowIntensity, 0.5f,
                         5.0f);
  ModEngineRegisterParam("scrawl.scrollSpeed", &cfg->scrollSpeed, -1.0f, 1.0f);
  ModEngineRegisterParam("scrawl.evolveSpeed", &cfg->evolveSpeed, -1.0f, 1.0f);
  ModEngineRegisterParam("scrawl.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("scrawl.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupScrawl(PostEffect *pe) {
  ScrawlEffectSetup(&pe->scrawl, &pe->effects.scrawl, pe->currentDeltaTime);
}

void SetupScrawlBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.scrawl.blendIntensity,
                       pe->effects.scrawl.blendMode);
}

// === UI ===

static void DrawScrawlParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  ScrawlConfig *cfg = &e->scrawl;

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##scrawl", &cfg->iterations, 2, 12);
  ModulatableSlider("Fold Offset##scrawl", &cfg->foldOffset,
                    "scrawl.foldOffset", "%.2f", modSources);
  ImGui::SliderFloat("Tile Scale##scrawl", &cfg->tileScale, 0.2f, 2.0f, "%.2f");
  ModulatableSlider("Zoom##scrawl", &cfg->zoom, "scrawl.zoom", "%.2f",
                    modSources);
  ModulatableSlider("Warp Freq##scrawl", &cfg->warpFreq, "scrawl.warpFreq",
                    "%.1f", modSources);
  ModulatableSlider("Warp Amp##scrawl", &cfg->warpAmp, "scrawl.warpAmp", "%.3f",
                    modSources);

  // Rendering
  ImGui::SeparatorText("Rendering");
  ModulatableSliderLog("Thickness##scrawl", &cfg->thickness, "scrawl.thickness",
                       "%.3f", modSources);
  ModulatableSlider("Glow Intensity##scrawl", &cfg->glowIntensity,
                    "scrawl.glowIntensity", "%.2f", modSources);
  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Scroll Speed##scrawl", &cfg->scrollSpeed,
                    "scrawl.scrollSpeed", "%.2f", modSources);
  ModulatableSlider("Evolve Speed##scrawl", &cfg->evolveSpeed,
                    "scrawl.evolveSpeed", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rotation Speed##scrawl", &cfg->rotationSpeed,
                            "scrawl.rotationSpeed", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(scrawl)
REGISTER_GENERATOR(TRANSFORM_SCRAWL_BLEND, Scrawl, scrawl,
                   "Scrawl", SetupScrawlBlend,
                   SetupScrawl, 12, DrawScrawlParams, DrawOutput_scrawl)
// clang-format on
