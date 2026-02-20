// Slit Scan Corridor effect module implementation
// Samples a vertical slit and extrudes it into a perspective corridor via
// ping-pong accumulation with optional display-time rotation

#include "slit_scan_corridor.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include <math.h>
#include <stddef.h>

static void CacheLocations(SlitScanCorridorEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->sceneTextureLoc = GetShaderLocation(e->shader, "sceneTexture");
  e->slitPositionLoc = GetShaderLocation(e->shader, "slitPosition");
  e->speedDtLoc = GetShaderLocation(e->shader, "speedDt");
  e->perspectiveLoc = GetShaderLocation(e->shader, "perspective");
  e->slitWidthLoc = GetShaderLocation(e->shader, "slitWidth");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");

  e->dispRotationLoc = GetShaderLocation(e->displayShader, "rotation");
  e->dispPerspectiveLoc = GetShaderLocation(e->displayShader, "perspective");
  e->dispFogLoc = GetShaderLocation(e->displayShader, "fogStrength");
}

static void InitPingPong(SlitScanCorridorEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height,
                            "SLIT_SCAN_CORRIDOR");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height,
                            "SLIT_SCAN_CORRIDOR");
}

static void UnloadPingPong(SlitScanCorridorEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool SlitScanCorridorEffectInit(SlitScanCorridorEffect *e,
                                const SlitScanCorridorConfig *cfg, int width,
                                int height) {
  (void)cfg;

  e->shader = LoadShader(NULL, "shaders/slit_scan_corridor.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->displayShader = LoadShader(NULL, "shaders/slit_scan_corridor_display.fs");
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

void SlitScanCorridorEffectSetup(SlitScanCorridorEffect *e,
                                 const SlitScanCorridorConfig *cfg,
                                 float deltaTime) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

  float speedDt = cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->slitPositionLoc, &cfg->slitPosition,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->speedDtLoc, &speedDt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->perspectiveLoc, &cfg->perspective,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->slitWidthLoc, &cfg->slitWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
}

void SlitScanCorridorEffectRender(SlitScanCorridorEffect *e,
                                  const SlitScanCorridorConfig *cfg,
                                  PostEffect *pe) {
  // Resolution uniform for accumulation shader
  float resolution[2] = {(float)pe->screenWidth, (float)pe->screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Ping-pong accumulation pass
  int writeIdx = 1 - e->readIdx;
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
  float totalRotation = cfg->rotationAngle + e->rotationAccum;
  SetShaderValue(e->displayShader, e->dispRotationLoc, &totalRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispPerspectiveLoc, &cfg->perspective,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispFogLoc, &cfg->fogStrength,
                 SHADER_UNIFORM_FLOAT);

  BeginTextureMode(*pe->currentRenderDest);
  BeginShaderMode(e->displayShader);
  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture,
                                pe->screenWidth, pe->screenHeight);
  EndShaderMode();
  EndTextureMode();
}

void SlitScanCorridorEffectResize(SlitScanCorridorEffect *e, int width,
                                  int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void SlitScanCorridorEffectUninit(SlitScanCorridorEffect *e) {
  UnloadShader(e->shader);
  UnloadShader(e->displayShader);
  UnloadPingPong(e);
}

SlitScanCorridorConfig SlitScanCorridorConfigDefault(void) {
  return SlitScanCorridorConfig{};
}

void SlitScanCorridorRegisterParams(SlitScanCorridorConfig *cfg) {
  ModEngineRegisterParam("slitScanCorridor.slitPosition", &cfg->slitPosition,
                         0.0f, 1.0f);
  ModEngineRegisterParam("slitScanCorridor.slitWidth", &cfg->slitWidth, 1.0f,
                         50.0f);
  ModEngineRegisterParam("slitScanCorridor.speed", &cfg->speed, 0.1f, 10.0f);
  ModEngineRegisterParam("slitScanCorridor.perspective", &cfg->perspective,
                         0.05f, 1.0f);
  ModEngineRegisterParam("slitScanCorridor.fogStrength", &cfg->fogStrength,
                         0.1f, 5.0f);
  ModEngineRegisterParam("slitScanCorridor.brightness", &cfg->brightness, 0.1f,
                         3.0f);
  ModEngineRegisterParam("slitScanCorridor.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("slitScanCorridor.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
}

// Bridge functions for EffectDescriptor dispatch
static void SetupSlitScanCorridor(PostEffect *pe) {
  SlitScanCorridorEffectSetup(&pe->slitScanCorridor,
                              &pe->effects.slitScanCorridor,
                              pe->currentDeltaTime);
}

static void RenderSlitScanCorridor(PostEffect *pe) {
  SlitScanCorridorEffectRender(&pe->slitScanCorridor,
                               &pe->effects.slitScanCorridor, pe);
}

// Manual registration — MOT badge, section 3, needs resize
static bool Init_slitScanCorridor(PostEffect *pe, int w, int h) {
  return SlitScanCorridorEffectInit(&pe->slitScanCorridor,
                                    &pe->effects.slitScanCorridor, w, h);
}

static void Uninit_slitScanCorridor(PostEffect *pe) {
  SlitScanCorridorEffectUninit(&pe->slitScanCorridor);
}

static void Resize_slitScanCorridor(PostEffect *pe, int w, int h) {
  SlitScanCorridorEffectResize(&pe->slitScanCorridor, w, h);
}

static void Register_slitScanCorridor(EffectConfig *cfg) {
  SlitScanCorridorRegisterParams(&cfg->slitScanCorridor);
}

static Shader *GetShader_slitScanCorridor(PostEffect *pe) {
  return &pe->slitScanCorridor.shader;
}

// clang-format off
static bool reg_slitScanCorridor = EffectDescriptorRegister(
    TRANSFORM_SLIT_SCAN_CORRIDOR,
    EffectDescriptor{TRANSFORM_SLIT_SCAN_CORRIDOR,
        "Slit Scan Corridor", "MOT", 3,
        offsetof(EffectConfig, slitScanCorridor.enabled),
        EFFECT_FLAG_NEEDS_RESIZE,
        Init_slitScanCorridor, Uninit_slitScanCorridor,
        Resize_slitScanCorridor, Register_slitScanCorridor,
        GetShader_slitScanCorridor, nullptr,
        nullptr, SetupSlitScanCorridor,
        RenderSlitScanCorridor});
// clang-format on
