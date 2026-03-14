#include "infinite_zoom.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/dual_lissajous_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool InfiniteZoomEffectInit(InfiniteZoomEffect *e) {
  e->shader = LoadShader(NULL, "shaders/infinite_zoom.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->zoomDepthLoc = GetShaderLocation(e->shader, "zoomDepth");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->spiralAngleLoc = GetShaderLocation(e->shader, "spiralAngle");
  e->spiralTwistLoc = GetShaderLocation(e->shader, "spiralTwist");
  e->layerRotateLoc = GetShaderLocation(e->shader, "layerRotate");
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->offsetLoc = GetShaderLocation(e->shader, "offset");
  e->warpTypeLoc = GetShaderLocation(e->shader, "warpType");
  e->warpStrengthLoc = GetShaderLocation(e->shader, "warpStrength");
  e->warpFreqLoc = GetShaderLocation(e->shader, "warpFreq");
  e->warpTimeLoc = GetShaderLocation(e->shader, "warpTime");
  e->blendModeLoc = GetShaderLocation(e->shader, "blendMode");
  e->parallaxStrengthLoc = GetShaderLocation(e->shader, "parallaxStrength");

  e->time = 0.0f;
  e->warpTime = 0.0f;

  return true;
}

void InfiniteZoomEffectSetup(InfiniteZoomEffect *e, InfiniteZoomConfig *cfg,
                             float deltaTime) {
  e->time += cfg->speed * deltaTime;
  e->warpTime += cfg->warpSpeed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomDepthLoc, &cfg->zoomDepth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->spiralAngleLoc, &cfg->spiralAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralTwistLoc, &cfg->spiralTwist,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layerRotateLoc, &cfg->layerRotate,
                 SHADER_UNIFORM_FLOAT);

  // Compute center with lissajous
  float centerX = cfg->centerX;
  float centerY = cfg->centerY;
  if (cfg->centerLissajous.amplitude > 0.0f) {
    float cx, cy;
    DualLissajousUpdate(&cfg->centerLissajous, deltaTime, 0.0f, &cx, &cy);
    centerX += cx;
    centerY += cy;
  }
  float center[2] = {centerX, centerY};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

  // Compute offset with lissajous
  float offX = cfg->offsetX;
  float offY = cfg->offsetY;
  if (cfg->offsetLissajous.amplitude > 0.0f) {
    float ox, oy;
    DualLissajousUpdate(&cfg->offsetLissajous, deltaTime, 0.0f, &ox, &oy);
    offX += ox;
    offY += oy;
  }
  float offset[2] = {offX, offY};
  SetShaderValue(e->shader, e->offsetLoc, offset, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->parallaxStrengthLoc, &cfg->parallaxStrength,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->warpTypeLoc, &cfg->warpType, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->warpStrengthLoc, &cfg->warpStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpFreqLoc, &cfg->warpFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpTimeLoc, &e->warpTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blendModeLoc, &cfg->blendMode,
                 SHADER_UNIFORM_INT);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

void InfiniteZoomEffectUninit(InfiniteZoomEffect *e) {
  UnloadShader(e->shader);
}

InfiniteZoomConfig InfiniteZoomConfigDefault(void) {
  return InfiniteZoomConfig{};
}

