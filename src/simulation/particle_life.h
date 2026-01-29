#ifndef PARTICLE_LIFE_H
#define PARTICLE_LIFE_H

#include <stdbool.h>
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"

typedef struct TrailMap TrailMap;

typedef struct ParticleLifeAgent {
    float x;        // Position X
    float y;        // Position Y
    float z;        // Position Z
    float vx;       // Velocity X
    float vy;       // Velocity Y
    float vz;       // Velocity Z
    float hue;      // Color hue derived from species (0-1)
    int species;    // Species index 0 to speciesCount-1
} ParticleLifeAgent;  // Total: 32 bytes

typedef struct ParticleLifeConfig {
    bool enabled = false;
    int agentCount = 50000;
    int speciesCount = 6;                // Number of species (2-8)
    float rMax = 0.3f;                   // Maximum interaction radius (normalized, 0-1)
    float forceFactor = 0.5f;            // Force multiplier
    float momentum = 0.8f;               // Velocity retention per frame (0-1, higher = keeps moving)
    float beta = 0.3f;                   // Inner repulsion zone threshold (0-1)
    int attractionSeed = 12345;          // Seed for attraction matrix randomization
    float evolutionSpeed = 0.0f;         // Matrix mutation rate (0-5.0, magnitude per second)
    bool symmetricForces = false;        // Enforce matrix[A][B] == matrix[B][A]
    float boundsRadius = 1.0f;           // Spherical boundary radius (normalized)
    float boundaryStiffness = 1.0f;      // Soft boundary repulsion strength (0.1-5.0)
    // Transform: screen position (0-1 normalized, 0.5=center) and 3D rotation
    float x = 0.5f;                      // Screen X position (0.0-1.0)
    float y = 0.5f;                      // Screen Y position (0.0-1.0)
    float rotationAngleX = 0.0f;         // Rotation around X axis (radians)
    float rotationAngleY = 0.0f;         // Rotation around Y axis (radians)
    float rotationAngleZ = 0.0f;         // Rotation around Z axis (radians)
    float rotationSpeedX = 0.0f;         // Rotation speed X (rad/sec)
    float rotationSpeedY = 0.0f;         // Rotation speed Y (rad/sec)
    float rotationSpeedZ = 0.0f;         // Rotation speed Z (rad/sec)
    float projectionScale = 0.4f;        // 3D to 2D projection scale (normalized to screen)
    // Trail parameters
    float depositAmount = 0.1f;          // Trail deposit strength (0.01-0.5)
    float decayHalfLife = 1.0f;          // Seconds for 50% decay (0.1-5.0)
    int diffusionScale = 1;              // Diffusion kernel scale in pixels (0-4)
    float boostIntensity = 1.0f;         // Trail boost strength (0.0-5.0)
    EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
    ColorConfig color;
    bool debugOverlay = false;
} ParticleLifeConfig;

typedef struct ParticleLife {
    unsigned int agentBuffer;
    unsigned int computeProgram;
    TrailMap* trailMap;
    Shader debugShader;
    int agentCount;
    int width;
    int height;
    // Agent shader uniform locations
    int resolutionLoc;
    int timeLoc;
    int numParticlesLoc;
    int numSpeciesLoc;
    int rMaxLoc;
    int forceFactorLoc;
    int momentumLoc;
    int betaLoc;
    int boundsRadiusLoc;
    int boundaryStiffnessLoc;
    int timeStepLoc;
    int centerLoc;
    int rotationMatrixLoc;
    int projectionScaleLoc;
    int depositAmountLoc;
    int saturationLoc;
    int valueLoc;
    int attractionMatrixLoc;
    // Runtime state
    float time;
    float rotationAccumX;  // Runtime accumulator (not saved to preset)
    float rotationAccumY;
    float rotationAccumZ;
    // Persistent attraction matrix (MAX_SPECIES=16, 16*16=256)
    float attractionMatrix[256];
    int lastSeed;
    unsigned int evolutionFrameCounter;
    ParticleLifeConfig config;
    bool supported;
} ParticleLife;

// Check if compute shaders are supported (OpenGL 4.3+)
bool ParticleLifeSupported(void);

// Initialize particle life simulation
// Returns NULL if compute shaders not supported or allocation fails
ParticleLife* ParticleLifeInit(int width, int height, const ParticleLifeConfig* config);

// Clean up particle life resources
void ParticleLifeUninit(ParticleLife* pl);

// Dispatch compute shader to update agents
void ParticleLifeUpdate(ParticleLife* pl, float deltaTime);

// Process trails with diffusion and decay (call after ParticleLifeUpdate)
void ParticleLifeProcessTrails(ParticleLife* pl, float deltaTime);

// Draw debug overlay (trail map visualization)
void ParticleLifeDrawDebug(ParticleLife* pl);

// Update dimensions (call when window resizes)
void ParticleLifeResize(ParticleLife* pl, int width, int height);

// Reinitialize agents to random positions, regenerate attraction matrix, clear trails
void ParticleLifeReset(ParticleLife* pl);

// Apply config changes (call before update if config may have changed)
// Handles agent count and species count changes (buffer/matrix reallocation)
void ParticleLifeApplyConfig(ParticleLife* pl, const ParticleLifeConfig* newConfig);

#endif // PARTICLE_LIFE_H
