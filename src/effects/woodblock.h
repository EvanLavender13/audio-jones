// Woodblock effect module
// Simulates relief woodcut printing with posterized tones, carved edges, and
// wood grain texture

#ifndef WOODBLOCK_H
#define WOODBLOCK_H

#include "raylib.h"
#include <stdbool.h>

struct WoodblockConfig {
  bool enabled = false;
  float levels = 5.0f;         // Posterization steps (2-12)
  float edgeThreshold = 0.25f; // Sobel sensitivity (0.0-1.0)
  float edgeSoftness = 0.05f;  // Outline transition width (0.0-0.5)
  float edgeThickness = 1.2f;  // Outline width multiplier (0.5-3.0)
  float grainIntensity =
      0.25f;               // Wood grain visibility in inked areas (0.0-1.0)
  float grainScale = 8.0f; // Grain pattern fineness (1.0-20.0)
  float grainAngle = 0.0f; // Grain direction (radians, -PI..PI)
  float registrationOffset = 0.004f; // Color layer misalignment (0.0-0.02)
  float inkDensity = 1.0f;           // Ink coverage strength (0.3-1.5)
  float paperTone = 0.3f;            // Paper warmth 0=white, 1=cream (0.0-1.0)
};

#define WOODBLOCK_CONFIG_FIELDS                                                \
  enabled, levels, edgeThreshold, edgeSoftness, edgeThickness, grainIntensity, \
      grainScale, grainAngle, registrationOffset, inkDensity, paperTone

typedef struct WoodblockEffect {
  Shader shader;
  int resolutionLoc;
  int levelsLoc;
  int edgeThresholdLoc;
  int edgeSoftnessLoc;
  int edgeThicknessLoc;
  int grainIntensityLoc;
  int grainScaleLoc;
  int grainAngleLoc;
  int registrationOffsetLoc;
  int inkDensityLoc;
  int paperToneLoc;
} WoodblockEffect;

// Returns true on success, false if shader fails to load
bool WoodblockEffectInit(WoodblockEffect *e);

// Sets all uniforms
void WoodblockEffectSetup(WoodblockEffect *e, const WoodblockConfig *cfg);

// Unloads shader
void WoodblockEffectUninit(WoodblockEffect *e);

// Returns default config
WoodblockConfig WoodblockConfigDefault(void);

// Registers modulatable params with the modulation engine
void WoodblockRegisterParams(WoodblockConfig *cfg);

#endif // WOODBLOCK_H
