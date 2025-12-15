#include "physarum.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/physarum_agents.glsl";

static char* LoadShaderSource(const char* path)
{
    char* source = LoadFileText(path);
    if (source == NULL) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to load shader: %s", path);
    }
    return source;
}

static void InitializeAgents(PhysarumAgent* agents, int count, int width, int height)
{
    for (int i = 0; i < count; i++) {
        agents[i].x = (float)(GetRandomValue(0, width - 1));
        agents[i].y = (float)(GetRandomValue(0, height - 1));
        agents[i].heading = (float)GetRandomValue(0, 628) / 100.0f;
        agents[i]._pad = 0.0f;
    }
}

static bool CreateTrailMap(RenderTexture2D* trailMap, int width, int height)
{
    trailMap->id = rlLoadFramebuffer();
    if (trailMap->id == 0) {
        return false;
    }

    rlEnableFramebuffer(trailMap->id);
    trailMap->texture.id = rlLoadTexture(NULL, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
    trailMap->texture.width = width;
    trailMap->texture.height = height;
    trailMap->texture.mipmaps = 1;
    trailMap->texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R32;
    rlFramebufferAttach(trailMap->id, trailMap->texture.id, RL_ATTACHMENT_COLOR_CHANNEL0,
                        RL_ATTACHMENT_TEXTURE2D, 0);

    if (!rlFramebufferComplete(trailMap->id)) {
        rlUnloadFramebuffer(trailMap->id);
        rlUnloadTexture(trailMap->texture.id);
        trailMap->id = 0;
        trailMap->texture.id = 0;
        return false;
    }

    rlDisableFramebuffer();
    trailMap->depth.id = 0;

    BeginTextureMode(*trailMap);
    ClearBackground(BLACK);
    EndTextureMode();

    return true;
}

static void ClearTrailMap(RenderTexture2D* trailMap)
{
    BeginTextureMode(*trailMap);
    ClearBackground(BLACK);
    EndTextureMode();
}

bool PhysarumSupported(void)
{
    int version = rlGetVersion();
    return version == RL_OPENGL_43;
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
    p->config = config ? *config : PhysarumConfig{};
    p->agentCount = p->config.agentCount;

    if (p->agentCount < 1) {
        p->agentCount = 1;
    }
    p->time = 0.0f;
    p->supported = true;

    char* shaderSource = LoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        free(p);
        return NULL;
    }

    unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to compile compute shader");
        free(p);
        return NULL;
    }

    p->computeProgram = rlLoadComputeShaderProgram(shaderId);
    if (p->computeProgram == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to load compute shader program");
        free(p);
        return NULL;
    }

    p->resolutionLoc = rlGetLocationUniform(p->computeProgram, "resolution");
    p->sensorDistanceLoc = rlGetLocationUniform(p->computeProgram, "sensorDistance");
    p->sensorAngleLoc = rlGetLocationUniform(p->computeProgram, "sensorAngle");
    p->turningAngleLoc = rlGetLocationUniform(p->computeProgram, "turningAngle");
    p->stepSizeLoc = rlGetLocationUniform(p->computeProgram, "stepSize");
    p->depositAmountLoc = rlGetLocationUniform(p->computeProgram, "depositAmount");
    p->timeLoc = rlGetLocationUniform(p->computeProgram, "time");

    if (!CreateTrailMap(&p->trailMap, width, height)) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to create trail map");
        rlUnloadShaderProgram(p->computeProgram);
        free(p);
        return NULL;
    }

    p->debugShader = LoadShader(NULL, "shaders/physarum_debug.fs");
    if (p->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "PHYSARUM: Failed to load debug shader, using default");
    }

    PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        if (p->debugShader.id != 0) {
            UnloadShader(p->debugShader);
        }
        UnloadRenderTexture(p->trailMap);
        rlUnloadShaderProgram(p->computeProgram);
        free(p);
        return NULL;
    }

    InitializeAgents(agents, p->agentCount, width, height);
    p->agentBuffer = rlLoadShaderBuffer(p->agentCount * sizeof(PhysarumAgent), agents, RL_DYNAMIC_COPY);
    free(agents);

    if (p->agentBuffer == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to create agent SSBO");
        if (p->debugShader.id != 0) {
            UnloadShader(p->debugShader);
        }
        UnloadRenderTexture(p->trailMap);
        rlUnloadShaderProgram(p->computeProgram);
        free(p);
        return NULL;
    }

    TraceLog(LOG_INFO, "PHYSARUM: Initialized with %d agents at %dx%d", p->agentCount, width, height);
    return p;
}

void PhysarumUninit(Physarum* p)
{
    if (p == NULL) {
        return;
    }

    rlUnloadShaderBuffer(p->agentBuffer);
    UnloadRenderTexture(p->trailMap);
    if (p->debugShader.id != 0) {
        UnloadShader(p->debugShader);
    }
    rlUnloadShaderProgram(p->computeProgram);
    free(p);
}

void PhysarumUpdate(Physarum* p, float deltaTime)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }

    p->time += deltaTime;

    rlEnableShader(p->computeProgram);

    float resolution[2] = { (float)p->width, (float)p->height };
    rlSetUniform(p->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(p->sensorDistanceLoc, &p->config.sensorDistance, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->sensorAngleLoc, &p->config.sensorAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->turningAngleLoc, &p->config.turningAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->stepSizeLoc, &p->config.stepSize, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->depositAmountLoc, &p->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->timeLoc, &p->time, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(p->agentBuffer, 0);
    rlBindImageTexture(p->trailMap.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, false);

    const int workGroupSize = 1024;
    const int numGroups = (p->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    // Ensure compute writes are visible to both image operations and texture fetches
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void PhysarumDrawDebug(Physarum* p)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }

    if (p->debugShader.id != 0) {
        BeginShaderMode(p->debugShader);
    }
    DrawTextureRec(p->trailMap.texture,
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

    UnloadRenderTexture(p->trailMap);
    if (!CreateTrailMap(&p->trailMap, width, height)) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to recreate trail map after resize");
    }

    PhysarumReset(p);
}

void PhysarumReset(Physarum* p)
{
    if (p == NULL) {
        return;
    }

    ClearTrailMap(&p->trailMap);

    PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, p->agentCount, p->width, p->height);
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

    bool needsBufferRealloc = (newAgentCount != p->agentCount);

    p->config = *newConfig;

    if (needsBufferRealloc) {
        rlUnloadShaderBuffer(p->agentBuffer);
        p->agentCount = newAgentCount;

        PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
        if (agents == NULL) {
            p->agentBuffer = 0;
            return;
        }

        InitializeAgents(agents, p->agentCount, p->width, p->height);
        p->agentBuffer = rlLoadShaderBuffer(p->agentCount * sizeof(PhysarumAgent), agents, RL_DYNAMIC_COPY);
        free(agents);

        ClearTrailMap(&p->trailMap);

        TraceLog(LOG_INFO, "PHYSARUM: Reallocated buffer for %d agents", p->agentCount);
    }
}
