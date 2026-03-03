// Byzantine effect module
// Dual-pass cellular automaton with zoom-reseed cycles and FFT-driven display

#ifndef BYZANTINE_H
#define BYZANTINE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ByzantineConfig {
  bool enabled = false;

  // Simulation
  float diffusionWeight = 0.368f; // Even-frame center weight (0.1-0.9)
  float sharpenWeight = 3.0f;     // Odd-frame center weight (1.5-5.0)
  float cycleLength = 360.0f;     // Frames between zoom reseeds (60-600)
  float zoomAmount = 2.0f;        // Zoom factor per reseed (1.2-4.0)
  float centerX = 0.5f;           // Zoom focus X in UV (0.0-1.0)
  float centerY = 0.5f;           // Zoom focus Y in UV (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;   // Low FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // High FFT frequency Hz (1000-16000)
  float gain = 1.0f;        // FFT energy multiplier (0.1-10.0)
  float curve = 1.0f;       // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.3f;  // Minimum brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define BYZANTINE_CONFIG_FIELDS                                                \
  enabled, diffusionWeight, sharpenWeight, cycleLength, zoomAmount, centerX,   \
      centerY, baseFreq, maxFreq, gain, curve, baseBright, gradient,           \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;
typedef struct PostEffect PostEffect;

typedef struct ByzantineEffect {
  Shader shader;        // Simulation shader (ping-pong)
  Shader displayShader; // Display pass (counter-zoom + LUT + FFT)
  RenderTexture2D pingPong[2];
  int readIdx;
  int frameCount;
  ColorLUT *gradientLUT;
  Texture2D currentFFTTexture;

  // Sim shader locations
  int simResolutionLoc;
  int simFrameCountLoc;
  int simCycleLengthLoc;
  int simDiffusionWeightLoc;
  int simSharpenWeightLoc;
  int simZoomAmountLoc;
  int simCenterLoc;

  // Display shader locations
  int dispResolutionLoc;
  int dispFrameCountLoc;
  int dispCycleLengthLoc;
  int dispZoomAmountLoc;
  int dispCenterLoc;
  int dispGradientLUTLoc;
  int dispFftTextureLoc;
  int dispSampleRateLoc;
  int dispBaseFreqLoc;
  int dispMaxFreqLoc;
  int dispGainLoc;
  int dispCurveLoc;
  int dispBaseBrightLoc;
} ByzantineEffect;

// Returns true on success, false if shader fails to load
bool ByzantineEffectInit(ByzantineEffect *e, const ByzantineConfig *cfg,
                         int width, int height);

// Binds all uniforms, advances frame counter, updates LUT texture
void ByzantineEffectSetup(ByzantineEffect *e, const ByzantineConfig *cfg,
                          float deltaTime, Texture2D fftTexture);

// Runs simulation step and display pass into post-effect chain
void ByzantineEffectRender(ByzantineEffect *e, PostEffect *pe);

// Reallocates ping-pong render textures on resolution change
void ByzantineEffectResize(ByzantineEffect *e, int width, int height);

// Unloads shaders and frees LUT
void ByzantineEffectUninit(ByzantineEffect *e);

// Returns default config
ByzantineConfig ByzantineConfigDefault(void);

// Registers modulatable params with the modulation engine
void ByzantineRegisterParams(ByzantineConfig *cfg);

#endif // BYZANTINE_H
