// Pitch spiral effect module implementation
// Maps FFT bins onto a logarithmic spiral — one full turn per octave —
// with pitch-class coloring via gradient LUT

#include "pitch_spiral.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "render/color_lut.h"
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
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  return true;
}

void PitchSpiralEffectSetup(PitchSpiralEffect *e, const PitchSpiralConfig *cfg,
                            float deltaTime, Texture2D fftTexture) {
  (void)deltaTime; // No phase accumulators needed

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

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void PitchSpiralEffectUninit(PitchSpiralEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

PitchSpiralConfig PitchSpiralConfigDefault(void) { return PitchSpiralConfig{}; }

void PitchSpiralRegisterParams(PitchSpiralConfig *cfg) {
  ModEngineRegisterParam("pitchSpiral.baseFreq", &cfg->baseFreq, 55.0f, 440.0f);
  ModEngineRegisterParam("pitchSpiral.spiralSpacing", &cfg->spiralSpacing,
                         0.03f, 0.1f);
  ModEngineRegisterParam("pitchSpiral.lineWidth", &cfg->lineWidth, 0.01f,
                         0.04f);
  ModEngineRegisterParam("pitchSpiral.blur", &cfg->blur, 0.01f, 0.03f);
  ModEngineRegisterParam("pitchSpiral.gain", &cfg->gain, 1.0f, 20.0f);
  ModEngineRegisterParam("pitchSpiral.curve", &cfg->curve, 0.5f, 4.0f);
  ModEngineRegisterParam("pitchSpiral.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}
