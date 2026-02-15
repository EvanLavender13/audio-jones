// Disco ball effect module
// Renders a rotating mirrored sphere with projected light spots

#ifndef DISCO_BALL_H
#define DISCO_BALL_H

#include "raylib.h"
#include <stdbool.h>

struct DiscoBallConfig {
  bool enabled = false;
  float sphereRadius =
      0.8f;               // Size of ball (0.2-1.5, fraction of screen height)
  float tileSize = 0.12f; // Facet grid density (0.05-0.3, smaller = more tiles)
  float rotationSpeed = 0.5f;    // Spin rate (radians/sec)
  float bumpHeight = 0.1f;       // Edge bevel depth (0.0-0.2)
  float reflectIntensity = 2.0f; // Brightness of reflected texture (0.5-5.0)

  // Light projection (spots outside sphere)
  float spotIntensity = 1.0f; // Background light spot brightness (0.0-3.0)
  float spotFalloff = 1.0f;   // Spot edge softness, higher = softer (0.5-3.0)
  float brightnessThreshold =
      0.1f; // Minimum input brightness to project (0.0-0.5)
};

#define DISCO_BALL_CONFIG_FIELDS                                               \
  enabled, sphereRadius, tileSize, rotationSpeed, bumpHeight,                  \
      reflectIntensity, spotIntensity, spotFalloff, brightnessThreshold

typedef struct DiscoBallEffect {
  Shader shader;
  int resolutionLoc;
  int sphereRadiusLoc;
  int tileSizeLoc;
  int sphereAngleLoc;
  int bumpHeightLoc;
  int reflectIntensityLoc;
  int spotIntensityLoc;
  int spotFalloffLoc;
  int brightnessThresholdLoc;
  float angle; // Rotation accumulator
} DiscoBallEffect;

// Returns true on success, false if shader fails to load
bool DiscoBallEffectInit(DiscoBallEffect *e);

// Accumulates rotation, sets all uniforms
void DiscoBallEffectSetup(DiscoBallEffect *e, const DiscoBallConfig *cfg,
                          float deltaTime);

// Unloads shader
void DiscoBallEffectUninit(DiscoBallEffect *e);

// Returns default config
DiscoBallConfig DiscoBallConfigDefault(void);

// Registers modulatable params with the modulation engine
void DiscoBallRegisterParams(DiscoBallConfig *cfg);

#endif // DISCO_BALL_H
