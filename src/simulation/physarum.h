#ifndef PHYSARUM_H
#define PHYSARUM_H

#include <stdbool.h>
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"

typedef struct TrailMap TrailMap;

typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float _pad[4];     // Pad to 32 bytes for GPU alignment
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
    EffectBlendMode blendMode = EFFECT_BLEND_BOOST; // Blend mode for trail compositing
    float accumSenseBlend = 0.0f; // Blend between trail (0) and accum (1) sensing
    float repulsionStrength = 0.0f; // Opposite-hue repulsion: 0 = soft clustering, 1 = hard territories
    bool debugOverlay = false;   // Show color debug visualization
    ColorConfig color;           // Hue distribution for species
} PhysarumConfig;

typedef struct Physarum {
    unsigned int agentBuffer;
    unsigned int computeProgram;
    TrailMap* trailMap;
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
    int accumSenseBlendLoc;
    int repulsionStrengthLoc;
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
void PhysarumUpdate(Physarum* p, float deltaTime, Texture2D accumTexture, Texture2D fftTexture);

// Process trails with diffusion and decay (call after PhysarumUpdate)
void PhysarumProcessTrails(Physarum* p, float deltaTime);

// Draw trail map as full-screen color overlay (debug visualization)
void PhysarumDrawDebug(Physarum* p);

// Update dimensions (call when window resizes)
void PhysarumResize(Physarum* p, int width, int height);

// Reinitialize agents to random positions
void PhysarumReset(Physarum* p);

// Apply config changes (call before update if config may have changed)
// Handles agent count changes (buffer reallocation)
void PhysarumApplyConfig(Physarum* p, const PhysarumConfig* newConfig);

// Begin drawing to trailMap (for waveform injection)
// Returns true if trailMap is now active for drawing
bool PhysarumBeginTrailMapDraw(Physarum* p);

// End drawing to trailMap
void PhysarumEndTrailMapDraw(Physarum* p);

#endif // PHYSARUM_H
