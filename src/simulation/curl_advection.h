#ifndef CURL_ADVECTION_H
#define CURL_ADVECTION_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

typedef struct TrailMap TrailMap;
typedef struct ColorLUT ColorLUT;

typedef struct CurlAdvectionConfig {
  bool enabled = false;
  int steps = 40;                   // Advection iterations (10-80)
  float advectionCurl = 0.2f;       // How much paths spiral (0.0-1.0)
  float curlScale = -2.0f;          // Vortex rotation strength (-4.0-4.0)
  float laplacianScale = 0.05f;     // Diffusion/smoothing (0.0-0.2)
  float pressureScale = -2.0f;      // Compression waves (-4.0-4.0)
  float divergenceScale = -0.4f;    // Source/sink strength (-1.0-1.0)
  float divergenceUpdate = -0.03f;  // Divergence feedback rate (-0.1-0.1)
  float divergenceSmoothing = 0.3f; // Divergence smoothing (0.0-0.5)
  float selfAmp = 1.0f;             // Self-amplification (0.5-2.0)
  float updateSmoothing = 0.4f;     // Temporal stability (0.25-0.9)
  float injectionIntensity = 0.0f;  // Energy injection (0.0-1.0, modulatable)
  float injectionThreshold = 0.1f;  // Accum brightness cutoff (0.0-1.0)
  float decayHalfLife = 0.5f;       // Trail decay half-life (0.1-5.0 s)
  int diffusionScale = 0;           // Trail diffusion passes (0-4)
  float boostIntensity = 1.0f;      // Trail boost strength (0.0-5.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  ColorConfig color;
  bool debugOverlay = false;
} CurlAdvectionConfig;

typedef struct CurlAdvection {
  unsigned int
      stateTextures[2]; // Ping-pong state (RGBA16F: xy=velocity, z=divergence)
  int currentBuffer;    // Which state texture to read from (0 or 1)
  unsigned int computeProgram;
  TrailMap *trailMap;
  ColorLUT *colorLUT;
  Shader debugShader;
  int width;
  int height;

  // Uniform locations
  int resolutionLoc;
  int stepsLoc;
  int advectionCurlLoc;
  int curlScaleLoc;
  int laplacianScaleLoc;
  int pressureScaleLoc;
  int divergenceScaleLoc;
  int divergenceUpdateLoc;
  int divergenceSmoothingLoc;
  int selfAmpLoc;
  int updateSmoothingLoc;
  int injectionIntensityLoc;
  int injectionThresholdLoc;
  int valueLoc;
  CurlAdvectionConfig config;
  bool supported;
} CurlAdvection;

// Check if compute shaders are supported (OpenGL 4.3+)
bool CurlAdvectionSupported(void);

// Initialize curl advection simulation
// Returns NULL if compute shaders not supported or allocation fails
CurlAdvection *CurlAdvectionInit(int width, int height,
                                 const CurlAdvectionConfig *config);

// Clean up curl advection resources
void CurlAdvectionUninit(CurlAdvection *ca);

// Dispatch compute shader to update simulation
void CurlAdvectionUpdate(CurlAdvection *ca, float deltaTime,
                         Texture2D accumTexture);

// Process trails with diffusion and decay
void CurlAdvectionProcessTrails(CurlAdvection *ca, float deltaTime);

// Draw debug overlay (state visualization)
void CurlAdvectionDrawDebug(CurlAdvection *ca);

// Update dimensions
void CurlAdvectionResize(CurlAdvection *ca, int width, int height);

// Reinitialize state to noise, clear trails
void CurlAdvectionReset(CurlAdvection *ca);

// Apply config changes
void CurlAdvectionApplyConfig(CurlAdvection *ca,
                              const CurlAdvectionConfig *newConfig);

// Register modulatable parameters with the modulation engine
void CurlAdvectionRegisterParams(CurlAdvectionConfig *cfg);

#endif // CURL_ADVECTION_H
