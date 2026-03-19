// Polyhedral Mirror effect module
// Raymarched interior reflections inside platonic solid polyhedra

#ifndef POLYHEDRAL_MIRROR_H
#define POLYHEDRAL_MIRROR_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PolyhedralMirrorConfig {
  bool enabled = false;

  // Shape
  int shape = 4; // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa

  // Camera
  float orbitSpeedXZ = 0.5f;   // XZ orbit rate rad/s (-PI..+PI)
  float orbitSpeedYZ = 0.2f;   // YZ orbit rate rad/s (-PI..+PI)
  float cameraDistance = 0.4f; // Origin to camera (0.1-0.8)

  // Reflection
  float maxBounces = 5.0f;   // Reflection bounces (1-8), float for modulation
  float reflectivity = 0.6f; // Per-bounce color mix factor (0.1-1.0)

  // Geometry
  float edgeRadius = 0.05f; // Wireframe capsule radius (0.01-0.15)
  float edgeGlow = 1.0f;    // Edge brightness multiplier (0.0-2.0)
  int maxIterations = 80;   // Raymarch step limit (32-128)

  // Audio
  float baseFreq = 55.0f;   // Low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f; // High frequency bound Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define POLYHEDRAL_MIRROR_CONFIG_FIELDS                                        \
  enabled, shape, orbitSpeedXZ, orbitSpeedYZ, cameraDistance, maxBounces,      \
      reflectivity, edgeRadius, edgeGlow, maxIterations, baseFreq, maxFreq,    \
      gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct PolyhedralMirrorEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated rotation angles
  float angleXZAccum;
  float angleYZAccum;

  // Uniform locations
  int resolutionLoc;
  int faceNormalsLoc;
  int faceCountLoc;
  int planeOffsetLoc;
  int edgeALoc;
  int edgeBLoc;
  int edgeCountLoc;
  int angleXZLoc;
  int angleYZLoc;
  int cameraDistanceLoc;
  int maxBouncesLoc;
  int reflectivityLoc;
  int edgeRadiusLoc;
  int edgeGlowLoc;
  int maxIterationsLoc;
  int gradientLUTLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int fftTextureLoc;
} PolyhedralMirrorEffect;

// Returns true on success, false if shader fails to load
bool PolyhedralMirrorEffectInit(PolyhedralMirrorEffect *e,
                                const PolyhedralMirrorConfig *cfg);

// Binds all uniforms, updates LUT texture
void PolyhedralMirrorEffectSetup(PolyhedralMirrorEffect *e,
                                 const PolyhedralMirrorConfig *cfg,
                                 float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void PolyhedralMirrorEffectUninit(PolyhedralMirrorEffect *e);

// Registers modulatable params with the modulation engine
void PolyhedralMirrorRegisterParams(PolyhedralMirrorConfig *cfg);

#endif // POLYHEDRAL_MIRROR_H
