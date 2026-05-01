// Polymorph effect module
// SDF ray-marched glowing wireframe polyhedron that morphs between platonic
// solids

#ifndef POLYMORPH_H
#define POLYMORPH_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>
#include <stdint.h>

struct PostEffect;

struct PolymorphConfig {
  bool enabled = false;

  // Shape
  int baseShape =
      4; // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa
  float randomness = 0.0f; // Shape selection chaos (0.0-1.0)
  float freeform = 0.0f;   // Vertex perturbation (0.0-1.0)

  // Morph
  float morphSpeed = 0.5f; // Morph cycles per second (0.1-5.0)
  float holdRatio = 0.4f;  // Fraction of cycle showing full wireframe (0.0-1.0)

  // Camera
  float orbitSpeed = 0.5f;      // Camera orbit rate rad/s (-PI..+PI)
  float cameraHeight = 5.0f;    // Camera Y offset (0.0-15.0)
  float cameraDistance = 25.0f; // Camera orbit radius (5.0-50.0)

  // Geometry
  float scale = 15.0f;         // Shape size (1.0-30.0)
  float edgeThickness = 0.02f; // Capsule radius (0.005-0.1)
  float glowIntensity = 0.2f;  // Glow brightness multiplier (0.05-1.0)
  float fov = 1.8f;            // Field of view factor (1.0-4.0)

  // Audio
  float baseFreq = 55.0f;   // (27.5-440.0)
  float maxFreq = 14000.0f; // (1000-16000)
  float gain = 2.0f;        // (0.1-10.0)
  float curve = 1.5f;       // (0.1-3.0)
  float baseBright = 0.15f; // (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define POLYMORPH_CONFIG_FIELDS                                                \
  enabled, baseShape, randomness, freeform, morphSpeed, holdRatio, orbitSpeed, \
      cameraHeight, cameraDistance, scale, edgeThickness, glowIntensity, fov,  \
      baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode,         \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct PolymorphEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU morph state
  float morphPhase;  // 0..1 within current cycle
  int currentShape;  // Index into SHAPES[] (0-4)
  uint32_t rngState; // PRNG state for shape selection and freeform

  // CPU-accumulated accumulators
  float orbitAccum;

  // Per-edge data computed each frame (max 30 edges)
  float edgeAx[30], edgeAy[30], edgeAz[30];
  float edgeBx[30], edgeBy[30], edgeBz[30];
  float edgeT[30];
  int edgeCount;

  // Freeform vertex offsets (regenerated each cycle)
  float vertexOffsetX[20], vertexOffsetY[20], vertexOffsetZ[20];

  // Uniform locations
  int resolutionLoc;
  int edgeALoc, edgeBLoc;
  int edgeTLoc;
  int edgeCountLoc;
  int edgeThicknessLoc;
  int glowIntensityLoc;
  int cameraOriginLoc;
  int cameraFovLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} PolymorphEffect;

// Returns true on success, false if shader fails to load
bool PolymorphEffectInit(PolymorphEffect *e, const PolymorphConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void PolymorphEffectSetup(PolymorphEffect *e, const PolymorphConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void PolymorphEffectUninit(PolymorphEffect *e);

// Registers modulatable params with the modulation engine
void PolymorphRegisterParams(PolymorphConfig *cfg);

PolymorphEffect *GetPolymorphEffect(PostEffect *pe);

#endif // POLYMORPH_H
