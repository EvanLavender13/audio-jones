// Pitch spiral effect module implementation
// Maps FFT bins onto a logarithmic spiral - one full turn per octave -
// with pitch-class coloring via gradient LUT

#include "pitch_spiral.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool PitchSpiralEffectInit(PitchSpiralEffect *e, const PitchSpiralConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/pitch_spiral.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->spiralSpacingLoc = GetShaderLocation(e->shader, "spiralSpacing");
  e->lineWidthLoc = GetShaderLocation(e->shader, "lineWidth");
  e->blurLoc = GetShaderLocation(e->shader, "blur");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->tiltLoc = GetShaderLocation(e->shader, "tilt");
  e->tiltAngleLoc = GetShaderLocation(e->shader, "tiltAngle");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->breathAccumLoc = GetShaderLocation(e->shader, "breathAccum");
  e->breathDepthLoc = GetShaderLocation(e->shader, "breathDepth");
  e->shapeExponentLoc = GetShaderLocation(e->shader, "shapeExponent");

  e->rotationAccum = 0.0f;
  e->breathAccum = 0.0f;

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  return true;
}

void PitchSpiralEffectSetup(PitchSpiralEffect *e, const PitchSpiralConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;
  e->rotationAccum = fmodf(e->rotationAccum, TWO_PI_F);
  if (e->rotationAccum < 0.0f) {
    e->rotationAccum += TWO_PI_F;
  }
  e->breathAccum += cfg->breathSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralSpacingLoc, &cfg->spiralSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineWidthLoc, &cfg->lineWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blurLoc, &cfg->blur, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltLoc, &cfg->tilt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltAngleLoc, &cfg->tiltAngle,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->breathAccumLoc, &e->breathAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->breathDepthLoc, &cfg->breathDepth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shapeExponentLoc, &cfg->shapeExponent,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void PitchSpiralEffectUninit(PitchSpiralEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void PitchSpiralRegisterParams(PitchSpiralConfig *cfg) {
  ModEngineRegisterParam("pitchSpiral.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("pitchSpiral.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("pitchSpiral.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("pitchSpiral.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("pitchSpiral.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("pitchSpiral.spiralSpacing", &cfg->spiralSpacing,
                         0.03f, 0.1f);
  ModEngineRegisterParam("pitchSpiral.lineWidth", &cfg->lineWidth, 0.01f,
                         0.04f);
  ModEngineRegisterParam("pitchSpiral.blur", &cfg->blur, 0.01f, 0.03f);
  ModEngineRegisterParam("pitchSpiral.tilt", &cfg->tilt, 0.0f, 3.0f);
  ModEngineRegisterParam("pitchSpiral.tiltAngle", &cfg->tiltAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("pitchSpiral.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("pitchSpiral.breathSpeed", &cfg->breathSpeed, 0.1f,
                         5.0f);
  ModEngineRegisterParam("pitchSpiral.breathDepth", &cfg->breathDepth, 0.0f,
                         0.5f);
  ModEngineRegisterParam("pitchSpiral.shapeExponent", &cfg->shapeExponent, 0.3f,
                         3.0f);
  ModEngineRegisterParam("pitchSpiral.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupPitchSpiral(PostEffect *pe) {
  PitchSpiralEffectSetup(&pe->pitchSpiral, &pe->effects.pitchSpiral,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupPitchSpiralBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.pitchSpiral.blendIntensity,
                       pe->effects.pitchSpiral.blendMode);
}

// === UI ===

static void DrawPitchSpiralParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  PitchSpiralConfig *ps = &e->pitchSpiral;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##pitchspiral", &ps->baseFreq,
                    "pitchSpiral.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##pitchspiral", &ps->maxFreq,
                    "pitchSpiral.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##pitchspiral", &ps->gain, "pitchSpiral.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##pitchspiral", &ps->curve, "pitchSpiral.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##pitchspiral", &ps->baseBright,
                    "pitchSpiral.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Ring Spacing##pitchspiral", &ps->spiralSpacing,
                    "pitchSpiral.spiralSpacing", "%.3f", modSources);
  ModulatableSlider("Line Width##pitchspiral", &ps->lineWidth,
                    "pitchSpiral.lineWidth", "%.3f", modSources);
  ModulatableSlider("AA Softness##pitchspiral", &ps->blur, "pitchSpiral.blur",
                    "%.3f", modSources);

  // Tilt
  ImGui::SeparatorText("Tilt");
  ModulatableSlider("Tilt##pitchspiral", &ps->tilt, "pitchSpiral.tilt", "%.2f",
                    modSources);
  ModulatableSliderAngleDeg("Tilt Angle##pitchspiral", &ps->tiltAngle,
                            "pitchSpiral.tiltAngle", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##pitchspiral", &ps->rotationSpeed,
                            "pitchSpiral.rotationSpeed", modSources);
  ModulatableSlider("Breath Speed##pitchspiral", &ps->breathSpeed,
                    "pitchSpiral.breathSpeed", "%.2f", modSources);
  ModulatableSlider("Breath Depth##pitchspiral", &ps->breathDepth,
                    "pitchSpiral.breathDepth", "%.3f", modSources);
  ModulatableSlider("Shape Exponent##pitchspiral", &ps->shapeExponent,
                    "pitchSpiral.shapeExponent", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(pitchSpiral)
REGISTER_GENERATOR(TRANSFORM_PITCH_SPIRAL_BLEND, PitchSpiral, pitchSpiral,
                   "Pitch Spiral", SetupPitchSpiralBlend,
                   SetupPitchSpiral, 10, DrawPitchSpiralParams, DrawOutput_pitchSpiral)
// clang-format on
