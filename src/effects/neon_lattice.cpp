// Neon Lattice effect module implementation
// Raymarched torus columns on a repeating 3D grid with forward-flying camera
// and glow

#include "neon_lattice.h"
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

// ---------------------------------------------------------------------------
// Init / Setup / Uninit
// ---------------------------------------------------------------------------

static void CacheLocations(NeonLatticeEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->spacingLoc = GetShaderLocation(e->shader, "spacing");
  e->lightSpacingLoc = GetShaderLocation(e->shader, "lightSpacing");
  e->attenuationLoc = GetShaderLocation(e->shader, "attenuation");
  e->glowExponentLoc = GetShaderLocation(e->shader, "glowExponent");
  e->cameraTimeLoc = GetShaderLocation(e->shader, "cameraTime");
  e->columnsTimeLoc = GetShaderLocation(e->shader, "columnsTime");
  e->lightsTimeLoc = GetShaderLocation(e->shader, "lightsTime");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->maxDistLoc = GetShaderLocation(e->shader, "maxDist");
  e->torusRadiusLoc = GetShaderLocation(e->shader, "torusRadius");
  e->torusTubeLoc = GetShaderLocation(e->shader, "torusTube");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->axisCountLoc = GetShaderLocation(e->shader, "axisCount");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
}

bool NeonLatticeEffectInit(NeonLatticeEffect *e, const NeonLatticeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/neon_lattice.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->cameraPhase = 0.0f;
  e->columnsPhase = 0.0f;
  e->lightsPhase = 0.0f;

  return true;
}

void NeonLatticeEffectSetup(NeonLatticeEffect *e, const NeonLatticeConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture) {
  e->cameraPhase += cfg->cameraSpeed * deltaTime;
  e->columnsPhase += cfg->columnsSpeed * deltaTime;
  e->lightsPhase += cfg->lightsSpeed * deltaTime;

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->spacingLoc, &cfg->spacing, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightSpacingLoc, &cfg->lightSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->attenuationLoc, &cfg->attenuation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowExponentLoc, &cfg->glowExponent,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->cameraTimeLoc, &e->cameraPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->columnsTimeLoc, &e->columnsPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lightsTimeLoc, &e->lightsPhase,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->maxDistLoc, &cfg->maxDist, SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->torusRadiusLoc, &cfg->torusRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->torusTubeLoc, &cfg->torusTube,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->axisCountLoc, &cfg->axisCount,
                 SHADER_UNIFORM_INT);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
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
}

void NeonLatticeEffectUninit(NeonLatticeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void NeonLatticeRegisterParams(NeonLatticeConfig *cfg) {
  ModEngineRegisterParam("neonLattice.spacing", &cfg->spacing, 2.0f, 20.0f);
  ModEngineRegisterParam("neonLattice.lightSpacing", &cfg->lightSpacing, 0.5f,
                         5.0f);
  ModEngineRegisterParam("neonLattice.attenuation", &cfg->attenuation, 5.0f,
                         60.0f);
  ModEngineRegisterParam("neonLattice.glowExponent", &cfg->glowExponent, 0.5f,
                         3.0f);
  ModEngineRegisterParam("neonLattice.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("neonLattice.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("neonLattice.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("neonLattice.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("neonLattice.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("neonLattice.cameraSpeed", &cfg->cameraSpeed, 0.0f,
                         5.0f);
  ModEngineRegisterParam("neonLattice.columnsSpeed", &cfg->columnsSpeed, 0.0f,
                         15.0f);
  ModEngineRegisterParam("neonLattice.lightsSpeed", &cfg->lightsSpeed, 0.0f,
                         60.0f);
  ModEngineRegisterParam("neonLattice.maxDist", &cfg->maxDist, 20.0f, 120.0f);
  ModEngineRegisterParam("neonLattice.torusRadius", &cfg->torusRadius, 0.2f,
                         1.5f);
  ModEngineRegisterParam("neonLattice.torusTube", &cfg->torusTube, 0.02f, 0.2f);
  ModEngineRegisterParam("neonLattice.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

NeonLatticeEffect *GetNeonLatticeEffect(PostEffect *pe) {
  return (NeonLatticeEffect *)pe->effectStates[TRANSFORM_NEON_LATTICE_BLEND];
}

void SetupNeonLattice(PostEffect *pe) {
  NeonLatticeEffectSetup(GetNeonLatticeEffect(pe), &pe->effects.neonLattice,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupNeonLatticeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.neonLattice.blendIntensity,
                       pe->effects.neonLattice.blendMode);
}

// === UI ===

static void DrawNeonLatticeParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  NeonLatticeConfig *cfg = &e->neonLattice;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##neonLattice", &cfg->baseFreq,
                    "neonLattice.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##neonLattice", &cfg->maxFreq,
                    "neonLattice.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##neonLattice", &cfg->gain, "neonLattice.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##neonLattice", &cfg->curve, "neonLattice.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##neonLattice", &cfg->baseBright,
                    "neonLattice.baseBright", "%.2f", modSources);

  ImGui::SeparatorText("Geometry");
  int comboIdx = cfg->axisCount - 1;
  if (ImGui::Combo("Axes##neonLattice", &comboIdx,
                   "1 Axis\0 2 Axes\0 3 Axes\0")) {
    cfg->axisCount = comboIdx + 1;
  }
  ModulatableSlider("Spacing##neonLattice", &cfg->spacing,
                    "neonLattice.spacing", "%.1f", modSources);
  ModulatableSlider("Light Spacing##neonLattice", &cfg->lightSpacing,
                    "neonLattice.lightSpacing", "%.2f", modSources);
  ModulatableSlider("Attenuation##neonLattice", &cfg->attenuation,
                    "neonLattice.attenuation", "%.1f", modSources);
  ModulatableSlider("Glow Exponent##neonLattice", &cfg->glowExponent,
                    "neonLattice.glowExponent", "%.2f", modSources);
  ModulatableSlider("Ring Radius##neonLattice", &cfg->torusRadius,
                    "neonLattice.torusRadius", "%.2f", modSources);
  ModulatableSlider("Tube Width##neonLattice", &cfg->torusTube,
                    "neonLattice.torusTube", "%.3f", modSources);

  ImGui::SeparatorText("Animation");
  ModulatableSlider("Camera Speed##neonLattice", &cfg->cameraSpeed,
                    "neonLattice.cameraSpeed", "%.2f", modSources);
  ModulatableSlider("Columns Speed##neonLattice", &cfg->columnsSpeed,
                    "neonLattice.columnsSpeed", "%.1f", modSources);
  ModulatableSlider("Lights Speed##neonLattice", &cfg->lightsSpeed,
                    "neonLattice.lightsSpeed", "%.1f", modSources);

  ImGui::SeparatorText("Quality");
  ImGui::SliderInt("Iterations##neonLattice", &cfg->iterations, 20, 80);
  ModulatableSlider("Max Distance##neonLattice", &cfg->maxDist,
                    "neonLattice.maxDist", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(neonLattice)
REGISTER_GENERATOR(TRANSFORM_NEON_LATTICE_BLEND, NeonLattice, neonLattice,
                   "Neon Lattice", SetupNeonLatticeBlend, SetupNeonLattice, 13,
                   DrawNeonLatticeParams, DrawOutput_neonLattice)
// clang-format on
