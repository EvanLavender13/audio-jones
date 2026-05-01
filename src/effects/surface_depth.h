#ifndef SURFACE_DEPTH_H
#define SURFACE_DEPTH_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

// Surface Depth - parallax occlusion and relief lighting from luminance height
struct SurfaceDepthConfig {
  bool enabled = false;

  // Depth
  int depthMode = 1;         // 0=Flat, 1=Simple, 2=POM
  float heightScale = 0.15f; // UV displacement magnitude (0.0-0.5)
  float heightPower = 1.0f;  // Luminance->height gamma (0.5-3.0)
  int steps = 15;            // Ray-march quality (8-64)

  // View
  float viewAngleX = 0.0f; // Horizontal viewing tilt (-1.0 to 1.0)
  float viewAngleY = 0.0f; // Vertical viewing tilt (-1.0 to 1.0)
  DualLissajousConfig viewLissajous = {.amplitude = 0.0f};

  // Lighting
  bool lighting = true;
  float lightAngle = 0.785f; // Light direction in radians (-pi to pi)
  float lightHeight = 0.5f;  // Light elevation (0.1-2.0)
  float intensity = 0.7f;    // Lighting blend strength (0.0-1.0)
  float reliefScale = 0.2f;  // Normal flatness, higher = subtler (0.02-1.0)
  float shininess = 32.0f;   // Specular exponent (1.0-128.0)

  // Fresnel
  bool fresnel = false;
  float fresnelExponent = 2.0f;  // Rim tightness (1.0-10.0)
  float fresnelIntensity = 2.0f; // Rim glow brightness (0.0-3.0)
};

#define SURFACE_DEPTH_CONFIG_FIELDS                                            \
  enabled, depthMode, heightScale, heightPower, steps, viewAngleX, viewAngleY, \
      viewLissajous, lighting, lightAngle, lightHeight, intensity,             \
      reliefScale, shininess, fresnel, fresnelExponent, fresnelIntensity

typedef struct SurfaceDepthEffect {
  Shader shader;
  int resolutionLoc;
  int depthModeLoc;
  int heightScaleLoc;
  int heightPowerLoc;
  int stepsLoc;
  int viewAngleLoc; // vec2 - computed on CPU from X/Y + lissajous
  int lightingLoc;  // int (bool as 0/1)
  int lightAngleLoc;
  int lightHeightLoc;
  int intensityLoc;
  int reliefScaleLoc;
  int shininessLoc;
  int fresnelEnabledLoc; // int (bool as 0/1)
  int fresnelExponentLoc;
  int fresnelIntensityLoc;
  int timeLoc; // float - for jitter temporal component
  float time;  // Animation accumulator
} SurfaceDepthEffect;

// Returns true on success, false if shader fails to load
bool SurfaceDepthEffectInit(SurfaceDepthEffect *e);

// Accumulates time, sets all uniforms
void SurfaceDepthEffectSetup(SurfaceDepthEffect *e, SurfaceDepthConfig *cfg,
                             float deltaTime);

// Unloads shader
void SurfaceDepthEffectUninit(const SurfaceDepthEffect *e);

// Registers modulatable params with the modulation engine
void SurfaceDepthRegisterParams(SurfaceDepthConfig *cfg);

SurfaceDepthEffect *GetSurfaceDepthEffect(PostEffect *pe);

#endif // SURFACE_DEPTH_H
