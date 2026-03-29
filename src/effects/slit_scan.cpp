// Slit Scan effect module implementation
// Samples a vertical slit and extrudes it into a perspective corridor or flat
// scroll via ping-pong accumulation with optional display-time rotation

#include "slit_scan.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

static void CacheLocations(SlitScanEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->sceneTextureLoc = GetShaderLocation(e->shader, "sceneTexture");
  e->slitPositionLoc = GetShaderLocation(e->shader, "slitPosition");
  e->speedDtLoc = GetShaderLocation(e->shader, "speedDt");
  e->pushAccelLoc = GetShaderLocation(e->shader, "pushAccel");
  e->slitWidthLoc = GetShaderLocation(e->shader, "slitWidth");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->centerLoc = GetShaderLocation(e->shader, "center");

  e->dispModeLoc = GetShaderLocation(e->displayShader, "mode");
  e->dispRotationLoc = GetShaderLocation(e->displayShader, "rotation");
  e->dispPerspectiveLoc = GetShaderLocation(e->displayShader, "perspective");
  e->dispFogLoc = GetShaderLocation(e->displayShader, "fogStrength");
  e->dispCenterLoc = GetShaderLocation(e->displayShader, "center");
  e->dispGlowLoc = GetShaderLocation(e->displayShader, "glow");
}

static void InitPingPong(SlitScanEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "SLIT_SCAN");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "SLIT_SCAN");
}

