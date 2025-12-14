#ifndef PHYSARUM_H
#define PHYSARUM_H

#include <stdbool.h>
#include "raylib.h"

typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    float _pad;
} PhysarumAgent;

typedef struct PhysarumConfig {
    int agentCount = 100000;
    float sensorDistance = 20.0f;
    float sensorAngle = 0.5f;
    float turningAngle = 0.3f;
    float stepSize = 1.5f;
    float depositAmount = 1.0f;
} PhysarumConfig;

typedef struct Physarum {
    unsigned int agentBuffer;
    unsigned int computeProgram;
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
// target: the texture agents sense from and deposit to (typically accumTexture)
void PhysarumUpdate(Physarum* p, float deltaTime, RenderTexture2D* target);

// Update dimensions (call when window resizes)
void PhysarumResize(Physarum* p, int width, int height);

// Reinitialize agents to random positions
void PhysarumReset(Physarum* p);

#endif // PHYSARUM_H
