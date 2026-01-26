#ifndef PHYSARUM_H
#define PHYSARUM_H

#include <stdbool.h>
#include "raylib.h"
#include "bounds_mode.h"
#include "render/blend_mode.h"
#include "render/color_config.h"

typedef struct TrailMap TrailMap;

typedef enum {
    PHYSARUM_WALK_NORMAL = 0,      // Fixed step = stepSize
    PHYSARUM_WALK_LEVY = 1,        // Power-law: stepSize * pow(u, -1/alpha)
    PHYSARUM_WALK_ADAPTIVE = 2,    // Step scales with local density
    PHYSARUM_WALK_CAUCHY = 3,      // Cauchy distribution (heavier tails than Levy)
    PHYSARUM_WALK_EXPONENTIAL = 4, // Exponential distribution
    PHYSARUM_WALK_GAUSSIAN = 5,    // Gaussian distribution around stepSize
    PHYSARUM_WALK_SPRINT = 6,      // Step scales with heading change
    PHYSARUM_WALK_GRADIENT = 7,    // Step scales with local gradient magnitude
} PhysarumWalkMode;

typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float _pad[4];     // Pad to 32 bytes for GPU alignment
} PhysarumAgent;

typedef struct PhysarumConfig {
    bool enabled = false;
    PhysarumBoundsMode boundsMode = PHYSARUM_BOUNDS_TOROIDAL;
    int agentCount = 100000;
    float sensorDistance = 20.0f;
    float sensorDistanceVariance = 0.0f;  // Gaussian stddev for sensing distance (0 = uniform)
    float sensorAngle = 0.5f;
    float turningAngle = 0.3f;
    float stepSize = 1.5f;
    PhysarumWalkMode walkMode = PHYSARUM_WALK_NORMAL;  // Agent step-size strategy
    float levyAlpha = 1.5f;  // Power-law exponent for step lengths (mode 1)
    float densityResponse = 1.5f;  // Step scale factor (mode 2)
    float cauchyScale = 0.5f;  // Cauchy distribution scale (mode 3)
    float expScale = 1.0f;  // Exponential distribution scale (mode 4)
    float gaussianVariance = 0.3f;  // Gaussian variance around stepSize (mode 5)
    float sprintFactor = 2.0f;  // Step multiplier per radian turned (mode 6)
    float gradientBoost = 3.0f;  // Step multiplier at max gradient (mode 7)
    float depositAmount = 0.05f;
    float decayHalfLife = 0.5f;  // Seconds for 50% decay (0.1-5.0 range)
    int diffusionScale = 1;      // Diffusion kernel scale in pixels (0-4 range)
    float boostIntensity = 1.0f; // Trail boost strength (0.0-5.0)
    EffectBlendMode blendMode = EFFECT_BLEND_SCREEN; // Blend mode for trail compositing
    float accumSenseBlend = 0.0f; // Blend between trail (0) and accum (1) sensing
    float repulsionStrength = 0.0f; // Opposite-hue repulsion: 0 = soft clustering, 1 = hard territories
    float samplingExponent = 0.0f; // MCPM mutation probability exponent (0 = deterministic, 1-10 = stochastic)
    bool vectorSteering = false; // Use vector-based steering (smoother, repulsion pushes away)
    bool respawnMode = false;    // Teleport to target instead of redirect heading
    float gravityStrength = 0.0f; // Continuous inward force toward center (0-1)
    float orbitOffset = 0.0f;    // Per-species angular separation in species orbit mode (0-1)
    int attractorCount = 4;      // Number of attractor points for multi-home mode (2-8)
    float lissajousAmplitude = 0.1f;
    float lissajousFreqX = 0.05f;
    float lissajousFreqY = 0.08f;
    float lissajousBaseRadius = 0.3f;
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
    int sensorDistanceVarianceLoc;
    int sensorAngleLoc;
    int turningAngleLoc;
    int stepSizeLoc;
    int levyAlphaLoc;
    int depositAmountLoc;
    int timeLoc;
    int saturationLoc;
    int valueLoc;
    int accumSenseBlendLoc;
    int repulsionStrengthLoc;
    int samplingExponentLoc;
    int vectorSteeringLoc;
    int boundsModeLoc;
    int attractorCountLoc;
    int respawnModeLoc;
    int gravityStrengthLoc;
    int orbitOffsetLoc;
    int attractorsLoc;
    int walkModeLoc;
    int densityResponseLoc;
    int cauchyScaleLoc;
    int expScaleLoc;
    int gaussianVarianceLoc;
    int sprintFactorLoc;
    int gradientBoostLoc;
    float time;
    float lissajousPhase;
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
