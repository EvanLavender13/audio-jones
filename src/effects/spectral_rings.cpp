// Spectral rings generator implementation
// Dense concentric rings with noise-texture coloring via gradient LUT,
// FFT-reactive brightness, elliptical deformation, and animated motion

#include "spectral_rings.h"
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
#include "render/noise_texture.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool SpectralRingsEffectInit(SpectralRingsEffect *e,
                             const SpectralRingsConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/spectral_rings.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->noiseTexLoc = GetShaderLocation(e->shader, "noiseTex");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->noiseScaleLoc = GetShaderLocation(e->shader, "noiseScale");
  e->qualityLoc = GetShaderLocation(e->shader, "quality");
  e->pulseAccumLoc = GetShaderLocation(e->shader, "pulseAccum");
  e->colorShiftAccumLoc = GetShaderLocation(e->shader, "colorShiftAccum");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->eccentricityLoc = GetShaderLocation(e->shader, "eccentricity");
  e->tiltAngleLoc = GetShaderLocation(e->shader, "tiltAngle");
  e->centerLoc = GetShaderLocation(e->shader, "centerOffset");
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

  e->pulseAccum = 0.0f;
  e->colorShiftAccum = 0.0f;
  e->rotationAccum = 0.0f;

  return true;
}

void SpectralRingsEffectSetup(SpectralRingsEffect *e, SpectralRingsConfig *cfg,
                              float deltaTime, Texture2D fftTexture) {
  e->pulseAccum += cfg->pulseSpeed * deltaTime;
  e->colorShiftAccum += cfg->colorShiftSpeed * deltaTime;
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  // Lissajous drift in normalized coords (shader handles screen centering)
  float lissX = 0.0f, lissY = 0.0f;
  if (cfg->lissajous.amplitude > 0.0f) {
    DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &lissX, &lissY);
  }
  float centerOffset[2] = {lissX, lissY};

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->noiseScaleLoc, &cfg->noiseScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->qualityLoc, &cfg->quality, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->pulseAccumLoc, &e->pulseAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorShiftAccumLoc, &e->colorShiftAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->eccentricityLoc, &cfg->eccentricity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltAngleLoc, &cfg->tiltAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerLoc, centerOffset, SHADER_UNIFORM_VEC2);
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
  SetShaderValueTexture(e->shader, e->noiseTexLoc, NoiseTextureGet());
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
}

void SpectralRingsEffectUninit(SpectralRingsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

SpectralRingsConfig SpectralRingsConfigDefault(void) {
  return SpectralRingsConfig{};
}

void SpectralRingsRegisterParams(SpectralRingsConfig *cfg) {
  ModEngineRegisterParam("spectralRings.eccentricity", &cfg->eccentricity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("spectralRings.tiltAngle", &cfg->tiltAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("spectralRings.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("spectralRings.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("spectralRings.pulseSpeed", &cfg->pulseSpeed, -2.0f,
                         2.0f);
  ModEngineRegisterParam("spectralRings.colorShiftSpeed", &cfg->colorShiftSpeed,
                         -2.0f, 2.0f);
  ModEngineRegisterParam("spectralRings.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("spectralRings.noiseScale", &cfg->noiseScale, 0.05f,
                         2.0f);
  ModEngineRegisterParam("spectralRings.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("spectralRings.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("spectralRings.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("spectralRings.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("spectralRings.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("spectralRings.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSpectralRings(PostEffect *pe) {
  SpectralRingsEffectSetup(&pe->spectralRings, &pe->effects.spectralRings,
                           pe->currentDeltaTime, pe->fftTexture);
}

void SetupSpectralRingsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.spectralRings.blendIntensity,
                       pe->effects.spectralRings.blendMode);
}

// === UI ===

static void DrawSpectralRingsParams(EffectConfig *e,
                                    const ModSources *modSources,
                                    ImU32 categoryGlow) {
  SpectralRingsConfig *cfg = &e->spectralRings;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##spectralrings", &cfg->baseFreq,
                    "spectralRings.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##spectralrings", &cfg->maxFreq,
                    "spectralRings.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##spectralrings", &cfg->gain, "spectralRings.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##spectralrings", &cfg->curve,
                    "spectralRings.curve", "%.2f", modSources);
  ModulatableSlider("Base Bright##spectralrings", &cfg->baseBright,
                    "spectralRings.baseBright", "%.2f", modSources);

  // Shape
  ImGui::SeparatorText("Shape");
  ModulatableSlider("Eccentricity##spectralrings", &cfg->eccentricity,
                    "spectralRings.eccentricity", "%.2f", modSources);
  ModulatableSliderAngleDeg("Tilt Angle##spectralrings", &cfg->tiltAngle,
                            "spectralRings.tiltAngle", modSources);

  // Center Motion
  ImGui::SeparatorText("Center Motion");
  DrawLissajousControls(&cfg->lissajous, "spectralrings",
                        "spectralRings.lissajous", modSources, 0.5f);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Pulse Speed##spectralrings", &cfg->pulseSpeed,
                    "spectralRings.pulseSpeed", "%.2f", modSources);
  ModulatableSlider("Color Shift##spectralrings", &cfg->colorShiftSpeed,
                    "spectralRings.colorShiftSpeed", "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rotation Speed##spectralrings",
                            &cfg->rotationSpeed, "spectralRings.rotationSpeed",
                            modSources);

  // Noise
  ImGui::SeparatorText("Noise");
  ModulatableSliderLog("Noise Scale##spectralrings", &cfg->noiseScale,
                       "spectralRings.noiseScale", "%.3f", modSources);

  // Quality
  ImGui::SeparatorText("Quality");
  ImGui::SliderInt("Iterations##spectralrings", &cfg->quality, 8, 256);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(spectralRings)
REGISTER_GENERATOR(TRANSFORM_SPECTRAL_RINGS_BLEND, SpectralRings, spectralRings,
                   "Spectral Rings", SetupSpectralRingsBlend,
                   SetupSpectralRings, 10, DrawSpectralRingsParams, DrawOutput_spectralRings)
// clang-format on
