// Shell effect module implementation
// Raymarched hollow sphere with outline contours from per-step view rotation

#include "shell.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool ShellEffectInit(ShellEffect *e, const ShellConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/shell.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->turbulenceOctavesLoc = GetShaderLocation(e->shader, "turbulenceOctaves");
  e->turbulenceGrowthLoc = GetShaderLocation(e->shader, "turbulenceGrowth");
  e->sphereRadiusLoc = GetShaderLocation(e->shader, "sphereRadius");
  e->ringThicknessLoc = GetShaderLocation(e->shader, "ringThickness");
  e->cameraDistanceLoc = GetShaderLocation(e->shader, "cameraDistance");
  e->phaseLoc = GetShaderLocation(e->shader, "phase");
  e->outlineSpreadLoc = GetShaderLocation(e->shader, "outlineSpread");
  e->colorStretchLoc = GetShaderLocation(e->shader, "colorStretch");
  e->colorPhaseLoc = GetShaderLocation(e->shader, "colorPhase");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;
  e->colorPhase = 0.0f;

  return true;
}

void ShellEffectSetup(ShellEffect *e, const ShellConfig *cfg, float deltaTime,
                      const Texture2D &fftTexture) {
  e->time += deltaTime;
  e->colorPhase += cfg->colorSpeed * deltaTime;
  e->currentFFTTexture = fftTexture;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->turbulenceOctavesLoc, &cfg->turbulenceOctaves,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->turbulenceGrowthLoc, &cfg->turbulenceGrowth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereRadiusLoc, &cfg->sphereRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ringThicknessLoc, &cfg->ringThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraDistanceLoc, &cfg->cameraDistance,
                 SHADER_UNIFORM_FLOAT);
  const float phase[3] = {cfg->phaseX, cfg->phaseY, cfg->phaseZ};
  SetShaderValue(e->shader, e->phaseLoc, phase, SHADER_UNIFORM_VEC3);
  SetShaderValue(e->shader, e->outlineSpreadLoc, &cfg->outlineSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorPhaseLoc, &e->colorPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorStretchLoc, &cfg->colorStretch,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFFTTexture);
}

void ShellEffectUninit(ShellEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void ShellRegisterParams(ShellConfig *cfg) {
  ModEngineRegisterParam("shell.turbulenceGrowth", &cfg->turbulenceGrowth, 1.2f,
                         3.0f);
  ModEngineRegisterParam("shell.sphereRadius", &cfg->sphereRadius, 0.5f, 10.0f);
  ModEngineRegisterParam("shell.ringThickness", &cfg->ringThickness, 0.01f,
                         0.5f);
  ModEngineRegisterParam("shell.cameraDistance", &cfg->cameraDistance, 3.0f,
                         20.0f);
  ModEngineRegisterParam("shell.phaseX", &cfg->phaseX, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("shell.phaseY", &cfg->phaseY, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("shell.phaseZ", &cfg->phaseZ, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("shell.outlineSpread", &cfg->outlineSpread, 0.0f,
                         0.5f);
  ModEngineRegisterParam("shell.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("shell.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("shell.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("shell.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("shell.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("shell.colorSpeed", &cfg->colorSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("shell.colorStretch", &cfg->colorStretch, 0.1f, 5.0f);
  ModEngineRegisterParam("shell.brightness", &cfg->brightness, 0.1f, 5.0f);
  ModEngineRegisterParam("shell.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupShell(PostEffect *pe) {
  ShellEffectSetup(&pe->shell, &pe->effects.shell, pe->currentDeltaTime,
                   pe->fftTexture);
}

void SetupShellBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.shell.blendIntensity,
                       pe->effects.shell.blendMode);
}

// === UI ===

static void DrawShellParams(EffectConfig *e, const ModSources *modSources,
                            ImU32 categoryGlow) {
  (void)categoryGlow;
  ShellConfig *s = &e->shell;

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("March Steps##shell", &s->marchSteps, 4, 200);
  ImGui::SliderInt("Octaves##shell", &s->turbulenceOctaves, 2, 12);
  ModulatableSlider("Growth##shell", &s->turbulenceGrowth,
                    "shell.turbulenceGrowth", "%.2f", modSources);
  ModulatableSlider("Radius##shell", &s->sphereRadius, "shell.sphereRadius",
                    "%.1f", modSources);
  ModulatableSlider("Thickness##shell", &s->ringThickness,
                    "shell.ringThickness", "%.3f", modSources);
  ModulatableSlider("Camera Dist##shell", &s->cameraDistance,
                    "shell.cameraDistance", "%.1f", modSources);
  ModulatableSliderAngleDeg("Phase X##shell", &s->phaseX, "shell.phaseX",
                            modSources);
  ModulatableSliderAngleDeg("Phase Y##shell", &s->phaseY, "shell.phaseY",
                            modSources);
  ModulatableSliderAngleDeg("Phase Z##shell", &s->phaseZ, "shell.phaseZ",
                            modSources);
  ModulatableSlider("Outline Spread##shell", &s->outlineSpread,
                    "shell.outlineSpread", "%.3f", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##shell", &s->baseFreq, "shell.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##shell", &s->maxFreq, "shell.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##shell", &s->gain, "shell.gain", "%.1f", modSources);
  ModulatableSlider("Contrast##shell", &s->curve, "shell.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##shell", &s->baseBright, "shell.baseBright",
                    "%.2f", modSources);

  // Color
  ImGui::SeparatorText("Color");
  ModulatableSlider("Color Speed##shell", &s->colorSpeed, "shell.colorSpeed",
                    "%.2f", modSources);
  ModulatableSlider("Color Stretch##shell", &s->colorStretch,
                    "shell.colorStretch", "%.2f", modSources);
  ModulatableSlider("Brightness##shell", &s->brightness, "shell.brightness",
                    "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(shell)
REGISTER_GENERATOR(TRANSFORM_SHELL_BLEND, Shell, shell, "Shell",
                   SetupShellBlend, SetupShell, 11,
                   DrawShellParams, DrawOutput_shell)
// clang-format on
