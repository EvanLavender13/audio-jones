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

    PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        rlUnloadShaderProgram(p->computeProgram);
        free(p);
        return NULL;
    }

    InitializeAgents(agents, p->agentCount, width, height);
    p->agentBuffer = rlLoadShaderBuffer(p->agentCount * sizeof(PhysarumAgent), agents, RL_DYNAMIC_COPY);
    free(agents);

    if (p->agentBuffer == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to create agent SSBO");
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
    rlUnloadShaderProgram(p->computeProgram);
    free(p);
}

void PhysarumUpdate(Physarum* p, float deltaTime, RenderTexture2D* target)
{
    if (p == NULL || !p->supported || target == NULL) {
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
    rlBindImageTexture(target->texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);

    const int workGroupSize = 1024;
    const int numGroups = (p->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    rlDisableShader();
}

void PhysarumResize(Physarum* p, int width, int height)
{
    if (p == NULL || (width == p->width && height == p->height)) {
        return;
    }

    p->width = width;
    p->height = height;
    PhysarumReset(p);
}

void PhysarumReset(Physarum* p)
{
    if (p == NULL) {
        return;
    }

    PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, p->agentCount, p->width, p->height);
    rlUpdateShaderBuffer(p->agentBuffer, agents, p->agentCount * sizeof(PhysarumAgent), 0);
    free(agents);
}
