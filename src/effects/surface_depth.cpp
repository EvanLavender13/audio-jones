// Surface Depth — parallax occlusion and relief lighting from luminance height

#include "surface_depth.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool SurfaceDepthEffectInit(SurfaceDepthEffect *e) {
  e->shader = LoadShader(NULL, "shaders/surface_depth.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->depthModeLoc = GetShaderLocation(e->shader, "depthMode");
  e->heightScaleLoc = GetShaderLocation(e->shader, "heightScale");
  e->heightPowerLoc = GetShaderLocation(e->shader, "heightPower");
  e->stepsLoc = GetShaderLocation(e->shader, "steps");
  e->viewAngleLoc = GetShaderLocation(e->shader, "viewAngle");
  e->lightingLoc = GetShaderLocation(e->shader, "lighting");
  e->lightAngleLoc = GetShaderLocation(e->shader, "lightAngle");
  e->lightHeightLoc = GetShaderLocation(e->shader, "lightHeight");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->reliefScaleLoc = GetShaderLocation(e->shader, "reliefScale");
  e->shininessLoc = GetShaderLocation(e->shader, "shininess");
  e->fresnelEnabledLoc = GetShaderLocation(e->shader, "fresnelEnabled");
  e->fresnelExponentLoc = GetShaderLocation(e->shader, "fresnelExponent");
  e->fresnelIntensityLoc = GetShaderLocation(e->shader, "fresnelIntensity");
  e->timeLoc = GetShaderLocation(e->shader, "time");

  e->time = 0.0f;

  return true;
}

void SurfaceDepthEffectSetup(SurfaceDepthEffect *e, SurfaceDepthConfig *cfg,
                             float deltaTime) {
  e->time += deltaTime;

  // Compute effective view angle with lissajous
  float effectiveX = cfg->viewAngleX;
  float effectiveY = cfg->viewAngleY;
  if (cfg->viewLissajous.amplitude > 0.0f) {
    float lx;
    float ly;
    DualLissajousUpdate(&cfg->viewLissajous, deltaTime, 0.0f, &lx, &ly);
    effectiveX += lx;
    effectiveY += ly;
  }

  float viewAngle[2] = {effectiveX, effectiveY};
  SetShaderValue(e->shader, e->viewAngleLoc, viewAngle, SHADER_UNIFORM_VEC2);

  int lightingInt = cfg->lighting ? 1 : 0;
  SetShaderValue(e->shader, e->lightingLoc, &lightingInt, SHADER_UNIFORM_INT);

  int fresnelInt = cfg->fresnel ? 1 : 0;
  SetShaderValue(e->shader, e->fresnelEnabledLoc, &fresnelInt,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->stepsLoc, &cfg->steps, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->depthModeLoc, &cfg->depthMode,
                 SHADER_UNIFORM_INT);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->heightScaleLoc, &cfg->heightScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->heightPowerLoc, &cfg->heightPower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightAngleLoc, &cfg->lightAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightHeightLoc, &cfg->lightHeight,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->reliefScaleLoc, &cfg->reliefScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shininessLoc, &cfg->shininess,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fresnelExponentLoc, &cfg->fresnelExponent,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fresnelIntensityLoc, &cfg->fresnelIntensity,
                 SHADER_UNIFORM_FLOAT);
}

void SurfaceDepthEffectUninit(SurfaceDepthEffect *e) {
  UnloadShader(e->shader);
}

void SurfaceDepthRegisterParams(SurfaceDepthConfig *cfg) {
  ModEngineRegisterParam("surfaceDepth.heightScale", &cfg->heightScale, 0.0f,
                         0.5f);
  ModEngineRegisterParam("surfaceDepth.heightPower", &cfg->heightPower, 0.5f,
                         3.0f);
  ModEngineRegisterParam("surfaceDepth.viewAngleX", &cfg->viewAngleX, -1.0f,
                         1.0f);
  ModEngineRegisterParam("surfaceDepth.viewAngleY", &cfg->viewAngleY, -1.0f,
                         1.0f);
  ModEngineRegisterParam("surfaceDepth.viewLissajous.amplitude",
                         &cfg->viewLissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("surfaceDepth.viewLissajous.motionSpeed",
                         &cfg->viewLissajous.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("surfaceDepth.lightAngle", &cfg->lightAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("surfaceDepth.intensity", &cfg->intensity, 0.0f, 1.0f);
  ModEngineRegisterParam("surfaceDepth.fresnelIntensity",
                         &cfg->fresnelIntensity, 0.0f, 3.0f);
}

// === UI ===

static void DrawSurfaceDepthParams(EffectConfig *e, const ModSources *ms,
                                   ImU32 glow) {
  (void)glow;
  SurfaceDepthConfig *s = &e->surfaceDepth;

  const bool hasDepth = s->depthMode > 0;

  ImGui::Combo("Depth Method##surfDepth", &s->depthMode, "Flat\0Simple\0POM\0");

  if (hasDepth) {
    ModulatableSlider("Height Scale##surfDepth", &s->heightScale,
                      "surfaceDepth.heightScale", "%.2f", ms);
    ModulatableSlider("Height Power##surfDepth", &s->heightPower,
                      "surfaceDepth.heightPower", "%.2f", ms);
    ImGui::SliderInt("Steps##surfDepth", &s->steps, 8, 64);

    ImGui::SeparatorText("View");
    ModulatableSlider("View X##surfDepth", &s->viewAngleX,
                      "surfaceDepth.viewAngleX", "%.2f", ms);
    ModulatableSlider("View Y##surfDepth", &s->viewAngleY,
                      "surfaceDepth.viewAngleY", "%.2f", ms);
    DrawLissajousControls(&s->viewLissajous, "surfDepth_view",
                          "surfaceDepth.viewLissajous", ms, 5.0f);
  }

  ImGui::SeparatorText("Lighting");
  ImGui::Checkbox("Lighting##surfDepth", &s->lighting);
  if (s->lighting) {
    ModulatableSliderAngleDeg("Light Angle##surfDepth", &s->lightAngle,
                              "surfaceDepth.lightAngle", ms);
    ImGui::SliderFloat("Light Height##surfDepth", &s->lightHeight, 0.1f, 2.0f,
                       "%.2f");
    ModulatableSlider("Intensity##surfDepth", &s->intensity,
                      "surfaceDepth.intensity", "%.2f", ms);
    ImGui::SliderFloat("Relief Scale##surfDepth", &s->reliefScale, 0.02f, 1.0f,
                       "%.2f");
    ImGui::SliderFloat("Shininess##surfDepth", &s->shininess, 1.0f, 128.0f,
                       "%.0f");
  }

  ImGui::SeparatorText("Fresnel");
  ImGui::Checkbox("Fresnel##surfDepth", &s->fresnel);
  if (s->fresnel) {
    ImGui::SliderFloat("Exponent##surfDepth", &s->fresnelExponent, 1.0f, 10.0f,
                       "%.1f");
    ModulatableSlider("Glow##surfDepth", &s->fresnelIntensity,
                      "surfaceDepth.fresnelIntensity", "%.2f", ms);
  }
}

void SetupSurfaceDepth(PostEffect *pe) {
  SurfaceDepthEffectSetup(&pe->surfaceDepth, &pe->effects.surfaceDepth,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_SURFACE_DEPTH, SurfaceDepth, surfaceDepth,
                "Surface Depth", "OPT", 7, EFFECT_FLAG_NONE,
                SetupSurfaceDepth, NULL, DrawSurfaceDepthParams)
// clang-format on
