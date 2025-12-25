#ifndef PHYSARUM_H
#define PHYSARUM_H

#include <stdbool.h>
#include "raylib.h"
#include "color_config.h"

typedef enum {
    TRAIL_BLEND_BOOST = 0,
    TRAIL_BLEND_TINTED_BOOST,
    TRAIL_BLEND_SCREEN,
    TRAIL_BLEND_MIX,
    TRAIL_BLEND_SOFT_LIGHT,
} TrailBlendMode;

typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float spectrumPos; // Position in color distribution (0-1) for FFT lookup
    float _pad[3];     // Pad to 32 bytes for GPU alignment
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
    TrailBlendMode trailBlendMode = TRAIL_BLEND_BOOST; // Blend mode for trail compositing
    float accumSenseBlend = 0.0f; // Blend between trail (0) and accum (1) sensing
    float frequencyModulation = 0.0f; // FFT repulsion strength (0-1)
    float stepBeatModulation = 0.0f;    // Beat intensity step size boost (0-3)
    float sensorBeatModulation = 0.0f;  // Beat intensity sensor range boost (0-3)
    bool debugOverlay = false;   // Show color debug visualization
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
    int accumSenseBlendLoc;
    int frequencyModulationLoc;
    int beatIntensityLoc;
    int stepBeatModulationLoc;
    int sensorBeatModulationLoc;
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
void PhysarumUpdate(Physarum* p, float deltaTime, Texture2D accumTexture, Texture2D fftTexture,
                    float beatIntensity);

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
