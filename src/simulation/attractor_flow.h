#ifndef ATTRACTOR_FLOW_H
#define ATTRACTOR_FLOW_H

#include <stdbool.h>
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"

typedef struct TrailMap TrailMap;

typedef enum AttractorType {
    ATTRACTOR_LORENZ = 0,
    ATTRACTOR_ROSSLER,
    ATTRACTOR_AIZAWA,
    ATTRACTOR_THOMAS,
    ATTRACTOR_COUNT
} AttractorType;

typedef struct AttractorAgent {
    float x;
    float y;
    float z;
    float hue;
    float age;
    float _pad[3];  // Pad to 32 bytes for GPU alignment
} AttractorAgent;

typedef struct AttractorFlowConfig {
    bool enabled = false;
    AttractorType attractorType = ATTRACTOR_LORENZ;
    int agentCount = 100000;
    float timeScale = 0.01f;         // Integration timestep (0.001-0.1)
    float attractorScale = 0.02f;    // World-to-screen scale (0.005-0.1)
    // Lorenz parameters (classic: sigma=10, rho=28, beta=8/3)
    float sigma = 10.0f;
    float rho = 28.0f;
    float beta = 2.666667f;
    // Rössler parameter (classic: c=5.7, chaotic range ~4.0-7.0)
    float rosslerC = 5.7f;
    // Thomas parameter (classic: b=0.208186, chaotic range ~0.17-0.22)
    float thomasB = 0.208186f;
    // Transform: screen position (0-1 normalized, 0.5=center) and 3D rotation
    float x = 0.5f;                  // Screen X position (0.0-1.0)
    float y = 0.5f;                  // Screen Y position (0.0-1.0)
    float rotationX = 0.0f;          // Rotation around X axis (radians)
    float rotationY = 0.0f;          // Rotation around Y axis (radians)
    float rotationZ = 0.0f;          // Rotation around Z axis (radians)
    float depositAmount = 0.1f;      // Trail deposit strength (0.01-0.2)
    float decayHalfLife = 1.0f;      // Seconds for 50% decay (0.1-5.0)
    int diffusionScale = 1;          // Diffusion kernel scale in pixels (0-4)
    float boostIntensity = 1.0f;     // Trail boost strength (0.0-2.0)
    EffectBlendMode blendMode = EFFECT_BLEND_BOOST;
    ColorConfig color;
    bool debugOverlay = false;
} AttractorFlowConfig;

typedef struct AttractorFlow {
    unsigned int agentBuffer;
    unsigned int computeProgram;
    TrailMap* trailMap;
    Shader debugShader;
    int agentCount;
    int width;
    int height;
    // Agent shader uniforms
    int resolutionLoc;
    int timeLoc;
    int attractorTypeLoc;
    int timeScaleLoc;
    int attractorScaleLoc;
    int sigmaLoc;
    int rhoLoc;
    int betaLoc;
    int rosslerCLoc;
    int thomasBLoc;
    int centerLoc;
    int rotationMatrixLoc;
    int depositAmountLoc;
    int saturationLoc;
    int valueLoc;
    float time;
    AttractorFlowConfig config;
    bool supported;
} AttractorFlow;

// Check if compute shaders are supported (OpenGL 4.3+)
bool AttractorFlowSupported(void);

// Initialize attractor flow simulation
// Returns NULL if compute shaders not supported or allocation fails
AttractorFlow* AttractorFlowInit(int width, int height, const AttractorFlowConfig* config);

// Clean up attractor flow resources
void AttractorFlowUninit(AttractorFlow* af);

// Dispatch compute shader to update agents
void AttractorFlowUpdate(AttractorFlow* af, float deltaTime);

// Process trails with diffusion and decay (call after AttractorFlowUpdate)
void AttractorFlowProcessTrails(AttractorFlow* af, float deltaTime);

// Draw debug overlay (trail map visualization)
void AttractorFlowDrawDebug(AttractorFlow* af);

// Update dimensions (call when window resizes)
void AttractorFlowResize(AttractorFlow* af, int width, int height);

// Reinitialize agents to random positions, clear trails
void AttractorFlowReset(AttractorFlow* af);

// Apply config changes (call before update if config may have changed)
// Handles agent count changes (buffer reallocation)
void AttractorFlowApplyConfig(AttractorFlow* af, const AttractorFlowConfig* newConfig);

// Begin/end drawing to trail map (for feedback injection)
bool AttractorFlowBeginTrailMapDraw(AttractorFlow* af);
void AttractorFlowEndTrailMapDraw(AttractorFlow* af);

#endif // ATTRACTOR_FLOW_H
