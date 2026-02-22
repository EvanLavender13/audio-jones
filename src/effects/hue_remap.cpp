// Hue remap effect module implementation

#include "hue_remap.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool HueRemapEffectInit(HueRemapEffect *e, const HueRemapConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/hue_remap.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->shiftLoc = GetShaderLocation(e->shader, "shift");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "texture1");
  e->blendRadialLoc = GetShaderLocation(e->shader, "blendRadial");
  e->blendAngularLoc = GetShaderLocation(e->shader, "blendAngular");
  e->blendAngularFreqLoc = GetShaderLocation(e->shader, "blendAngularFreq");
  e->blendLinearLoc = GetShaderLocation(e->shader, "blendLinear");
  e->blendLinearAngleLoc = GetShaderLocation(e->shader, "blendLinearAngle");
  e->blendLuminanceLoc = GetShaderLocation(e->shader, "blendLuminance");
  e->blendNoiseLoc = GetShaderLocation(e->shader, "blendNoise");
  e->shiftRadialLoc = GetShaderLocation(e->shader, "shiftRadial");
  e->shiftAngularLoc = GetShaderLocation(e->shader, "shiftAngular");
  e->shiftAngularFreqLoc = GetShaderLocation(e->shader, "shiftAngularFreq");
  e->shiftLinearLoc = GetShaderLocation(e->shader, "shiftLinear");
  e->shiftLinearAngleLoc = GetShaderLocation(e->shader, "shiftLinearAngle");
  e->shiftLuminanceLoc = GetShaderLocation(e->shader, "shiftLuminance");
  e->shiftNoiseLoc = GetShaderLocation(e->shader, "shiftNoise");
  e->noiseScaleLoc = GetShaderLocation(e->shader, "noiseScale");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->shiftModeLoc = GetShaderLocation(e->shader, "shiftMode");

  e->time = 0.0f;

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  return true;
}

