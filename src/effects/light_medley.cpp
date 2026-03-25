// Light Medley effect module implementation
// Fly-through raymarcher rendering a warped crystalline lattice with
// FFT-reactive glow and gradient coloring

#include "light_medley.h"
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

bool LightMedleyEffectInit(LightMedleyEffect *e, const LightMedleyConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/light_medley.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->flyPhaseLoc = GetShaderLocation(e->shader, "flyPhase");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->slabModeLoc = GetShaderLocation(e->shader, "slabMode");
  e->latticeModeLoc = GetShaderLocation(e->shader, "latticeMode");
  e->swirlPermLoc = GetShaderLocation(e->shader, "swirlPerm");
  e->swirlFuncLoc = GetShaderLocation(e->shader, "swirlFunc");
  e->swirlAmountLoc = GetShaderLocation(e->shader, "swirlAmount");
  e->swirlRateLoc = GetShaderLocation(e->shader, "swirlRate");
  e->swirlPhaseLoc = GetShaderLocation(e->shader, "swirlPhase");
  e->twistRateLoc = GetShaderLocation(e->shader, "twistRate");
  e->slabFreqLoc = GetShaderLocation(e->shader, "slabFreq");
  e->latticeScaleLoc = GetShaderLocation(e->shader, "latticeScale");
  e->panLoc = GetShaderLocation(e->shader, "pan");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->swirlPhase = 0.0f;
  e->flyPhase = 0.0f;

  return true;
}

void LightMedleyEffectSetup(LightMedleyEffect *e, LightMedleyConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture) {
  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  e->swirlPhase += cfg->swirlTimeRate * deltaTime;
  e->flyPhase += cfg->flySpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flyPhaseLoc, &e->flyPhase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->slabModeLoc, &cfg->slabMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->latticeModeLoc, &cfg->latticeMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->swirlPermLoc, &cfg->swirlPerm,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->swirlFuncLoc, &cfg->swirlFunc,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->swirlAmountLoc, &cfg->swirlAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->swirlRateLoc, &cfg->swirlRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->swirlPhaseLoc, &e->swirlPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistRateLoc, &cfg->twistRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->slabFreqLoc, &cfg->slabFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->latticeScaleLoc, &cfg->latticeScale,
                 SHADER_UNIFORM_FLOAT);

  // Camera pan from CPU-side Lissajous
  float panX;
  float panY;
  DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &panX, &panY);
  const float pan[2] = {panX, panY};
  SetShaderValue(e->shader, e->panLoc, pan, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void LightMedleyEffectUninit(LightMedleyEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void LightMedleyRegisterParams(LightMedleyConfig *cfg) {
  ModEngineRegisterParam("lightMedley.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("lightMedley.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("lightMedley.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("lightMedley.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("lightMedley.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("lightMedley.swirlAmount", &cfg->swirlAmount, 0.0f,
                         10.0f);
  ModEngineRegisterParam("lightMedley.swirlRate", &cfg->swirlRate, 0.1f, 2.0f);
  ModEngineRegisterParam("lightMedley.swirlTimeRate", &cfg->swirlTimeRate,
                         0.01f, 0.5f);
  ModEngineRegisterParam("lightMedley.twistRate", &cfg->twistRate,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("lightMedley.flySpeed", &cfg->flySpeed, 0.1f, 3.0f);
  ModEngineRegisterParam("lightMedley.slabFreq", &cfg->slabFreq, 0.5f, 4.0f);
  ModEngineRegisterParam("lightMedley.latticeScale", &cfg->latticeScale, 0.5f,
                         4.0f);
  ModEngineRegisterParam("lightMedley.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 2.0f);
  ModEngineRegisterParam("lightMedley.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("lightMedley.glowIntensity", &cfg->glowIntensity, 0.1f,
                         5.0f);
  ModEngineRegisterParam("lightMedley.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupLightMedley(PostEffect *pe) {
  LightMedleyEffectSetup(&pe->lightMedley, &pe->effects.lightMedley,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupLightMedleyBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.lightMedley.blendIntensity,
                       pe->effects.lightMedley.blendMode);
}

// === UI ===

static void DrawLightMedleyParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  LightMedleyConfig *lm = &e->lightMedley;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##lightMedley", &lm->baseFreq,
                    "lightMedley.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##lightMedley", &lm->maxFreq,
                    "lightMedley.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##lightMedley", &lm->gain, "lightMedley.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##lightMedley", &lm->curve, "lightMedley.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##lightMedley", &lm->baseBright,
                    "lightMedley.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::Combo("Slab Shape##lightMedley", &lm->slabMode,
               "X Planes\0Y Planes\0Z Planes\0XY Radial\0XZ "
               "Radial\0Spherical\0Diagonal\0");
  ImGui::Combo("Lattice##lightMedley", &lm->latticeMode,
               "Octahedral\0Cubic\0Cylindrical\0");
  ImGui::Combo("Swirl Perm##lightMedley", &lm->swirlPerm, "YZX\0ZXY\0XZY\0");
  ImGui::Combo("Swirl Func##lightMedley", &lm->swirlFunc,
               "Cosine\0Sine\0Triangle\0Rectified\0");
  ModulatableSlider("Swirl Amount##lightMedley", &lm->swirlAmount,
                    "lightMedley.swirlAmount", "%.1f", modSources);
  ModulatableSlider("Swirl Freq##lightMedley", &lm->swirlRate,
                    "lightMedley.swirlRate", "%.2f", modSources);
  ModulatableSlider("Swirl Speed##lightMedley", &lm->swirlTimeRate,
                    "lightMedley.swirlTimeRate", "%.3f", modSources);
  ModulatableSliderAngleDeg("Twist##lightMedley", &lm->twistRate,
                            "lightMedley.twistRate", modSources);
  ModulatableSlider("Fly Speed##lightMedley", &lm->flySpeed,
                    "lightMedley.flySpeed", "%.2f", modSources);
  ModulatableSlider("Slab Freq##lightMedley", &lm->slabFreq,
                    "lightMedley.slabFreq", "%.2f", modSources);
  ModulatableSlider("Lattice Scale##lightMedley", &lm->latticeScale,
                    "lightMedley.latticeScale", "%.2f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  DrawLissajousControls(&lm->lissajous, "lm_cam", "lightMedley.lissajous",
                        modSources, 2.0f);

  // Output
  ImGui::SeparatorText("Output");
  ModulatableSlider("Glow Intensity##lightMedley", &lm->glowIntensity,
                    "lightMedley.glowIntensity", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(lightMedley)
REGISTER_GENERATOR(TRANSFORM_LIGHT_MEDLEY_BLEND, LightMedley, lightMedley, "Light Medley",
                   SetupLightMedleyBlend, SetupLightMedley, 13, DrawLightMedleyParams, DrawOutput_lightMedley)
// clang-format on
