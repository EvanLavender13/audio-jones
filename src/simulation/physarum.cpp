#include "physarum.h"
#include "trail_map.h"
#include "shader_utils.h"
#include "render/color_config.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/physarum_agents.glsl";

static void InitializeAgents(PhysarumAgent* agents, int count, int width, int height, const ColorConfig* color)
{
    for (int i = 0; i < count; i++) {
        agents[i].x = (float)(GetRandomValue(0, width - 1));
        agents[i].y = (float)(GetRandomValue(0, height - 1));
        agents[i].heading = (float)GetRandomValue(0, 628) / 100.0f;
        agents[i].hue = ColorConfigAgentHue(color, i, count);
    }
}

bool PhysarumSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

// Load and link the agent compute shader, returning the program ID (0 on failure)
static GLuint LoadComputeProgram(Physarum* p)
{
    char* shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to compile compute shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to load compute shader program");
        return 0;
    }

    p->resolutionLoc = rlGetLocationUniform(program, "resolution");
    p->sensorDistanceLoc = rlGetLocationUniform(program, "sensorDistance");
    p->sensorDistanceVarianceLoc = rlGetLocationUniform(program, "sensorDistanceVariance");
    p->sensorAngleLoc = rlGetLocationUniform(program, "sensorAngle");
    p->turningAngleLoc = rlGetLocationUniform(program, "turningAngle");
    p->stepSizeLoc = rlGetLocationUniform(program, "stepSize");
    p->levyAlphaLoc = rlGetLocationUniform(program, "levyAlpha");
    p->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
    p->timeLoc = rlGetLocationUniform(program, "time");
    p->saturationLoc = rlGetLocationUniform(program, "saturation");
    p->valueLoc = rlGetLocationUniform(program, "value");
    p->accumSenseBlendLoc = rlGetLocationUniform(program, "accumSenseBlend");
    p->repulsionStrengthLoc = rlGetLocationUniform(program, "repulsionStrength");
    p->samplingExponentLoc = rlGetLocationUniform(program, "samplingExponent");
    p->vectorSteeringLoc = rlGetLocationUniform(program, "vectorSteering");
    p->boundsModeLoc = rlGetLocationUniform(program, "boundsMode");
    p->attractorCountLoc = rlGetLocationUniform(program, "attractorCount");
    p->respawnModeLoc = rlGetLocationUniform(program, "respawnMode");
    p->gravityStrengthLoc = rlGetLocationUniform(program, "gravityStrength");
    p->orbitOffsetLoc = rlGetLocationUniform(program, "orbitOffset");

    return program;
}

// Create and upload the agent SSBO, returning the buffer ID (0 on failure)
static GLuint CreateAgentBuffer(int agentCount, int width, int height, const ColorConfig* color)
{
    PhysarumAgent* agents = (PhysarumAgent*)malloc(agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        return 0;
    }

    InitializeAgents(agents, agentCount, width, height, color);
    const GLuint buffer = rlLoadShaderBuffer(agentCount * sizeof(PhysarumAgent), agents, RL_DYNAMIC_COPY);
    free(agents);

    if (buffer == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to create agent SSBO");
    }
    return buffer;
}

Physarum* PhysarumInit(int width, int height, const PhysarumConfig* config)
{
    if (!PhysarumSupported()) {
        TraceLog(LOG_WARNING, "PHYSARUM: Compute shaders not supported (requires OpenGL 4.3)");
        return NULL;
    }

    Physarum* p = (Physarum*)calloc(1, sizeof(Physarum));
    if (p == NULL) {
        return NULL;
    }

    p->width = width;
    p->height = height;
    p->config = (config != NULL) ? *config : PhysarumConfig{};
    p->agentCount = p->config.agentCount;
    if (p->agentCount < 1) {
        p->agentCount = 1;
    }
    p->time = 0.0f;
    p->supported = true;

    p->computeProgram = LoadComputeProgram(p);
    if (p->computeProgram == 0) {
        goto cleanup;
    }

    p->trailMap = TrailMapInit(width, height);
    if (p->trailMap == NULL) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to create trail map");
        goto cleanup;
    }

    p->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
    if (p->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "PHYSARUM: Failed to load debug shader, using default");
    }

    p->agentBuffer = CreateAgentBuffer(p->agentCount, width, height, &p->config.color);
    if (p->agentBuffer == 0) {
        goto cleanup;
    }

    TraceLog(LOG_INFO, "PHYSARUM: Initialized with %d agents at %dx%d", p->agentCount, width, height);
    return p;

cleanup:
    PhysarumUninit(p);
    return NULL;
}

void PhysarumUninit(Physarum* p)
{
    if (p == NULL) {
        return;
    }

    rlUnloadShaderBuffer(p->agentBuffer);
    TrailMapUninit(p->trailMap);
    if (p->debugShader.id != 0) {
        UnloadShader(p->debugShader);
    }
    rlUnloadShaderProgram(p->computeProgram);
    free(p);
}