static void UnloadPingPong(const SlitScanEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool SlitScanEffectInit(SlitScanEffect *e, const SlitScanConfig *cfg, int width,
                        int height) {
  (void)cfg;

  e->shader = LoadShader(NULL, "shaders/slit_scan.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->displayShader = LoadShader(NULL, "shaders/slit_scan_display.fs");
  if (e->displayShader.id == 0) {
    UnloadShader(e->shader);
    return false;
  }

  CacheLocations(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
  e->rotationAccum = 0.0f;

  return true;
}

void SlitScanEffectSetup(SlitScanEffect *e, const SlitScanConfig *cfg,
                         float deltaTime) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

  float speedDt = cfg->speed * deltaTime;
  float center = (cfg->mode == 0) ? 0.5f : 0.0f;

  SetShaderValue(e->shader, e->slitPositionLoc, &cfg->slitPosition,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->speedDtLoc, &speedDt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pushAccelLoc, &cfg->pushAccel,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->slitWidthLoc, &cfg->slitWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerLoc, &center, SHADER_UNIFORM_FLOAT);

  // Display shader uniforms
  SetShaderValue(e->displayShader, e->dispModeLoc, &cfg->mode,
                 SHADER_UNIFORM_INT);
  float totalRotation = cfg->rotationAngle + e->rotationAccum;
  SetShaderValue(e->displayShader, e->dispRotationLoc, &totalRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispPerspectiveLoc, &cfg->perspective,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispFogLoc, &cfg->fogStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispCenterLoc, &center,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispGlowLoc, &cfg->glow,
                 SHADER_UNIFORM_FLOAT);
}

void SlitScanEffectRender(SlitScanEffect *e, const SlitScanConfig *cfg,
                          const PostEffect *pe) {
  (void)cfg;

  // Resolution uniform for accumulation shader
  const float resolution[2] = {(float)pe->screenWidth, (float)pe->screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Ping-pong accumulation pass
  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  // Texture bindings must be set after BeginTextureMode/BeginShaderMode
  // (both flush raylib's activeTextureId[])
  SetShaderValueTexture(e->shader, e->sceneTextureLoc, pe->currentSceneTexture);

  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture,
                                pe->screenWidth, pe->screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;

  // Output through display shader (perspective + rotation)
  BeginTextureMode(*pe->currentRenderDest);
  BeginShaderMode(e->displayShader);
  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture,
                                pe->screenWidth, pe->screenHeight);
  EndShaderMode();
  EndTextureMode();
}

void SlitScanEffectResize(SlitScanEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void SlitScanEffectUninit(SlitScanEffect *e) {
  UnloadShader(e->shader);
  UnloadShader(e->displayShader);
  UnloadPingPong(e);
}

void SlitScanRegisterParams(SlitScanConfig *cfg) {
  ModEngineRegisterParam("slitScan.slitPosition", &cfg->slitPosition, 0.0f,
                         1.0f);
  ModEngineRegisterParam("slitScan.slitWidth", &cfg->slitWidth, 0.001f, 1.0f);
  ModEngineRegisterParam("slitScan.speed", &cfg->speed, 0.1f, 10.0f);
  ModEngineRegisterParam("slitScan.pushAccel", &cfg->pushAccel, 0.0f, 10.0f);
  ModEngineRegisterParam("slitScan.perspective", &cfg->perspective, 0.0f,
                         10.0f);
  ModEngineRegisterParam("slitScan.fogStrength", &cfg->fogStrength, 0.1f, 5.0f);
  ModEngineRegisterParam("slitScan.brightness", &cfg->brightness, 0.1f, 3.0f);
  ModEngineRegisterParam("slitScan.glow", &cfg->glow, 0.0f, 5.0f);
  ModEngineRegisterParam("slitScan.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("slitScan.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
}

// === UI ===

static void DrawSlitScanParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  ModulatableSlider("Slit Position##slitscan", &e->slitScan.slitPosition,
                    "slitScan.slitPosition", "%.2f", ms);
  ModulatableSliderLog("Slit Width##slitscan", &e->slitScan.slitWidth,
                       "slitScan.slitWidth", "%.3f", ms);
  ImGui::Combo("Mode##slitscan", &e->slitScan.mode, "Corridor\0Flat\0");
  ModulatableSlider("Speed##slitscan", &e->slitScan.speed, "slitScan.speed",
                    "%.1f", ms);
  ModulatableSlider("Push Accel##slitscan", &e->slitScan.pushAccel,
                    "slitScan.pushAccel", "%.1f", ms);
  ModulatableSlider("Brightness##slitscan", &e->slitScan.brightness,
                    "slitScan.brightness", "%.2f", ms);
  if (e->slitScan.mode == 0) {
    ModulatableSlider("Perspective##slitscan", &e->slitScan.perspective,
                      "slitScan.perspective", "%.1f", ms);
    ModulatableSlider("Fog Strength##slitscan", &e->slitScan.fogStrength,
                      "slitScan.fogStrength", "%.1f", ms);
    ModulatableSlider("Glow##slitscan", &e->slitScan.glow, "slitScan.glow",
                      "%.2f", ms);
  }
  ModulatableSliderAngleDeg("Rotation##slitscan", &e->slitScan.rotationAngle,
                            "slitScan.rotationAngle", ms);
  ModulatableSliderSpeedDeg("Rotation Speed##slitscan",
                            &e->slitScan.rotationSpeed,
                            "slitScan.rotationSpeed", ms);
}

// Bridge functions for EffectDescriptor dispatch
void SetupSlitScan(PostEffect *pe) {
  SlitScanEffectSetup(&pe->slitScan, &pe->effects.slitScan,
                      pe->currentDeltaTime);
}

void RenderSlitScan(PostEffect *pe) {
  SlitScanEffectRender(&pe->slitScan, &pe->effects.slitScan, pe);
}

// Manual registration - MOT badge, section 3, needs resize
static bool Init_slitScan(PostEffect *pe, int w, int h) {
  return SlitScanEffectInit(&pe->slitScan, &pe->effects.slitScan, w, h);
}

static void Uninit_slitScan(PostEffect *pe) {
  SlitScanEffectUninit(&pe->slitScan);
}

static void Resize_slitScan(PostEffect *pe, int w, int h) {
  SlitScanEffectResize(&pe->slitScan, w, h);
}

static void Register_slitScan(EffectConfig *cfg) {
  SlitScanRegisterParams(&cfg->slitScan);
}

static Shader *GetShader_slitScan(PostEffect *pe) {
  return &pe->slitScan.shader;
}

// clang-format off
static bool reg_slitScan = EffectDescriptorRegister(
    TRANSFORM_SLIT_SCAN,
    EffectDescriptor{TRANSFORM_SLIT_SCAN,
        "Slit Scan", "MOT", 3,
        offsetof(EffectConfig, slitScan.enabled), "slitScan.",
        EFFECT_FLAG_NEEDS_RESIZE,
        Init_slitScan, Uninit_slitScan,
        Resize_slitScan, Register_slitScan,
        GetShader_slitScan, nullptr,
        nullptr, SetupSlitScan,
        RenderSlitScan,
        DrawSlitScanParams, nullptr});
// clang-format on
