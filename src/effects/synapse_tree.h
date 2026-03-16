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
  float foldOffset = 0.5f;       // Abs-fold translation (0.2-0.6)
  float branchThickness = 0.06f; // Cylinder radius (0.01-0.2)

  // Animation
  float animSpeed = 0.4f;  // Fractal morph rate (0.0-2.0)
  float orbitSpeed = 0.3f; // Camera Y-orbit rate radians/sec (-PI..PI)

  // Synapse
  float synapseIntensity = 0.5f;   // Pulse brightness (0.0-2.0)
  float synapseBounceFreq = 11.0f; // Vertical bounce speed (1.0-20.0)
  float synapsePulseFreq = 7.0f;   // Size oscillation speed (1.0-15.0)

  // Color
  float colorSpeed = 0.3f;   // LUT scroll rate (0.0-2.0)
  float colorStretch = 1.0f; // Spatial color frequency (0.1-5.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define SYNAPSE_TREE_CONFIG_FIELDS                                             \
  enabled, marchSteps, foldIterations, fov, foldOffset, branchThickness,       \
      animSpeed, orbitSpeed, synapseIntensity, synapseBounceFreq,              \
      synapsePulseFreq, colorSpeed, colorStretch, gradient, blendMode,         \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SynapseTreeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float animPhase;  // CPU-accumulated anim time
  float orbitAngle; // CPU-accumulated orbit angle
  float colorPhase; // CPU-accumulated color scroll
  // Uniform locations — one int per shader uniform
  int resolutionLoc;
  int animPhaseLoc;
  int marchStepsLoc;
  int foldIterationsLoc;
  int fovLoc;
  int foldOffsetLoc;
  int branchThicknessLoc;
  int orbitAngleLoc;
  int synapseIntensityLoc;
  int synapseBounceFreqLoc;
  int synapsePulseFreqLoc;
  int colorPhaseLoc;
  int colorStretchLoc;
  int gradientLUTLoc;
} SynapseTreeEffect;

// Returns true on success, false if shader fails to load
bool SynapseTreeEffectInit(SynapseTreeEffect *e, const SynapseTreeConfig *cfg);

// Binds all uniforms, advances time accumulators, updates LUT texture
void SynapseTreeEffectSetup(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                            float deltaTime);

// Unloads shader and frees LUT
void SynapseTreeEffectUninit(SynapseTreeEffect *e);

// Returns default config
SynapseTreeConfig SynapseTreeConfigDefault(void);

// Registers modulatable params with the modulation engine
void SynapseTreeRegisterParams(SynapseTreeConfig *cfg);

#endif // SYNAPSE_TREE_H
