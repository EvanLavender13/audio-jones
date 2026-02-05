// Interference effect module implementation
// Overlapping wave emitters create ripple patterns via constructive/destructive
// interference

#include "interference.h"
#include "render/color_lut.h"
#include <math.h>
#include <stddef.h>

static const float TWO_PI = 6.28318530718f;

bool InterferenceEffectInit(InterferenceEffect *e,
                            const InterferenceConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/interference.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->sourcesLoc = GetShaderLocation(e->shader, "sources");
  e->phasesLoc = GetShaderLocation(e->shader, "phases");
  e->sourceCountLoc = GetShaderLocation(e->shader, "sourceCount");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->falloffTypeLoc = GetShaderLocation(e->shader, "falloffType");
  e->falloffStrengthLoc = GetShaderLocation(e->shader, "falloffStrength");
  e->boundariesLoc = GetShaderLocation(e->shader, "boundaries");
  e->reflectionGainLoc = GetShaderLocation(e->shader, "reflectionGain");
  e->visualModeLoc = GetShaderLocation(e->shader, "visualMode");
  e->contourCountLoc = GetShaderLocation(e->shader, "contourCount");
  e->visualGainLoc = GetShaderLocation(e->shader, "visualGain");
  e->chromaSpreadLoc = GetShaderLocation(e->shader, "chromaSpread");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->colorLUTLoc = GetShaderLocation(e->shader, "colorLUT");

  e->colorLUT = ColorLUTInit(&cfg->color);
  if (e->colorLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->time = 0.0f;

  return true;
}

void InterferenceEffectSetup(InterferenceEffect *e, InterferenceConfig *cfg,
                             float deltaTime) {
  e->time += cfg->waveSpeed * deltaTime;

  // Compute animated source positions with circular distribution
  float sources[16]; // 8 sources * 2 components (x, y)
  float phases[8];   // Per-source phase offsets
  const int count = cfg->sourceCount > 8 ? 8 : cfg->sourceCount;
  DualLissajousUpdateCircular(&cfg->lissajous, deltaTime, cfg->baseRadius, 0.0f,
                              0.0f, count, sources);

  // Compute per-source phase offsets for shader
  for (int i = 0; i < count; i++) {
    phases[i] = (float)i / (float)count * TWO_PI;
  }

  // Update color LUT from gradient config
  ColorLUTUpdate(e->colorLUT, &cfg->color);

  // Resolution
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Time
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  // Source array uniforms
  SetShaderValueV(e->shader, e->sourcesLoc, sources, SHADER_UNIFORM_VEC2,
                  count);
  SetShaderValueV(e->shader, e->phasesLoc, phases, SHADER_UNIFORM_FLOAT, count);
  SetShaderValue(e->shader, e->sourceCountLoc, &count, SHADER_UNIFORM_INT);

  // Wave parameters
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);

  // Falloff parameters
  SetShaderValue(e->shader, e->falloffTypeLoc, &cfg->falloffType,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffStrengthLoc, &cfg->falloffStrength,
                 SHADER_UNIFORM_FLOAT);

  // Boundary parameters
  int boundariesInt = cfg->boundaries ? 1 : 0;
  SetShaderValue(e->shader, e->boundariesLoc, &boundariesInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->reflectionGainLoc, &cfg->reflectionGain,
                 SHADER_UNIFORM_FLOAT);

  // Visualization parameters
  SetShaderValue(e->shader, e->visualModeLoc, &cfg->visualMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->contourCountLoc, &cfg->contourCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->visualGainLoc, &cfg->visualGain,
                 SHADER_UNIFORM_FLOAT);

  // Chroma spread (for chromatic color mode)
  SetShaderValue(e->shader, e->chromaSpreadLoc, &cfg->chromaSpread,
                 SHADER_UNIFORM_FLOAT);

  // Color mode
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);

  // Bind color LUT texture
  SetShaderValueTexture(e->shader, e->colorLUTLoc,
                        ColorLUTGetTexture(e->colorLUT));
}

void InterferenceEffectUninit(InterferenceEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->colorLUT);
}

InterferenceConfig InterferenceConfigDefault(void) {
  return InterferenceConfig{};
}
