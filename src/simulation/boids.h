#ifndef BOIDS_H
#define BOIDS_H

#include <stdbool.h>
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"

typedef struct TrailMap TrailMap;

typedef struct BoidAgent {
    float x;           // Position X
    float y;           // Position Y
    float vx;          // Velocity X
    float vy;          // Velocity Y
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float _pad[3];     // Pad to 32 bytes for GPU alignment
} BoidAgent;

typedef struct BoidsConfig {
    bool enabled = false;
    int agentCount = 10000;
    float perceptionRadius = 50.0f;   // Neighbor detection range (10-100 px)
    float separationRadius = 20.0f;   // Crowding avoidance range (5-50 px)
    float cohesionWeight = 1.0f;      // Strength of center-seeking (0-2)
    float separationWeight = 1.5f;    // Strength of avoidance (0-2)
    float alignmentWeight = 1.0f;     // Strength of velocity matching (0-2)
    float hueAffinity = 1.0f;         // How strongly like-colors flock (0-2, 0=ignore hue)
    float maxSpeed = 4.0f;            // Velocity clamp (1-10)
    float minSpeed = 0.5f;            // Prevents stalling (0-2)
    float depositAmount = 0.05f;      // Trail brightness (0.01-0.5)
    float decayHalfLife = 0.5f;       // Trail persistence in seconds (0.1-5.0)
    int diffusionScale = 1;           // Blur kernel size (0-4)
    float boostIntensity = 0.0f;      // BlendCompositor intensity (0-5)
    EffectBlendMode blendMode = EFFECT_BLEND_BOOST;
    bool debugOverlay = false;
    ColorConfig color;
} BoidsConfig;

typedef struct Boids {
    unsigned int agentBuffer;
    unsigned int computeProgram;
    TrailMap* trailMap;
    Shader debugShader;
    int agentCount;
    int width;
    int height;

    // Uniform locations
    int resolutionLoc;
    int perceptionRadiusLoc;
    int separationRadiusLoc;
    int cohesionWeightLoc;
    int separationWeightLoc;
    int alignmentWeightLoc;
    int hueAffinityLoc;
    int maxSpeedLoc;
    int minSpeedLoc;
    int depositAmountLoc;
    int saturationLoc;
    int valueLoc;
    int numBoidsLoc;

    float time;
    BoidsConfig config;
    bool supported;
} Boids;

// Check if compute shaders are supported (OpenGL 4.3+)
bool BoidsSupported(void);

// Initialize boids simulation
// Returns NULL if compute shaders not supported or allocation fails
Boids* BoidsInit(int width, int height, const BoidsConfig* config);

// Clean up boids resources
void BoidsUninit(Boids* b);

// Dispatch compute shader to update agents
void BoidsUpdate(Boids* b, float deltaTime, Texture2D accumTexture, Texture2D fftTexture);

// Process trails with diffusion and decay (call after BoidsUpdate)
void BoidsProcessTrails(Boids* b, float deltaTime);

// Apply config changes (call before update if config may have changed)
// Handles agent count changes (buffer reallocation)
void BoidsApplyConfig(Boids* b, const BoidsConfig* newConfig);

// Reinitialize agents to random positions
void BoidsReset(Boids* b);

// Update dimensions (call when window resizes)
void BoidsResize(Boids* b, int width, int height);

// Begin drawing to trailMap (for waveform injection)
// Returns true if trailMap is now active for drawing
bool BoidsBeginTrailMapDraw(Boids* b);

// End drawing to trailMap
void BoidsEndTrailMapDraw(Boids* b);

// Draw debug overlay (trail texture)
void BoidsDrawDebug(Boids* b);

#endif // BOIDS_H
