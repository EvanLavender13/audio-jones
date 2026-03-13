// Neon Lattice effect module implementation
// Raymarched torus columns on a repeating 3D grid with orbital camera and glow

#include "neon_lattice.h"
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

// ---------------------------------------------------------------------------
// Init / Setup / Uninit
// ---------------------------------------------------------------------------

static void CacheLocations(NeonLatticeEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->spacingLoc = GetShaderLocation(e->shader, "spacing");
  e->lightSpacingLoc = GetShaderLocation(e->shader, "lightSpacing");
  e->attenuationLoc = GetShaderLocation(e->shader, "attenuation");
  e->glowExponentLoc = GetShaderLocation(e->shader, "glowExponent");
  e->cameraTimeLoc = GetShaderLocation(e->shader, "cameraTime");
  e->columnsTimeLoc = GetShaderLocation(e->shader, "columnsTime");
  e->lightsTimeLoc = GetShaderLocation(e->shader, "lightsTime");
  e->orbitRadiusLoc = GetShaderLocation(e->shader, "orbitRadius");
  e->orbitVariationLoc = GetShaderLocation(e->shader, "orbitVariation");
  e->orbitRatioXLoc = GetShaderLocation(e->shader, "orbitRatioX");
  e->orbitRatioYLoc = GetShaderLocation(e->shader, "orbitRatioY");
  e->orbitRatioZLoc = GetShaderLocation(e->shader, "orbitRatioZ");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->maxDistLoc = GetShaderLocation(e->shader, "maxDist");
  e->torusRadiusLoc = GetShaderLocation(e->shader, "torusRadius");
  e->torusTubeLoc = GetShaderLocation(e->shader, "torusTube");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->axisCountLoc = GetShaderLocation(e->shader, "axisCount");
}

bool NeonLatticeEffectInit(NeonLatticeEffect *e, const NeonLatticeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/neon_lattice.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->cameraPhase = 0.0f;
  e->columnsPhase = 0.0f;
  e->lightsPhase = 0.0f;

  return true;
}

void NeonLatticeEffectSetup(NeonLatticeEffect *e, const NeonLatticeConfig *cfg,
                            float deltaTime) {
  e->cameraPhase += cfg->cameraSpeed * deltaTime;
  e->columnsPhase += cfg->columnsSpeed * deltaTime;
  e->lightsPhase += cfg->lightsSpeed * deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->spacingLoc, &cfg->spacing, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightSpacingLoc, &cfg->lightSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->attenuationLoc, &cfg->attenuation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowExponentLoc, &cfg->glowExponent,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->cameraTimeLoc, &e->cameraPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->columnsTimeLoc, &e->columnsPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightsTimeLoc, &e->lightsPhase,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->orbitRadiusLoc, &cfg->orbitRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitVariationLoc, &cfg->orbitVariation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitRatioXLoc, &cfg->orbitRatioX,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitRatioYLoc, &cfg->orbitRatioY,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitRatioZLoc, &cfg->orbitRatioZ,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->maxDistLoc, &cfg->maxDist, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->torusRadiusLoc, &cfg->torusRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->torusTubeLoc, &cfg->torusTube,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->axisCountLoc, &cfg->axisCount,
                 SHADER_UNIFORM_INT);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void NeonLatticeEffectUninit(NeonLatticeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

NeonLatticeConfig NeonLatticeConfigDefault(void) { return NeonLatticeConfig{}; }

void NeonLatticeRegisterParams(NeonLatticeConfig *cfg) {
  ModEngineRegisterParam("neonLattice.spacing", &cfg->spacing, 2.0f, 20.0f);
  ModEngineRegisterParam("neonLattice.lightSpacing", &cfg->lightSpacing, 0.5f,
                         5.0f);
  ModEngineRegisterParam("neonLattice.attenuation", &cfg->attenuation, 5.0f,
                         60.0f);
  ModEngineRegisterParam("neonLattice.glowExponent", &cfg->glowExponent, 0.5f,
                         3.0f);
  ModEngineRegisterParam("neonLattice.cameraSpeed", &cfg->cameraSpeed, 0.0f,
                         3.0f);
  ModEngineRegisterParam("neonLattice.columnsSpeed", &cfg->columnsSpeed, 0.0f,
                         15.0f);
  ModEngineRegisterParam("neonLattice.lightsSpeed", &cfg->lightsSpeed, 0.0f,
                         60.0f);
  ModEngineRegisterParam("neonLattice.orbitRadius", &cfg->orbitRadius, 20.0f,
                         120.0f);
  ModEngineRegisterParam("neonLattice.orbitVariation", &cfg->orbitVariation,
                         0.0f, 40.0f);
  ModEngineRegisterParam("neonLattice.orbitRatioX", &cfg->orbitRatioX, 0.5f,
                         2.0f);
  ModEngineRegisterParam("neonLattice.orbitRatioY", &cfg->orbitRatioY, 0.5f,
                         2.0f);
  ModEngineRegisterParam("neonLattice.orbitRatioZ", &cfg->orbitRatioZ, 0.5f,
                         2.0f);
  ModEngineRegisterParam("neonLattice.maxDist", &cfg->maxDist, 20.0f, 120.0f);
  ModEngineRegisterParam("neonLattice.torusRadius", &cfg->torusRadius, 0.2f,
                         1.5f);
  ModEngineRegisterParam("neonLattice.torusTube", &cfg->torusTube, 0.02f, 0.2f);
  ModEngineRegisterParam("neonLattice.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupNeonLattice(PostEffect *pe) {
  NeonLatticeEffectSetup(&pe->neonLattice, &pe->effects.neonLattice,
                         pe->currentDeltaTime);
}

void SetupNeonLatticeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.neonLattice.blendIntensity,
                       pe->effects.neonLattice.blendMode);
}

