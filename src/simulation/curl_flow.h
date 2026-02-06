#ifndef CURL_FLOW_H
#define CURL_FLOW_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

typedef struct TrailMap TrailMap;
typedef struct ColorLUT ColorLUT;

typedef struct CurlFlowAgent {
  float x;
  float y;
  float velocityAngle;
  float _pad[5]; // Pad to 32 bytes for GPU alignment
} CurlFlowAgent;

typedef struct CurlFlowConfig {
  bool enabled = false;
  int agentCount = 100000;
  float noiseFrequency = 0.005f; // Spatial frequency (0.001-0.1)
  float noiseEvolution = 0.5f;   // Temporal evolution speed (0.0-2.0)
  float momentum = 0.0f; // Agent inertia (0.0=instant turn, 1.0=never turn)
  float trailInfluence =
      0.3f; // Density bends flow field (0.0-1.0, Bridson 2007)
  float accumSenseBlend =
      0.0f; // Blend trail (0) vs feedback (1) for density sensing
  float gradientRadius =
      4.0f;              // Density gradient sample distance in pixels (1-32)
  float stepSize = 2.0f; // Movement speed (0.5-5.0)
  float respawnProbability = 0.0f; // Per-frame teleport chance (0.0-0.1)
  float depositAmount = 0.1f;      // Trail deposit strength (0.01-0.2)
  float decayHalfLife = 1.0f;      // Seconds for 50% decay (0.1-5.0)
  int diffusionScale = 1;          // Diffusion kernel scale in pixels (0-4)
  float boostIntensity = 1.0f;     // Trail boost strength (0.0-5.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  ColorConfig color;
  bool debugOverlay = false;
} CurlFlowConfig;

typedef struct CurlFlow {
  unsigned int agentBuffer;
  unsigned int computeProgram;
  TrailMap *trailMap;
  ColorLUT *colorLUT;
  Shader debugShader;
  int agentCount;
  int width;
  int height;
  // Agent shader uniforms
  int resolutionLoc;
  int timeLoc;
  int noiseFrequencyLoc;
  int noiseEvolutionLoc;
  int trailInfluenceLoc;
  int stepSizeLoc;
  int depositAmountLoc;
  int valueLoc;
  int accumSenseBlendLoc;
  int gradientRadiusLoc;
  int momentumLoc;
  int respawnProbabilityLoc;
  // Gradient pass resources
  unsigned int gradientTexture;
  unsigned int gradientProgram;
  int gradResolutionLoc;
  int gradRadiusLoc;
  int gradAccumBlendLoc;
  float time;
  CurlFlowConfig config;
  bool supported;
} CurlFlow;

// Check if compute shaders are supported (OpenGL 4.3+)
bool CurlFlowSupported(void);

// Initialize curl flow simulation
// Returns NULL if compute shaders not supported or allocation fails
CurlFlow *CurlFlowInit(int width, int height, const CurlFlowConfig *config);

// Clean up curl flow resources
void CurlFlowUninit(CurlFlow *cf);

// Dispatch compute shader to update agents
void CurlFlowUpdate(CurlFlow *cf, float deltaTime, Texture2D accumTexture);

// Process trails with diffusion and decay (call after CurlFlowUpdate)
void CurlFlowProcessTrails(CurlFlow *cf, float deltaTime);

// Draw debug overlay (trail map visualization)
void CurlFlowDrawDebug(CurlFlow *cf);

// Update dimensions (call when window resizes)
void CurlFlowResize(CurlFlow *cf, int width, int height);

// Reinitialize agents to random positions, clear trails
void CurlFlowReset(CurlFlow *cf);

// Register modulatable params with the modulation engine
void CurlFlowRegisterParams(CurlFlowConfig *cfg);

// Apply config changes (call before update if config may have changed)
// Handles agent count changes (buffer reallocation)
void CurlFlowApplyConfig(CurlFlow *cf, const CurlFlowConfig *newConfig);

// Begin/end drawing to trail map (for feedback injection)
bool CurlFlowBeginTrailMapDraw(CurlFlow *cf);
void CurlFlowEndTrailMapDraw(CurlFlow *cf);

#endif // CURL_FLOW_H
