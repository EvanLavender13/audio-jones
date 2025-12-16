#ifndef PHYSARUM_H
#define PHYSARUM_H

#include <stdbool.h>
#include "raylib.h"
#include "color_config.h"

typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    float hue;  // Agent's hue identity (0-1), maintains 16-byte GPU alignment
} PhysarumAgent;

typedef struct PhysarumConfig {
    bool enabled = false;
    int agentCount = 100000;
    float sensorDistance = 20.0f;
    float sensorAngle = 0.5f;
    float turningAngle = 0.3f;
    float stepSize = 1.5f;
    float depositAmount = 0.05f;
    float decayHalfLife = 0.5f;  // Seconds for 50% decay (0.1-5.0 range)
    int diffusionScale = 1;      // Diffusion kernel scale in pixels (0-4 range)
    float boostIntensity = 0.0f; // Trail boost strength (0.0-2.0)
    bool debugOverlay = false;   // Show grayscale debug visualization
    ColorConfig color;           // Hue distribution for species
} PhysarumConfig;

typedef struct Physarum {
    unsigned int agentBuffer;
    unsigned int computeProgram;
    RenderTexture2D trailMap;
    RenderTexture2D trailMapTemp;  // Ping-pong texture for trail processing
    Shader debugShader;
    int agentCount;
    int width;
    int height;
    int resolutionLoc;
    int sensorDistanceLoc;
    int sensorAngleLoc;
    int turningAngleLoc;
    int stepSizeLoc;
    int depositAmountLoc;
    int timeLoc;
    int saturationLoc;
    int valueLoc;
    unsigned int trailProgram;  // Trail processing compute shader
    int trailResolutionLoc;
    int trailDiffusionScaleLoc;
    int trailDecayFactorLoc;
    int trailApplyDecayLoc;
    int trailDirectionLoc;
    float time;
    PhysarumConfig config;
    bool supported;
} Physarum;

// Check if compute shaders are supported (OpenGL 4.3+)
bool PhysarumSupported(void);

// Initialize physarum simulation
// Returns NULL if compute shaders not supported or allocation fails
Physarum* PhysarumInit(int width, int height, const PhysarumConfig* config);

// Clean up physarum resources
void PhysarumUninit(Physarum* p);

// Dispatch compute shader to update agents
void PhysarumUpdate(Physarum* p, float deltaTime);

// Process trails with diffusion and decay (call after PhysarumUpdate)
void PhysarumProcessTrails(Physarum* p, float deltaTime);

// Draw trail map as full-screen grayscale overlay (debug visualization)
void PhysarumDrawDebug(Physarum* p);

// Update dimensions (call when window resizes)
void PhysarumResize(Physarum* p, int width, int height);

// Reinitialize agents to random positions
void PhysarumReset(Physarum* p);

// Apply config changes (call before update if config may have changed)
// Handles agent count changes (buffer reallocation)
void PhysarumApplyConfig(Physarum* p, const PhysarumConfig* newConfig);

#endif // PHYSARUM_H
