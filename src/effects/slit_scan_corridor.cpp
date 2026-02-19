// Slit Scan Corridor effect module implementation
// Samples a vertical slit and extrudes it into a perspective corridor via
// ping-pong accumulation with optional display-time rotation

#include "slit_scan_corridor.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
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
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");

  e->dispRotationLoc = GetShaderLocation(e->displayShader, "rotation");
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
  float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);

  SetShaderValue(e->shader, e->slitPositionLoc, &cfg->slitPosition,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->speedDtLoc, &speedDt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->perspectiveLoc, &cfg->perspective,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->slitWidthLoc, &cfg->slitWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
}

void SlitScanCorridorEffectRender(SlitScanCorridorEffect *e, PostEffect *pe) {
  const SlitScanCorridorConfig *cfg = &pe->effects.slitScanCorridor;

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

  // Rotation display pass (skip when rotation is negligible)
  float totalRotation = cfg->rotationAngle + e->rotationAccum;
  if (fabsf(totalRotation) >= 0.001f) {
    SetShaderValue(e->displayShader, e->dispRotationLoc, &totalRotation,
                   SHADER_UNIFORM_FLOAT);

    BeginTextureMode(pe->generatorScratch);
    BeginShaderMode(e->displayShader);
    RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture,
                                  pe->screenWidth, pe->screenHeight);
    EndShaderMode();
    EndTextureMode();
  }
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
  ModEngineRegisterParam("slitScanCorridor.slitWidth", &cfg->slitWidth, 0.001f,
                         0.05f);
  ModEngineRegisterParam("slitScanCorridor.speed", &cfg->speed, 0.1f, 10.0f);
  ModEngineRegisterParam("slitScanCorridor.perspective", &cfg->perspective,
                         0.5f, 8.0f);
  ModEngineRegisterParam("slitScanCorridor.decayHalfLife", &cfg->decayHalfLife,
                         0.1f, 10.0f);
  ModEngineRegisterParam("slitScanCorridor.brightness", &cfg->brightness, 0.1f,
                         3.0f);
  ModEngineRegisterParam("slitScanCorridor.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("slitScanCorridor.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("slitScanCorridor.blendIntensity",
                         &cfg->blendIntensity, 0.0f, 5.0f);
}

// Bridge functions for EffectDescriptor dispatch
void SetupSlitScanCorridor(PostEffect *pe) {
  SlitScanCorridorEffectSetup(&pe->slitScanCorridor,
                              &pe->effects.slitScanCorridor,
                              pe->currentDeltaTime);
}

void SetupSlitScanCorridorBlend(PostEffect *pe) {
  const SlitScanCorridorConfig *cfg = &pe->effects.slitScanCorridor;
  float totalRotation = cfg->rotationAngle + pe->slitScanCorridor.rotationAccum;

  // Use generatorScratch if rotation pass was active, otherwise pingPong
  Texture2D blendTex;
  if (fabsf(totalRotation) >= 0.001f) {
    blendTex = pe->generatorScratch.texture;
  } else {
    blendTex =
        pe->slitScanCorridor.pingPong[pe->slitScanCorridor.readIdx].texture;
  }

  BlendCompositorApply(pe->blendCompositor, blendTex, cfg->blendIntensity,
                       cfg->blendMode);
}

void RenderSlitScanCorridor(PostEffect *pe) {
  SlitScanCorridorEffectRender(&pe->slitScanCorridor, pe);
}

// Manual registration — MOT badge, section 3, blend + needs resize
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
  return &pe->blendCompositor->shader;
}

static Shader *GetScratchShader_slitScanCorridor(PostEffect *pe) {
  return &pe->slitScanCorridor.shader;
}

// clang-format off
static bool reg_slitScanCorridor = EffectDescriptorRegister(
    TRANSFORM_SLIT_SCAN_CORRIDOR_BLEND,
    EffectDescriptor{TRANSFORM_SLIT_SCAN_CORRIDOR_BLEND,
        "Slit Scan Corridor", "MOT", 3,
        offsetof(EffectConfig, slitScanCorridor.enabled),
        (uint8_t)(EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE),
        Init_slitScanCorridor, Uninit_slitScanCorridor,
        Resize_slitScanCorridor, Register_slitScanCorridor,
        GetShader_slitScanCorridor, SetupSlitScanCorridorBlend,
        GetScratchShader_slitScanCorridor, SetupSlitScanCorridor,
        RenderSlitScanCorridor});
// clang-format on
