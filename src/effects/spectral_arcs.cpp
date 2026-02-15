// Spectral arcs effect module implementation
// Cosmic-style tilted concentric ring arcs driven by FFT semitone energy —
// perspective tilt, cos() multi-arc clipping, per-ring rotation,
// inverse-distance glow

#include "spectral_arcs.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include <stddef.h>

bool SpectralArcsEffectInit(SpectralArcsEffect *e,
                            const SpectralArcsConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/spectral_arcs.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->numOctavesLoc = GetShaderLocation(e->shader, "numOctaves");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->ringScaleLoc = GetShaderLocation(e->shader, "ringScale");
  e->tiltLoc = GetShaderLocation(e->shader, "tilt");
  e->tiltAngleLoc = GetShaderLocation(e->shader, "tiltAngle");
  e->arcWidthLoc = GetShaderLocation(e->shader, "arcWidth");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->glowFalloffLoc = GetShaderLocation(e->shader, "glowFalloff");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->rotationAccumLoc = GetShaderLocation(e->shader, "rotationAccum");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->rotationAccum = 0.0f;

  return true;
}

void SpectralArcsEffectSetup(SpectralArcsEffect *e,
                             const SpectralArcsConfig *cfg, float deltaTime,
                             Texture2D fftTexture) {
  e->rotationAccum += cfg->rotationSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->numOctavesLoc, &cfg->numOctaves,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ringScaleLoc, &cfg->ringScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltLoc, &cfg->tilt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltAngleLoc, &cfg->tiltAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->arcWidthLoc, &cfg->arcWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowFalloffLoc, &cfg->glowFalloff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationAccumLoc, &e->rotationAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SpectralArcsEffectUninit(SpectralArcsEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

SpectralArcsConfig SpectralArcsConfigDefault(void) {
  return SpectralArcsConfig{};
}

void SpectralArcsRegisterParams(SpectralArcsConfig *cfg) {
  ModEngineRegisterParam("spectralArcs.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("spectralArcs.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("spectralArcs.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("spectralArcs.ringScale", &cfg->ringScale, 0.5f,
                         10.0f);
  ModEngineRegisterParam("spectralArcs.tilt", &cfg->tilt, 0.0f, 3.0f);
  ModEngineRegisterParam("spectralArcs.tiltAngle", &cfg->tiltAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("spectralArcs.arcWidth", &cfg->arcWidth, 0.0f, 1.0f);
  ModEngineRegisterParam("spectralArcs.glowIntensity", &cfg->glowIntensity,
                         0.01f, 1.0f);
  ModEngineRegisterParam("spectralArcs.glowFalloff", &cfg->glowFalloff, 1.0f,
                         200.0f);
  ModEngineRegisterParam("spectralArcs.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("spectralArcs.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("spectralArcs.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSpectralArcs(PostEffect *pe) {
  SpectralArcsEffectSetup(&pe->spectralArcs, &pe->effects.spectralArcs,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupSpectralArcsBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.spectralArcs.blendIntensity,
                       pe->effects.spectralArcs.blendMode);
}

// clang-format off
REGISTER_GENERATOR(TRANSFORM_SPECTRAL_ARCS_BLEND, SpectralArcs, spectralArcs,
                   "Spectral Arcs Blend", SetupSpectralArcsBlend,
                   SetupSpectralArcs)
// clang-format on