void HueRemapEffectSetup(HueRemapEffect *e, const HueRemapConfig *cfg,
                         float deltaTime) {
  e->time += cfg->noiseSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  int sm = cfg->shiftMode ? 1 : 0;
  SetShaderValue(e->shader, e->shiftModeLoc, &sm, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->shiftLoc, &cfg->shift, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);

  float center[2] = {cfg->cx, cfg->cy};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Blend spatial uniforms
  SetShaderValue(e->shader, e->blendRadialLoc, &cfg->blendRadial,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blendAngularLoc, &cfg->blendAngular,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blendAngularFreqLoc, &cfg->blendAngularFreq,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->blendLinearLoc, &cfg->blendLinear,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blendLinearAngleLoc, &cfg->blendLinearAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blendLuminanceLoc, &cfg->blendLuminance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blendNoiseLoc, &cfg->blendNoise,
                 SHADER_UNIFORM_FLOAT);

  // Shift spatial uniforms
  SetShaderValue(e->shader, e->shiftRadialLoc, &cfg->shiftRadial,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shiftAngularLoc, &cfg->shiftAngular,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shiftAngularFreqLoc, &cfg->shiftAngularFreq,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->shiftLinearLoc, &cfg->shiftLinear,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shiftLinearAngleLoc, &cfg->shiftLinearAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shiftLuminanceLoc, &cfg->shiftLuminance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shiftNoiseLoc, &cfg->shiftNoise,
                 SHADER_UNIFORM_FLOAT);

  // Shared noise uniforms
  SetShaderValue(e->shader, e->noiseScaleLoc, &cfg->noiseScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void HueRemapEffectUninit(HueRemapEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

HueRemapConfig HueRemapConfigDefault(void) { return HueRemapConfig{}; }

void HueRemapRegisterParams(HueRemapConfig *cfg) {
  ModEngineRegisterParam("hueRemap.shift", &cfg->shift, 0.0f, 1.0f);
  ModEngineRegisterParam("hueRemap.intensity", &cfg->intensity, 0.0f, 1.0f);
  ModEngineRegisterParam("hueRemap.cx", &cfg->cx, 0.0f, 1.0f);
  ModEngineRegisterParam("hueRemap.cy", &cfg->cy, 0.0f, 1.0f);

  // Blend spatial params
  ModEngineRegisterParam("hueRemap.blendRadial", &cfg->blendRadial, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.blendAngular", &cfg->blendAngular, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.blendLinear", &cfg->blendLinear, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.blendLinearAngle", &cfg->blendLinearAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("hueRemap.blendLuminance", &cfg->blendLuminance, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.blendNoise", &cfg->blendNoise, -1.0f, 1.0f);

  // Shift spatial params
  ModEngineRegisterParam("hueRemap.shiftRadial", &cfg->shiftRadial, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.shiftAngular", &cfg->shiftAngular, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.shiftLinear", &cfg->shiftLinear, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.shiftLinearAngle", &cfg->shiftLinearAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("hueRemap.shiftLuminance", &cfg->shiftLuminance, -1.0f,
                         1.0f);
  ModEngineRegisterParam("hueRemap.shiftNoise", &cfg->shiftNoise, -1.0f, 1.0f);

  // Shared noise params
  ModEngineRegisterParam("hueRemap.noiseScale", &cfg->noiseScale, 1.0f, 20.0f);
  ModEngineRegisterParam("hueRemap.noiseSpeed", &cfg->noiseSpeed, 0.0f, 2.0f);
}

// === UI ===

static void DrawHueRemapParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  HueRemapConfig *hr = &e->hueRemap;

  ImGui::Checkbox("Hue Shift Mode##hueremap", &hr->shiftMode);
  if (!hr->shiftMode) {
    ImGuiDrawColorMode(&hr->gradient);
  }

  ImGui::SeparatorText("Core");
  ModulatableSlider("Shift##hueremap", &hr->shift, "hueRemap.shift", "%.0f °",
                    ms, 360.0f);
  ModulatableSlider("Intensity##hueremap", &hr->intensity, "hueRemap.intensity",
                    "%.2f", ms);
  ModulatableSlider("Center X##hueremap", &hr->cx, "hueRemap.cx", "%.2f", ms);
  ModulatableSlider("Center Y##hueremap", &hr->cy, "hueRemap.cy", "%.2f", ms);

  ImGui::SeparatorText("Blend Spatial");
  ModulatableSlider("Radial##hueremap_blend", &hr->blendRadial,
                    "hueRemap.blendRadial", "%.2f", ms);
  ModulatableSlider("Angular##hueremap_blend", &hr->blendAngular,
                    "hueRemap.blendAngular", "%.2f", ms);
  ImGui::SliderInt("Angular Freq##hueremap_blend", &hr->blendAngularFreq, 1, 8);
  ModulatableSlider("Linear##hueremap_blend", &hr->blendLinear,
                    "hueRemap.blendLinear", "%.2f", ms);
  ModulatableSliderAngleDeg("Linear Angle##hueremap_blend",
                            &hr->blendLinearAngle, "hueRemap.blendLinearAngle",
                            ms);
  ModulatableSlider("Luminance##hueremap_blend", &hr->blendLuminance,
                    "hueRemap.blendLuminance", "%.2f", ms);
  ModulatableSlider("Noise##hueremap_blend", &hr->blendNoise,
                    "hueRemap.blendNoise", "%.2f", ms);

  ImGui::SeparatorText("Shift Spatial");
  ModulatableSlider("Radial##hueremap_shift", &hr->shiftRadial,
                    "hueRemap.shiftRadial", "%.2f", ms);
  ModulatableSlider("Angular##hueremap_shift", &hr->shiftAngular,
                    "hueRemap.shiftAngular", "%.2f", ms);
  ImGui::SliderInt("Angular Freq##hueremap_shift", &hr->shiftAngularFreq, 1, 8);
  ModulatableSlider("Linear##hueremap_shift", &hr->shiftLinear,
                    "hueRemap.shiftLinear", "%.2f", ms);
  ModulatableSliderAngleDeg("Linear Angle##hueremap_shift",
                            &hr->shiftLinearAngle, "hueRemap.shiftLinearAngle",
                            ms);
  ModulatableSlider("Luminance##hueremap_shift", &hr->shiftLuminance,
                    "hueRemap.shiftLuminance", "%.2f", ms);
  ModulatableSlider("Noise##hueremap_shift", &hr->shiftNoise,
                    "hueRemap.shiftNoise", "%.2f", ms);

  ImGui::SeparatorText("Noise Field");
  ModulatableSlider("Scale##hueremap", &hr->noiseScale, "hueRemap.noiseScale",
                    "%.1f", ms);
  ModulatableSlider("Speed##hueremap", &hr->noiseSpeed, "hueRemap.noiseSpeed",
                    "%.2f", ms);
}

void SetupHueRemap(PostEffect *pe) {
  HueRemapEffectSetup(&pe->hueRemap, &pe->effects.hueRemap,
                      pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT_CFG(TRANSFORM_HUE_REMAP, HueRemap, hueRemap, "Hue Remap",
                    "COL", 8, EFFECT_FLAG_NONE, SetupHueRemap, NULL,
                    DrawHueRemapParams)
// clang-format on