void InfiniteZoomRegisterParams(InfiniteZoomConfig *cfg) {
  ModEngineRegisterParam("infiniteZoom.spiralAngle", &cfg->spiralAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("infiniteZoom.spiralTwist", &cfg->spiralTwist,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("infiniteZoom.layerRotate", &cfg->layerRotate,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("infiniteZoom.warpStrength", &cfg->warpStrength, 0.0f,
                         1.0f);
  ModEngineRegisterParam("infiniteZoom.warpFreq", &cfg->warpFreq, 1.0f, 10.0f);
  ModEngineRegisterParam("infiniteZoom.centerX", &cfg->centerX, 0.0f, 1.0f);
  ModEngineRegisterParam("infiniteZoom.centerY", &cfg->centerY, 0.0f, 1.0f);
  ModEngineRegisterParam("infiniteZoom.parallaxStrength",
                         &cfg->parallaxStrength, 0.0f, 5.0f);
  ModEngineRegisterParam("infiniteZoom.offsetX", &cfg->offsetX, -0.1f, 0.1f);
  ModEngineRegisterParam("infiniteZoom.offsetY", &cfg->offsetY, -0.1f, 0.1f);
  ModEngineRegisterParam("infiniteZoom.centerLissajous.amplitude",
                         &cfg->centerLissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("infiniteZoom.centerLissajous.motionSpeed",
                         &cfg->centerLissajous.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("infiniteZoom.offsetLissajous.amplitude",
                         &cfg->offsetLissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("infiniteZoom.offsetLissajous.motionSpeed",
                         &cfg->offsetLissajous.motionSpeed, 0.0f, 10.0f);
}

// === UI ===

static void DrawInfiniteZoomParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  ImGui::SliderFloat("Speed##infzoom", &e->infiniteZoom.speed, -2.0f, 2.0f,
                     "%.2f");
  ImGui::SliderFloat("Zoom Depth##infzoom", &e->infiniteZoom.zoomDepth, 1.0f,
                     5.0f, "%.1f");
  ImGui::SliderInt("Layers##infzoom", &e->infiniteZoom.layers, 2, 8);
  ModulatableSliderAngleDeg("Spiral Angle##infzoom",
                            &e->infiniteZoom.spiralAngle,
                            "infiniteZoom.spiralAngle", ms);
  ModulatableSliderAngleDeg("Twist##infzoom", &e->infiniteZoom.spiralTwist,
                            "infiniteZoom.spiralTwist", ms);
  ModulatableSliderAngleDeg("Layer Rotate##infzoom",
                            &e->infiniteZoom.layerRotate,
                            "infiniteZoom.layerRotate", ms);

  ImGui::SeparatorText("Warp");
  ImGui::Combo("Warp Type##infzoom", &e->infiniteZoom.warpType,
               "None\0Sine\0Noise\0");
  ModulatableSlider("Warp Strength##infzoom", &e->infiniteZoom.warpStrength,
                    "infiniteZoom.warpStrength", "%.2f", ms);
  ModulatableSlider("Warp Freq##infzoom", &e->infiniteZoom.warpFreq,
                    "infiniteZoom.warpFreq", "%.1f", ms);
  ImGui::SliderFloat("Warp Speed##infzoom", &e->infiniteZoom.warpSpeed, -2.0f,
                     2.0f, "%.2f");

  ImGui::SeparatorText("Center");
  ModulatableSlider("Center X##infzoom", &e->infiniteZoom.centerX,
                    "infiniteZoom.centerX", "%.2f", ms);
  ModulatableSlider("Center Y##infzoom", &e->infiniteZoom.centerY,
                    "infiniteZoom.centerY", "%.2f", ms);
  DrawLissajousControls(&e->infiniteZoom.centerLissajous, "infzoom_center",
                        "infiniteZoom.centerLissajous", ms, 5.0f);

  ImGui::SeparatorText("Parallax");
  ModulatableSlider("Parallax Strength##infzoom",
                    &e->infiniteZoom.parallaxStrength,
                    "infiniteZoom.parallaxStrength", "%.2f", ms);
  ModulatableSlider("Offset X##infzoom", &e->infiniteZoom.offsetX,
                    "infiniteZoom.offsetX", "%.3f", ms);
  ModulatableSlider("Offset Y##infzoom", &e->infiniteZoom.offsetY,
                    "infiniteZoom.offsetY", "%.3f", ms);
  DrawLissajousControls(&e->infiniteZoom.offsetLissajous, "infzoom_offset",
                        "infiniteZoom.offsetLissajous", ms, 5.0f);

  ImGui::SeparatorText("Blending");
  ImGui::Combo("Blend Mode##infzoom", &e->infiniteZoom.blendMode,
               "Weighted Avg\0Additive\0Screen\0");
}

void SetupInfiniteZoom(PostEffect *pe) {
  InfiniteZoomEffectSetup(&pe->infiniteZoom, &pe->effects.infiniteZoom,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_INFINITE_ZOOM, InfiniteZoom, infiniteZoom,
                "Infinite Zoom", "MOT", 3, EFFECT_FLAG_NONE,
                SetupInfiniteZoom, NULL, DrawInfiniteZoomParams)
// clang-format on
