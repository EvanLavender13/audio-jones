// Attractor lines effect module
// Traces 3D strange attractor trajectories as glowing lines with trail
// persistence

#ifndef ATTRACTOR_LINES_H
#define ATTRACTOR_LINES_H

#include "config/attractor_types.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct AttractorLinesConfig {
  bool enabled = false;

  // Attractor system
  AttractorType attractorType = ATTRACTOR_LORENZ;
  float sigma = 10.0f;    // Lorenz coupling (1-30)
  float rho = 28.0f;      // Lorenz z-folding (10-50)
  float beta = 2.667f;    // Lorenz z-damping (0.5-5)
  float rosslerC = 5.7f;  // Rossler chaos transition (2-12)
  float thomasB = 0.208f; // Thomas damping (0.1-0.3)
  float dadrasA = 3.0f;   // Dadras a (1-5)
  float dadrasB = 2.7f;   // Dadras b (1-5)
  float dadrasC = 1.7f;   // Dadras c (0.5-3)
  float dadrasD = 2.0f;   // Dadras d (0.5-4)
  float dadrasE = 9.0f;   // Dadras e (4-15)

  // Line tracing
  float steps = 96.0f; // Integration steps/frame (32-256), float for modulation
  float speed = 1.0f;  // Trajectory advance rate multiplier (0.05-1.0)
  float viewScale = 0.025f; // Attractor-to-screen scale (0.005-0.1)

  // Appearance
  float intensity = 0.18f;    // Line brightness (0.01-1.0)
  float decayHalfLife = 2.0f; // Trail decay half-life in seconds (0.1-10.0)
  float focus = 2.0f;         // Line sharpness (0.5-5.0)
  float maxSpeed = 50.0f;     // Velocity normalization ceiling (5-200)

  // Transform
  float x = 0.5f;              // Screen X position (0.0-1.0)
  float y = 0.5f;              // Screen Y position (0.0-1.0)
  float rotationAngleX = 0.0f; // Static X rotation (radians, -PI to PI)
  float rotationAngleY = 0.0f; // Static Y rotation
  float rotationAngleZ = 0.0f; // Static Z rotation
  float rotationSpeedX = 0.0f; // X rotation rate (rad/s, -2 to 2)
  float rotationSpeedY = 0.0f; // Y rotation rate
  float rotationSpeedZ = 0.0f; // Z rotation rate

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct AttractorLinesEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2]; // Trail persistence pair
  int readIdx;                 // Which pingPong to read from (0 or 1)
  AttractorType lastType;      // Detect type changes to reset state
  float rotationAccumX;        // Accumulated X rotation angle
  float rotationAccumY;        // Accumulated Y rotation angle
  float rotationAccumZ;        // Accumulated Z rotation angle

  // Shader uniform locations
  int resolutionLoc;
  int previousFrameLoc;
  int attractorTypeLoc;
  int sigmaLoc;
  int rhoLoc;
  int betaLoc;
  int rosslerCLoc;
  int thomasBLoc;
  int dadrasALoc;
  int dadrasBLoc;
  int dadrasCLoc;
  int dadrasDLoc;
  int dadrasELoc;
  int stepsLoc;
  int speedLoc;

  int viewScaleLoc;
  int intensityLoc;
  int decayFactorLoc;
  int focusLoc;
  int maxSpeedLoc;
  int xLoc;
  int yLoc;
  int rotationMatrixLoc;
  int gradientLUTLoc;
} AttractorLinesEffect;

// Loads shader, caches uniform locations, allocates ping-pong textures
bool AttractorLinesEffectInit(AttractorLinesEffect *e,
                              const AttractorLinesConfig *cfg, int width,
                              int height);

// Binds scalar uniforms and accumulates rotation state
void AttractorLinesEffectSetup(AttractorLinesEffect *e,
                               const AttractorLinesConfig *cfg, float deltaTime,
                               int screenWidth, int screenHeight);

// Executes ping-pong render pass: traces lines + fades previous trails
void AttractorLinesEffectRender(AttractorLinesEffect *e,
                                const AttractorLinesConfig *cfg,
                                float deltaTime, int screenWidth,
                                int screenHeight);

// Unloads ping-pong textures, reallocates at new dimensions
void AttractorLinesEffectResize(AttractorLinesEffect *e, int width, int height);

// Unloads shader, frees LUT and ping-pong textures
void AttractorLinesEffectUninit(AttractorLinesEffect *e);

// Returns default config
AttractorLinesConfig AttractorLinesConfigDefault(void);

// Registers modulatable params with the modulation engine
void AttractorLinesRegisterParams(AttractorLinesConfig *cfg);

#endif // ATTRACTOR_LINES_H
