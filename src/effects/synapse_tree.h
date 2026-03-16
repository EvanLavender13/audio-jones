// Synapse Tree: raymarched sphere-inversion fractal tree with synapse pulses

#ifndef SYNAPSE_TREE_H
#define SYNAPSE_TREE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SynapseTreeConfig {
  bool enabled = false;

  // Raymarching
  int marchSteps = 120;   // Ray budget (40-200)
  int foldIterations = 8; // Branching depth (4-12)
  float fov = 1.8f;       // Field of view multiplier (1.0-3.0)

  // Fractal shape
  float foldOffset = 0.5f;       // Abs-fold translation (0.2-1.0)
  float yFold = 1.78f;           // Vertical fold height (1.0-3.0)
  float branchThickness = 0.06f; // Cylinder radius (0.01-0.2)

  // Animation
  float animSpeed = 0.4f;  // Fractal morph rate (0.0-2.0)
  float driftSpeed = 0.1f; // Forward camera drift (0.0-1.0)

  // Synapse
  float synapseIntensity = 0.5f;   // Pulse brightness (0.0-2.0)
  float synapseBounceFreq = 11.0f; // Vertical bounce speed (1.0-20.0)
  float synapsePulseFreq = 7.0f;   // Size oscillation speed (1.0-15.0)

  // Trail persistence
  float decayHalfLife = 2.0f; // Trail persistence seconds (0.1-10.0)
  float trailBlur = 0.5f;     // Trail blur amount (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Min brightness floor (0.0-1.0)

  // Color
  float colorSpeed = 0.3f;   // LUT scroll rate (0.0-2.0)
  float colorStretch = 1.0f; // Spatial color frequency (0.1-5.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Tonemap
  float brightness = 1.0f; // Pre-tonemap multiplier (0.1-5.0)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define SYNAPSE_TREE_CONFIG_FIELDS                                             \
  enabled, marchSteps, foldIterations, fov, foldOffset, yFold,                 \
      branchThickness, animSpeed, driftSpeed, synapseIntensity,                \
      synapseBounceFreq, synapsePulseFreq, decayHalfLife, trailBlur, baseFreq, \
      maxFreq, gain, curve, baseBright, colorSpeed, colorStretch, gradient,    \
      brightness, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SynapseTreeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float animPhase;  // CPU-accumulated anim time
  float camZ;       // CPU-accumulated camera Z offset
  float colorPhase; // CPU-accumulated color scroll
  // Uniform locations — one int per shader uniform
  int resolutionLoc;
  int animPhaseLoc;
  int marchStepsLoc;
  int foldIterationsLoc;
  int fovLoc;
  int foldOffsetLoc;
  int yFoldLoc;
  int branchThicknessLoc;
  int camZLoc;
  int synapseIntensityLoc;
  int synapseBounceFreqLoc;
  int synapsePulseFreqLoc;
  int colorPhaseLoc;
  int colorStretchLoc;
  int brightnessLoc;
  int gradientLUTLoc;
  int previousFrameLoc;
  int decayFactorLoc;
  int trailBlurLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} SynapseTreeEffect;

// Returns true on success, false if shader fails to load
bool SynapseTreeEffectInit(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                           int width, int height);

// Binds all uniforms, advances time accumulators, updates LUT texture
void SynapseTreeEffectSetup(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                            float deltaTime, Texture2D fftTexture);

// Renders fractal tree into ping-pong trail buffer with decay blending
void SynapseTreeEffectRender(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                             float deltaTime, int screenWidth,
                             int screenHeight);

// Reallocates ping-pong render textures on resolution change
void SynapseTreeEffectResize(SynapseTreeEffect *e, int width, int height);

// Unloads shader and frees LUT
void SynapseTreeEffectUninit(SynapseTreeEffect *e);

// Returns default config
SynapseTreeConfig SynapseTreeConfigDefault(void);

// Registers modulatable params with the modulation engine
void SynapseTreeRegisterParams(SynapseTreeConfig *cfg);

#endif // SYNAPSE_TREE_H
