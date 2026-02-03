#ifndef CYMATICS_H
#define CYMATICS_H

#include "raylib.h"
#include <stdbool.h>

#include "config/dual_lissajous_config.h"
#include "render/blend_mode.h"
#include "render/color_config.h"

typedef struct TrailMap TrailMap;
typedef struct ColorLUT ColorLUT;

typedef struct CymaticsConfig {
  bool enabled = false;
  float waveScale = 10.0f;       // Pattern scale - higher = larger (1-50)
  float falloff = 1.0f;          // Distance attenuation (0-5)
  float visualGain = 2.0f;       // Output intensity (0.5-5)
  int contourCount = 0;          // Banding (0=smooth, 1-10)
  float decayHalfLife = 0.5f;    // Trail persistence (0.1-5)
  int diffusionScale = 1;        // Blur kernel size (0-4)
  float boostIntensity = 1.0f;   // Trail boost strength (0.0-5.0)
  int sourceCount = 5;           // Number of sources (1-8)
  float baseRadius = 0.4f;       // Base position distance from center (0.0-0.5)
  DualLissajousConfig lissajous; // Source motion pattern
  float patternAngle = 0.0f;     // Pattern rotation offset (radians)
  bool boundaries = false;       // Enable edge reflections
  float reflectionGain = 1.0f;   // Mirror source attenuation (0.0-1.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  bool debugOverlay = false;
  ColorConfig color;
} CymaticsConfig;

typedef struct Cymatics {
  unsigned int computeProgram;
  TrailMap *trailMap;
  ColorLUT *colorLUT;
  Shader debugShader;
  int width;
  int height;

  // Uniform locations
  int resolutionLoc;
  int waveScaleLoc;
  int falloffLoc;
  int visualGainLoc;
  int contourCountLoc;
  int bufferSizeLoc;
  int writeIndexLoc;
  int valueLoc;
  int sourcesLoc;
  int sourceCountLoc;
  int boundariesLoc;
  int reflectionGainLoc;

  CymaticsConfig config;
  bool supported;
} Cymatics;

// Check if compute shaders are supported (OpenGL 4.3+)
bool CymaticsSupported(void);

// Initialize cymatics simulation
// Returns NULL if compute shaders not supported or allocation fails
Cymatics *CymaticsInit(int width, int height, const CymaticsConfig *config);

// Clean up cymatics resources
void CymaticsUninit(Cymatics *cym);

// Dispatch compute shader to update simulation
void CymaticsUpdate(Cymatics *cym, Texture2D waveformTexture, int writeIndex,
                    float deltaTime);

// Process trails with diffusion and decay
void CymaticsProcessTrails(Cymatics *cym, float deltaTime);

// Draw debug overlay (trail visualization)
void CymaticsDrawDebug(Cymatics *cym);

// Update dimensions
void CymaticsResize(Cymatics *cym, int width, int height);

// Clear trails
void CymaticsReset(Cymatics *cym);

// Apply config changes
void CymaticsApplyConfig(Cymatics *cym, const CymaticsConfig *newConfig);

#endif // CYMATICS_H
