// Arc strobe effect module implementation
// FFT-driven Lissajous web — octave-mapped line segments with strobe pulsing
// and gradient coloring

#include "arc_strobe.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "render/color_lut.h"
#include <stddef.h>

bool ArcStrobeEffectInit(ArcStrobeEffect *e, const ArcStrobeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/arc_strobe.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->phaseLoc = GetShaderLocation(e->shader, "phase");
  e->amplitudeLoc = GetShaderLocation(e->shader, "amplitude");
  e->orbitOffsetLoc = GetShaderLocation(e->shader, "orbitOffset");
  e->lineThicknessLoc = GetShaderLocation(e->shader, "lineThickness");
  e->freqX1Loc = GetShaderLocation(e->shader, "freqX1");
  e->freqY1Loc = GetShaderLocation(e->shader, "freqY1");
  e->freqX2Loc = GetShaderLocation(e->shader, "freqX2");
  e->freqY2Loc = GetShaderLocation(e->shader, "freqY2");
  e->offsetX2Loc = GetShaderLocation(e->shader, "offsetX2");
  e->offsetY2Loc = GetShaderLocation(e->shader, "offsetY2");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->strobeSpeedLoc = GetShaderLocation(e->shader, "strobeSpeed");
  e->strobeTimeLoc = GetShaderLocation(e->shader, "strobeTime");
  e->strobeDecayLoc = GetShaderLocation(e->shader, "strobeDecay");
  e->strobeBoostLoc = GetShaderLocation(e->shader, "strobeBoost");
  e->strobeStrideLoc = GetShaderLocation(e->shader, "strobeStride");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->numOctavesLoc = GetShaderLocation(e->shader, "numOctaves");
  e->segmentsPerOctaveLoc = GetShaderLocation(e->shader, "segmentsPerOctave");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->strobeTime = 0.0f;

  return true;
}

void ArcStrobeEffectSetup(ArcStrobeEffect *e, ArcStrobeConfig *cfg,
                          float deltaTime, Texture2D fftTexture) {
  cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime;
  e->strobeTime += cfg->strobeSpeed * deltaTime;
  // Wrap to prevent float precision loss at large values
  if (e->strobeTime > 1000.0f)
    e->strobeTime -= 1000.0f;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseLoc, &cfg->lissajous.phase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->amplitudeLoc, &cfg->lissajous.amplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->orbitOffsetLoc, &cfg->orbitOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineThicknessLoc, &cfg->lineThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqX1Loc, &cfg->lissajous.freqX1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqY1Loc, &cfg->lissajous.freqY1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqX2Loc, &cfg->lissajous.freqX2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqY2Loc, &cfg->lissajous.freqY2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetX2Loc, &cfg->lissajous.offsetX2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetY2Loc, &cfg->lissajous.offsetY2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strobeSpeedLoc, &cfg->strobeSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strobeTimeLoc, &e->strobeTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strobeDecayLoc, &cfg->strobeDecay,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strobeBoostLoc, &cfg->strobeBoost,
                 SHADER_UNIFORM_FLOAT);
  int stride = cfg->strobeStride < 1 ? 1 : cfg->strobeStride;
  SetShaderValue(e->shader, e->strobeStrideLoc, &stride, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  int numOctavesInt = (int)cfg->numOctaves;
  SetShaderValue(e->shader, e->numOctavesLoc, &numOctavesInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->segmentsPerOctaveLoc, &cfg->segmentsPerOctave,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void ArcStrobeEffectUninit(ArcStrobeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

ArcStrobeConfig ArcStrobeConfigDefault(void) { return ArcStrobeConfig{}; }

void ArcStrobeRegisterParams(ArcStrobeConfig *cfg) {
  ModEngineRegisterParam("arcStrobe.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.05f, 2.0f);
  ModEngineRegisterParam("arcStrobe.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("arcStrobe.lissajous.offsetX2",
                         &cfg->lissajous.offsetX2, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("arcStrobe.lissajous.offsetY2",
                         &cfg->lissajous.offsetY2, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("arcStrobe.orbitOffset", &cfg->orbitOffset, 0.01f,
                         PI_F);
  ModEngineRegisterParam("arcStrobe.lineThickness", &cfg->lineThickness, 0.001f,
                         0.05f);
  ModEngineRegisterParam("arcStrobe.glowIntensity", &cfg->glowIntensity, 0.5f,
                         10.0f);
  ModEngineRegisterParam("arcStrobe.strobeSpeed", &cfg->strobeSpeed, 0.0f,
                         25.0f);
  ModEngineRegisterParam("arcStrobe.strobeDecay", &cfg->strobeDecay, 5.0f,
                         40.0f);
  ModEngineRegisterParam("arcStrobe.strobeBoost", &cfg->strobeBoost, 0.0f,
                         5.0f);
  ModEngineRegisterParam("arcStrobe.numOctaves", &cfg->numOctaves, 1.0f, 8.0f);
  ModEngineRegisterParam("arcStrobe.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("arcStrobe.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("arcStrobe.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("arcStrobe.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("arcStrobe.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}
