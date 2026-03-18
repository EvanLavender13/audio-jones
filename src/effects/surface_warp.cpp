#include "surface_warp.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool SurfaceWarpEffectInit(SurfaceWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/surface_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->angleLoc = GetShaderLocation(e->shader, "angle");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->scrollOffsetLoc = GetShaderLocation(e->shader, "scrollOffset");
  e->depthShadeLoc = GetShaderLocation(e->shader, "depthShade");

  e->rotation = 0.0f;
  e->scrollOffset = 0.0f;

  return true;
}

void SurfaceWarpEffectSetup(SurfaceWarpEffect *e, const SurfaceWarpConfig *cfg,
                            float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->scrollOffset += cfg->scrollSpeed * deltaTime;

  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->angleLoc, &cfg->angle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollOffsetLoc, &e->scrollOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->depthShadeLoc, &cfg->depthShade,
                 SHADER_UNIFORM_FLOAT);
}

void SurfaceWarpEffectUninit(const SurfaceWarpEffect *e) {
  UnloadShader(e->shader);
}

void SurfaceWarpRegisterParams(SurfaceWarpConfig *cfg) {
  ModEngineRegisterParam("surfaceWarp.intensity", &cfg->intensity, 0.0f, 2.0f);
  ModEngineRegisterParam("surfaceWarp.angle", &cfg->angle, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("surfaceWarp.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("surfaceWarp.scrollSpeed", &cfg->scrollSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("surfaceWarp.depthShade", &cfg->depthShade, 0.0f,
                         1.0f);
}

// === UI ===

static void DrawSurfaceWarpParams(EffectConfig *e, const ModSources *ms,
                                  ImU32 glow) {
  (void)glow;
  ModulatableSlider("Intensity##surfacewarp", &e->surfaceWarp.intensity,
                    "surfaceWarp.intensity", "%.2f", ms);
  ModulatableSliderAngleDeg("Angle##surfacewarp", &e->surfaceWarp.angle,
                            "surfaceWarp.angle", ms);
  ModulatableSliderSpeedDeg("Rotation Speed##surfacewarp",
                            &e->surfaceWarp.rotationSpeed,
                            "surfaceWarp.rotationSpeed", ms);
  ModulatableSlider("Scroll Speed##surfacewarp", &e->surfaceWarp.scrollSpeed,
                    "surfaceWarp.scrollSpeed", "%.2f", ms);
  ModulatableSlider("Depth Shade##surfacewarp", &e->surfaceWarp.depthShade,
                    "surfaceWarp.depthShade", "%.2f", ms);
}

void SetupSurfaceWarp(PostEffect *pe) {
  SurfaceWarpEffectSetup(&pe->surfaceWarp, &pe->effects.surfaceWarp,
                         pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_SURFACE_WARP, SurfaceWarp, surfaceWarp,
                "Surface Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupSurfaceWarp, NULL, DrawSurfaceWarpParams)
// clang-format on
