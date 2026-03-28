// MazeWorms - maze-carving agents that leave glowing trails via collision
// steering
#ifndef MAZE_WORMS_H
#define MAZE_WORMS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

typedef struct TrailMap TrailMap;
typedef struct ColorLUT ColorLUT;

typedef enum {
  MAZE_WORM_TURN_SPIRAL = 0, // angle += curvature / age
  MAZE_WORM_TURN_WALL_FOLLOW =
      1, // angle -= curvature / sqrt(age); while(collision) angle += turnAngle
  MAZE_WORM_TURN_WALL_HUG =
      2, // scan from angle-1.6 for nearest wall, back off by gap
  MAZE_WORM_TURN_MIXED =
      3, // angle += curvature * age * chirality (+/-1 per agent)
} MazeWormTurningMode;

typedef struct MazeWormAgent {
  float x;            // position in pixels
  float y;            // position in pixels
  float angle;        // heading in radians
  float age;          // step count (increments each step, starts at 1)
  float alive;        // 1.0 = alive, 0.0 = dead (awaiting respawn)
  float hue;          // gradient LUT coordinate [0,1]
  float respawnTimer; // seconds remaining until next respawn attempt
  float _pad;         // padding to 32 bytes
} MazeWormAgent;

typedef struct MazeWormsConfig {
  bool enabled = false;
  int wormCount = 50; // Number of simultaneous agents (4-200)
  MazeWormTurningMode turningMode =
      MAZE_WORM_TURN_SPIRAL; // Agent steering strategy
  float curvature = 1.0f;    // Turning strength (0.01-10.0)
  float turnAngle =
      0.5f; // Collision avoidance step (+/- ROTATION_OFFSET_MAX rad)
  float trailWidth = 1.5f;      // Smoothstep circle radius in pixels (0.5-5.0)
  float decayHalfLife = 8.0f;   // Trail persistence in seconds (0.5-30.0)
  int diffusionScale = 0;       // Trail blur radius in pixels (0-5)
  float respawnCooldown = 0.5f; // Seconds before dead worm respawns (0.0-5.0)
  int stepsPerFrame = 2;        // Agent steps per frame (1-8)
  float collisionGap = 2.0f;    // Lookahead beyond trail width (0.0-5.0 pixels)
  float boostIntensity = 1.0f;  // Blend intensity (0.0-2.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN; // Compositing mode
  ColorConfig color; // Hue distribution for agent coloring
} MazeWormsConfig;

#define MAZE_WORMS_CONFIG_FIELDS                                               \
  enabled, wormCount, turningMode, curvature, turnAngle, trailWidth,           \
      decayHalfLife, diffusionScale, respawnCooldown, stepsPerFrame,           \
      collisionGap, boostIntensity, blendMode, color

typedef struct MazeWorms {
  unsigned int agentBuffer;    // SSBO for agent data
  unsigned int computeProgram; // Agent update compute shader
  TrailMap *trailMap; // Shared trail infrastructure (diffusion + decay)
  ColorLUT *colorLUT; // Gradient texture for agent coloring
  int agentCount;     // Current agent count (tracks config changes)
  int width;
  int height;

  // Compute shader uniform locations
  int resolutionLoc;
  int turningModeLoc;
  int curvatureLoc;
  int turnAngleLoc;
  int trailWidthLoc;
  int collisionGapLoc;
  int timeLoc;
  int gradientLUTLoc;
  int respawnCooldownLoc;
  int stepDeltaTimeLoc;

  float time;             // Animation time accumulator
  MazeWormsConfig config; // Cached config for change detection
  bool supported;
} MazeWorms;

// Check if compute shaders are supported (OpenGL 4.3+)
bool MazeWormsSupported(void);

// Initialize maze worms simulation
// Returns NULL if compute shaders not supported or allocation fails
MazeWorms *MazeWormsInit(int width, int height, const MazeWormsConfig *config);

// Clean up maze worms resources
void MazeWormsUninit(MazeWorms *mw);

// Dispatch compute shader to update agents
void MazeWormsUpdate(MazeWorms *mw, float deltaTime);

// Process trails with diffusion and decay (call after MazeWormsUpdate)
void MazeWormsProcessTrails(MazeWorms *mw, float deltaTime);

// Update dimensions (call when window resizes)
void MazeWormsResize(MazeWorms *mw, int width, int height);

// Reinitialize agents to random positions
void MazeWormsReset(MazeWorms *mw);

// Register modulatable params with the modulation engine
void MazeWormsRegisterParams(MazeWormsConfig *cfg);

// Apply config changes (call before update if config may have changed)
// Handles worm count changes (buffer reallocation)
void MazeWormsApplyConfig(MazeWorms *mw, const MazeWormsConfig *newConfig);

#endif // MAZE_WORMS_H