void PhysarumUpdate(Physarum* p, float deltaTime, Texture2D accumTexture, Texture2D fftTexture)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }

    p->time += deltaTime;

    rlEnableShader(p->computeProgram);

    float resolution[2] = { (float)p->width, (float)p->height };
    rlSetUniform(p->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(p->sensorDistanceLoc, &p->config.sensorDistance, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->sensorDistanceVarianceLoc, &p->config.sensorDistanceVariance, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->sensorAngleLoc, &p->config.sensorAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->turningAngleLoc, &p->config.turningAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->stepSizeLoc, &p->config.stepSize, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->levyAlphaLoc, &p->config.levyAlpha, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->depositAmountLoc, &p->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->timeLoc, &p->time, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->accumSenseBlendLoc, &p->config.accumSenseBlend, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->repulsionStrengthLoc, &p->config.repulsionStrength, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->samplingExponentLoc, &p->config.samplingExponent, RL_SHADER_UNIFORM_FLOAT, 1);
    float vectorSteeringVal = p->config.vectorSteering ? 1.0f : 0.0f;
    rlSetUniform(p->vectorSteeringLoc, &vectorSteeringVal, RL_SHADER_UNIFORM_FLOAT, 1);
    int boundsMode = (int)p->config.boundsMode;
    rlSetUniform(p->boundsModeLoc, &boundsMode, RL_SHADER_UNIFORM_INT, 1);
    int attractorCount = p->config.attractorCount;
    rlSetUniform(p->attractorCountLoc, &attractorCount, RL_SHADER_UNIFORM_INT, 1);
    float respawnModeVal = p->config.respawnMode ? 1.0f : 0.0f;
    rlSetUniform(p->respawnModeLoc, &respawnModeVal, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->gravityStrengthLoc, &p->config.gravityStrength, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->orbitOffsetLoc, &p->config.orbitOffset, RL_SHADER_UNIFORM_FLOAT, 1);

    float saturation;
    float value;
    ColorConfigGetSV(&p->config.color, &saturation, &value);
    rlSetUniform(p->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(p->agentBuffer, 0);
    rlBindImageTexture(TrailMapGetTexture(p->trailMap).id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, accumTexture.id);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, fftTexture.id);

    const int workGroupSize = 1024;
    const int numGroups = (p->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    // Ensure compute writes are visible to both image operations and texture fetches
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void PhysarumProcessTrails(Physarum* p, float deltaTime)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }

    TrailMapProcess(p->trailMap, deltaTime, p->config.decayHalfLife, p->config.diffusionScale);
}

void PhysarumDrawDebug(Physarum* p)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }

    const Texture2D trailTex = TrailMapGetTexture(p->trailMap);
    if (p->debugShader.id != 0) {
        BeginShaderMode(p->debugShader);
    }
    DrawTextureRec(trailTex,
        {0, 0, (float)p->width, (float)-p->height},
        {0, 0}, WHITE);
    if (p->debugShader.id != 0) {
        EndShaderMode();
    }
}

void PhysarumResize(Physarum* p, int width, int height)
{
    if (p == NULL || (width == p->width && height == p->height)) {
        return;
    }

    p->width = width;
    p->height = height;

    TrailMapResize(p->trailMap, width, height);

    PhysarumReset(p);
}

void PhysarumReset(Physarum* p)
{
    if (p == NULL) {
        return;
    }

    TrailMapClear(p->trailMap);

    PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, p->agentCount, p->width, p->height, &p->config.color);
    rlUpdateShaderBuffer(p->agentBuffer, agents, p->agentCount * sizeof(PhysarumAgent), 0);
    free(agents);
}

void PhysarumApplyConfig(Physarum* p, const PhysarumConfig* newConfig)
{
    if (p == NULL || newConfig == NULL) {
        return;
    }

    int newAgentCount = newConfig->agentCount;
    if (newAgentCount < 1) {
        newAgentCount = 1;
    }

    const bool needsBufferRealloc = (newAgentCount != p->agentCount);
    const bool needsHueReinit = !ColorConfigEquals(&p->config.color, &newConfig->color);

    p->config = *newConfig;

    if (needsBufferRealloc) {
        rlUnloadShaderBuffer(p->agentBuffer);
        p->agentCount = newAgentCount;

        PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
        if (agents == NULL) {
            p->agentBuffer = 0;
            return;
        }

        InitializeAgents(agents, p->agentCount, p->width, p->height, &p->config.color);
        p->agentBuffer = rlLoadShaderBuffer(p->agentCount * sizeof(PhysarumAgent), agents, RL_DYNAMIC_COPY);
        free(agents);

        TrailMapClear(p->trailMap);

        TraceLog(LOG_INFO, "PHYSARUM: Reallocated buffer for %d agents", p->agentCount);
    } else if (needsHueReinit) {
        PhysarumReset(p);
    }
}

bool PhysarumBeginTrailMapDraw(Physarum* p)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return false;
    }
    return TrailMapBeginDraw(p->trailMap);
}

void PhysarumEndTrailMapDraw(Physarum* p)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }
    TrailMapEndDraw(p->trailMap);
}
