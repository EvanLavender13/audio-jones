// CRT display emulation effect module implementation

#include "crt.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
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

// === UI ===

static void DrawCrtParams(EffectConfig *e, const ModSources *ms, ImU32 glow) {
  CrtConfig *c = &e->crt;

  if (TreeNodeAccented("Phosphor Mask##crt", glow)) {
    ImGui::Combo("Mask Mode##crt", &c->maskMode,
                 "Shadow Mask\0Aperture Grille\0");
    ModulatableSlider("Mask Size##crt", &c->maskSize, "crt.maskSize", "%.1f",
                      ms);
    ModulatableSlider("Mask Intensity##crt", &c->maskIntensity,
                      "crt.maskIntensity", "%.2f", ms);
    ImGui::SliderFloat("Mask Border##crt", &c->maskBorder, 0.0f, 1.0f, "%.2f");
    TreeNodeAccentedPop();
  }

  if (TreeNodeAccented("Scanlines##crt", glow)) {
    ModulatableSlider("Scanline Intensity##crt", &c->scanlineIntensity,
                      "crt.scanlineIntensity", "%.2f", ms);
    ImGui::SliderFloat("Scanline Spacing##crt", &c->scanlineSpacing, 1.0f, 8.0f,
                       "%.1f");
    ImGui::SliderFloat("Scanline Sharpness##crt", &c->scanlineSharpness, 0.5f,
                       4.0f, "%.2f");
    ImGui::SliderFloat("Bright Boost##crt", &c->scanlineBrightBoost, 0.0f, 1.0f,
                       "%.2f");
    TreeNodeAccentedPop();
  }

  if (TreeNodeAccented("Curvature##crt", glow)) {
    ImGui::Checkbox("Curvature##crt_enable", &c->curvatureEnabled);
    if (c->curvatureEnabled) {
      ModulatableSlider("Curvature Amount##crt", &c->curvatureAmount,
                        "crt.curvatureAmount", "%.3f", ms);
    }
    TreeNodeAccentedPop();
  }

  if (TreeNodeAccented("Vignette##crt", glow)) {
    ImGui::Checkbox("Vignette##crt_enable", &c->vignetteEnabled);
    if (c->vignetteEnabled) {
      ImGui::SliderFloat("Vignette Exponent##crt", &c->vignetteExponent, 0.1f,
                         1.0f, "%.2f");
    }
    TreeNodeAccentedPop();
  }

  if (TreeNodeAccented("Pulse##crt", glow)) {
    ImGui::Checkbox("Pulse##crt_enable", &c->pulseEnabled);
    if (c->pulseEnabled) {
      ModulatableSlider("Pulse Intensity##crt", &c->pulseIntensity,
                        "crt.pulseIntensity", "%.3f", ms);
      ModulatableSlider("Pulse Speed##crt", &c->pulseSpeed, "crt.pulseSpeed",
                        "%.1f", ms);
      ImGui::SliderFloat("Pulse Width##crt", &c->pulseWidth, 20.0f, 200.0f,
                         "%.0f");
    }
    TreeNodeAccentedPop();
  }
}

void SetupCrt(PostEffect *pe) {
  CrtEffectSetup(&pe->crt, &pe->effects.crt, pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CRT, Crt, crt, "CRT", "RET", 6, EFFECT_FLAG_NONE,
                SetupCrt, NULL, DrawCrtParams)
// clang-format on
