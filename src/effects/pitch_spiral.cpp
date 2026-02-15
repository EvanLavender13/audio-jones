// Pitch spiral effect module implementation
// Maps FFT bins onto a logarithmic spiral — one full turn per octave —
// with pitch-class coloring via gradient LUT

#include "pitch_spiral.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
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
  e->numTurnsLoc = GetShaderLocation(e->shader, "numTurns");
  e->spiralSpacingLoc = GetShaderLocation(e->shader, "spiralSpacing");
  e->lineWidthLoc = GetShaderLocation(e->shader, "lineWidth");
  e->blurLoc = GetShaderLocation(e->shader, "blur");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->numOctavesLoc = GetShaderLocation(e->shader, "numOctaves");
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
                            float deltaTime, Texture2D fftTexture) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;
  e->rotationAccum = fmodf(e->rotationAccum, TWO_PI_F);
  if (e->rotationAccum < 0.0f)
    e->rotationAccum += TWO_PI_F;
  e->breathAccum += cfg->breathSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->numTurnsLoc, &cfg->numTurns, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->spiralSpacingLoc, &cfg->spiralSpacing,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineWidthLoc, &cfg->lineWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blurLoc, &cfg->blur, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->numOctavesLoc, &cfg->numOctaves,
                 SHADER_UNIFORM_INT);
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

PitchSpiralConfig PitchSpiralConfigDefault(void) { return PitchSpiralConfig{}; }

void PitchSpiralRegisterParams(PitchSpiralConfig *cfg) {
  ModEngineRegisterParam("pitchSpiral.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
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

// clang-format off
REGISTER_GENERATOR(TRANSFORM_PITCH_SPIRAL_BLEND, PitchSpiral, pitchSpiral,
                   "Pitch Spiral Blend", SetupPitchSpiralBlend,
                   SetupPitchSpiral)
// clang-format on
