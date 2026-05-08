// Pillar Grid: Top-down grid of rounded pillars with luminance-driven height

#include "pillar_grid.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"

#include "imgui.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

static const float HALF_PI = PI_F / 2.0f;
static const float PITCH_MIN = 1.484f; // 85 degrees

bool PillarGridEffectInit(PillarGridEffect *e) {
  e->shader = LoadShader(NULL, "shaders/pillar_grid.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->densityLoc = GetShaderLocation(e->shader, "density");
  e->pitchLoc = GetShaderLocation(e->shader, "pitch");
  e->heightScaleLoc = GetShaderLocation(e->shader, "heightScale");
  e->cornerRadiusLoc = GetShaderLocation(e->shader, "cornerRadius");

  return true;
}

void PillarGridEffectSetup(const PillarGridEffect *e,
                           const PillarGridConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->densityLoc, &cfg->density, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pitchLoc, &cfg->pitch, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->heightScaleLoc, &cfg->heightScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cornerRadiusLoc, &cfg->cornerRadius,
                 SHADER_UNIFORM_FLOAT);
}

void PillarGridEffectUninit(const PillarGridEffect *e) {
  UnloadShader(e->shader);
}

void PillarGridRegisterParams(PillarGridConfig *cfg) {
  ModEngineRegisterParam("pillarGrid.density", &cfg->density, 48.0f, 128.0f);
  ModEngineRegisterParam("pillarGrid.pitch", &cfg->pitch, PITCH_MIN, HALF_PI);
  ModEngineRegisterParam("pillarGrid.heightScale", &cfg->heightScale, 0.0f,
                         8.0f);
  ModEngineRegisterParam("pillarGrid.cornerRadius", &cfg->cornerRadius, 0.0f,
                         0.5f);
}

PillarGridEffect *GetPillarGridEffect(PostEffect *pe) {
  return (PillarGridEffect *)pe->effectStates[TRANSFORM_PILLAR_GRID];
}

void SetupPillarGrid(PostEffect *pe) {
  PillarGridEffectSetup(GetPillarGridEffect(pe), &pe->effects.pillarGrid);
}

// === UI ===

static void DrawPillarGridParams(EffectConfig *e, const ModSources *ms,
                                 ImU32 glow) {
  (void)glow;
  PillarGridConfig *p = &e->pillarGrid;

  ImGui::SeparatorText("Camera");
  ModulatableSliderAngleDeg("Pitch##pillarGrid", &p->pitch, "pillarGrid.pitch",
                            ms);

  ImGui::SeparatorText("Geometry");
  ModulatableSliderInt("Density##pillarGrid", &p->density, "pillarGrid.density",
                       ms);
  ModulatableSlider("Height Scale##pillarGrid", &p->heightScale,
                    "pillarGrid.heightScale", "%.2f", ms);
  ModulatableSlider("Corner Radius##pillarGrid", &p->cornerRadius,
                    "pillarGrid.cornerRadius", "%.2f", ms);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_PILLAR_GRID, PillarGrid, pillarGrid,
                "Pillar Grid", "CELL", 2, EFFECT_FLAG_HALF_RES,
                SetupPillarGrid, NULL, DrawPillarGridParams)
// clang-format on