// === UI ===

static void DrawNeonLatticeParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  NeonLatticeConfig *cfg = &e->neonLattice;

  ImGui::SeparatorText("Grid");
  int comboIdx = cfg->axisCount - 1;
  if (ImGui::Combo("Axes##neonLattice", &comboIdx,
                   "1 Axis\0 2 Axes\0 3 Axes\0")) {
    cfg->axisCount = comboIdx + 1;
  }
  ModulatableSlider("Spacing##neonLattice", &cfg->spacing,
                    "neonLattice.spacing", "%.1f", modSources);
  ModulatableSlider("Light Spacing##neonLattice", &cfg->lightSpacing,
                    "neonLattice.lightSpacing", "%.2f", modSources);
  ModulatableSlider("Attenuation##neonLattice", &cfg->attenuation,
                    "neonLattice.attenuation", "%.1f", modSources);
  ModulatableSlider("Glow Exponent##neonLattice", &cfg->glowExponent,
                    "neonLattice.glowExponent", "%.2f", modSources);

  ImGui::SeparatorText("Speed");
  ModulatableSlider("Camera Speed##neonLattice", &cfg->cameraSpeed,
                    "neonLattice.cameraSpeed", "%.2f", modSources);
  ModulatableSlider("Columns Speed##neonLattice", &cfg->columnsSpeed,
                    "neonLattice.columnsSpeed", "%.1f", modSources);
  ModulatableSlider("Lights Speed##neonLattice", &cfg->lightsSpeed,
                    "neonLattice.lightsSpeed", "%.1f", modSources);

  ImGui::SeparatorText("Camera");
  ModulatableSlider("Orbit Radius##neonLattice", &cfg->orbitRadius,
                    "neonLattice.orbitRadius", "%.1f", modSources);
  ModulatableSlider("Orbit Variation##neonLattice", &cfg->orbitVariation,
                    "neonLattice.orbitVariation", "%.1f", modSources);
  ModulatableSlider("Ratio X##neonLattice", &cfg->orbitRatioX,
                    "neonLattice.orbitRatioX", "%.2f", modSources);
  ModulatableSlider("Ratio Y##neonLattice", &cfg->orbitRatioY,
                    "neonLattice.orbitRatioY", "%.2f", modSources);
  ModulatableSlider("Ratio Z##neonLattice", &cfg->orbitRatioZ,
                    "neonLattice.orbitRatioZ", "%.2f", modSources);

  ImGui::SeparatorText("Shape");
  ModulatableSlider("Ring Radius##neonLattice", &cfg->torusRadius,
                    "neonLattice.torusRadius", "%.2f", modSources);
  ModulatableSlider("Tube Width##neonLattice", &cfg->torusTube,
                    "neonLattice.torusTube", "%.3f", modSources);

  ImGui::SeparatorText("Quality");
  ImGui::SliderInt("Iterations##neonLattice", &cfg->iterations, 20, 80);
  ModulatableSlider("Max Distance##neonLattice", &cfg->maxDist,
                    "neonLattice.maxDist", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(neonLattice)
REGISTER_GENERATOR(TRANSFORM_NEON_LATTICE_BLEND, NeonLattice, neonLattice,
                   "Neon Lattice", SetupNeonLatticeBlend, SetupNeonLattice, 10,
                   DrawNeonLatticeParams, DrawOutput_neonLattice)
// clang-format on
