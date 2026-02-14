// CRT display emulation effect module implementation

#include "crt.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

static void CacheLocations(CrtEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->maskModeLoc = GetShaderLocation(e->shader, "maskMode");
  e->maskSizeLoc = GetShaderLocation(e->shader, "maskSize");
  e->maskIntensityLoc = GetShaderLocation(e->shader, "maskIntensity");
  e->maskBorderLoc = GetShaderLocation(e->shader, "maskBorder");
  e->scanlineIntensityLoc = GetShaderLocation(e->shader, "scanlineIntensity");
  e->scanlineSpacingLoc = GetShaderLocation(e->shader, "scanlineSpacing");
  e->scanlineSharpnessLoc = GetShaderLocation(e->shader, "scanlineSharpness");
  e->scanlineBrightBoostLoc =
      GetShaderLocation(e->shader, "scanlineBrightBoost");
  e->curvatureEnabledLoc = GetShaderLocation(e->shader, "curvatureEnabled");
  e->curvatureAmountLoc = GetShaderLocation(e->shader, "curvatureAmount");
  e->vignetteEnabledLoc = GetShaderLocation(e->shader, "vignetteEnabled");
  e->vignetteExponentLoc = GetShaderLocation(e->shader, "vignetteExponent");
  e->pulseEnabledLoc = GetShaderLocation(e->shader, "pulseEnabled");
  e->pulseIntensityLoc = GetShaderLocation(e->shader, "pulseIntensity");
  e->pulseWidthLoc = GetShaderLocation(e->shader, "pulseWidth");
  e->pulseSpeedLoc = GetShaderLocation(e->shader, "pulseSpeed");
}

bool CrtEffectInit(CrtEffect *e) {
  e->shader = LoadShader(NULL, "shaders/crt.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);
  e->time = 0.0f;

  return true;
}

static void SetupMask(CrtEffect *e, const CrtConfig *cfg) {
  SetShaderValue(e->shader, e->maskModeLoc, &cfg->maskMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->maskSizeLoc, &cfg->maskSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maskIntensityLoc, &cfg->maskIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maskBorderLoc, &cfg->maskBorder,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupScanlines(CrtEffect *e, const CrtConfig *cfg) {
  SetShaderValue(e->shader, e->scanlineIntensityLoc, &cfg->scanlineIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scanlineSpacingLoc, &cfg->scanlineSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scanlineSharpnessLoc, &cfg->scanlineSharpness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scanlineBrightBoostLoc,
                 &cfg->scanlineBrightBoost, SHADER_UNIFORM_FLOAT);
}

static void SetupCurvature(CrtEffect *e, const CrtConfig *cfg) {
  int curvatureEnabled = cfg->curvatureEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->curvatureEnabledLoc, &curvatureEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->curvatureAmountLoc, &cfg->curvatureAmount,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupVignette(CrtEffect *e, const CrtConfig *cfg) {
  int vignetteEnabled = cfg->vignetteEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->vignetteEnabledLoc, &vignetteEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->vignetteExponentLoc, &cfg->vignetteExponent,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupPulse(CrtEffect *e, const CrtConfig *cfg) {
  int pulseEnabled = cfg->pulseEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->pulseEnabledLoc, &pulseEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->pulseIntensityLoc, &cfg->pulseIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulseWidthLoc, &cfg->pulseWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->pulseSpeedLoc, &cfg->pulseSpeed,
                 SHADER_UNIFORM_FLOAT);
}

void CrtEffectSetup(CrtEffect *e, const CrtConfig *cfg, float deltaTime) {
  e->time += deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetupMask(e, cfg);
  SetupScanlines(e, cfg);
  SetupCurvature(e, cfg);
  SetupVignette(e, cfg);
  SetupPulse(e, cfg);
}

void CrtEffectUninit(CrtEffect *e) { UnloadShader(e->shader); }

CrtConfig CrtConfigDefault(void) { return CrtConfig{}; }

void CrtRegisterParams(CrtConfig *cfg) {
  ModEngineRegisterParam("crt.maskSize", &cfg->maskSize, 2.0f, 24.0f);
  ModEngineRegisterParam("crt.maskIntensity", &cfg->maskIntensity, 0.0f, 1.0f);
  ModEngineRegisterParam("crt.scanlineIntensity", &cfg->scanlineIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("crt.curvatureAmount", &cfg->curvatureAmount, 0.0f,
                         0.3f);
  ModEngineRegisterParam("crt.pulseIntensity", &cfg->pulseIntensity, 0.0f,
                         0.1f);
  ModEngineRegisterParam("crt.pulseSpeed", &cfg->pulseSpeed, 1.0f, 40.0f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CRT, Crt, crt, "CRT", "RET", 6, EFFECT_FLAG_NONE,
                SetupCrt, NULL)
// clang-format on
