#include "curl_flow.h"
#include "trail_map.h"
#include "shader_utils.h"
#include "render/color_config.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/curl_flow_agents.glsl";

static void InitializeAgents(CurlFlowAgent* agents, int count, int width, int height)
{
    for (int i = 0; i < count; i++) {
        agents[i].x = (float)(GetRandomValue(0, width - 1));
        agents[i].y = (float)(GetRandomValue(0, height - 1));
        agents[i].velocityAngle = 0.0f;
        agents[i]._pad[0] = 0.0f;
        agents[i]._pad[1] = 0.0f;
        agents[i]._pad[2] = 0.0f;
        agents[i]._pad[3] = 0.0f;
        agents[i]._pad[4] = 0.0f;
    }
}

bool CurlFlowSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

static GLuint LoadComputeProgram(CurlFlow* cf)
{
    char* shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to compile compute shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to load compute shader program");
        return 0;
    }

    cf->resolutionLoc = rlGetLocationUniform(program, "resolution");
    cf->timeLoc = rlGetLocationUniform(program, "time");
    cf->noiseFrequencyLoc = rlGetLocationUniform(program, "noiseFrequency");
    cf->noiseEvolutionLoc = rlGetLocationUniform(program, "noiseEvolution");
    cf->trailInfluenceLoc = rlGetLocationUniform(program, "trailInfluence");
    cf->stepSizeLoc = rlGetLocationUniform(program, "stepSize");
    cf->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
    cf->saturationLoc = rlGetLocationUniform(program, "saturation");
    cf->valueLoc = rlGetLocationUniform(program, "value");
    cf->accumSenseBlendLoc = rlGetLocationUniform(program, "accumSenseBlend");

    return program;
}

static GLuint CreateAgentBuffer(int agentCount, int width, int height)
{
    CurlFlowAgent* agents = (CurlFlowAgent*)malloc(agentCount * sizeof(CurlFlowAgent));
    if (agents == NULL) {
        return 0;
    }

    InitializeAgents(agents, agentCount, width, height);
    const GLuint buffer = rlLoadShaderBuffer(agentCount * sizeof(CurlFlowAgent), agents, RL_DYNAMIC_COPY);
    free(agents);

    if (buffer == 0) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to create agent SSBO");
    }
    return buffer;
}

CurlFlow* CurlFlowInit(int width, int height, const CurlFlowConfig* config)
{
    if (!CurlFlowSupported()) {
        TraceLog(LOG_WARNING, "CURL_FLOW: Compute shaders not supported (requires OpenGL 4.3)");
        return NULL;
    }

    CurlFlow* cf = (CurlFlow*)calloc(1, sizeof(CurlFlow));
    if (cf == NULL) {
        return NULL;
    }

    cf->width = width;
    cf->height = height;
    cf->config = (config != NULL) ? *config : CurlFlowConfig{};
    cf->agentCount = cf->config.agentCount;
    if (cf->agentCount < 1) {
        cf->agentCount = 1;
    }
    cf->time = 0.0f;
    cf->supported = true;

    cf->computeProgram = LoadComputeProgram(cf);
    if (cf->computeProgram == 0) {
        goto cleanup;
    }

    cf->trailMap = TrailMapInit(width, height);
    if (cf->trailMap == NULL) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to create trail map");
        goto cleanup;
    }

    cf->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
    if (cf->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "CURL_FLOW: Failed to load debug shader, using default");
    }

    cf->agentBuffer = CreateAgentBuffer(cf->agentCount, width, height);
    if (cf->agentBuffer == 0) {
        goto cleanup;
    }

    TraceLog(LOG_INFO, "CURL_FLOW: Initialized with %d agents at %dx%d", cf->agentCount, width, height);
    return cf;

cleanup:
    CurlFlowUninit(cf);
    return NULL;
}

void CurlFlowUninit(CurlFlow* cf)
{
    if (cf == NULL) {
        return;
    }

    rlUnloadShaderBuffer(cf->agentBuffer);
    TrailMapUninit(cf->trailMap);
    if (cf->debugShader.id != 0) {
        UnloadShader(cf->debugShader);
    }
    rlUnloadShaderProgram(cf->computeProgram);
    free(cf);
}

void CurlFlowUpdate(CurlFlow* cf, float deltaTime, Texture2D accumTexture)
{
    if (cf == NULL || !cf->supported || !cf->config.enabled) {
        return;
    }

    cf->time += deltaTime;

    rlEnableShader(cf->computeProgram);

    float resolution[2] = { (float)cf->width, (float)cf->height };
    rlSetUniform(cf->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(cf->timeLoc, &cf->time, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->noiseFrequencyLoc, &cf->config.noiseFrequency, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->noiseEvolutionLoc, &cf->config.noiseEvolution, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->trailInfluenceLoc, &cf->config.trailInfluence, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->stepSizeLoc, &cf->config.stepSize, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->depositAmountLoc, &cf->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->accumSenseBlendLoc, &cf->config.accumSenseBlend, RL_SHADER_UNIFORM_FLOAT, 1);

    float saturation;
    float value;
    if (cf->config.color.mode == COLOR_MODE_SOLID) {
        float h;
        ColorConfigRGBToHSV(cf->config.color.solid, &h, &saturation, &value);
    } else {
        saturation = cf->config.color.rainbowSat;
        value = cf->config.color.rainbowVal;
    }
    rlSetUniform(cf->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(cf->agentBuffer, 0);
    rlBindImageTexture(TrailMapGetTexture(cf->trailMap).id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, accumTexture.id);

    const int workGroupSize = 1024;
    const int numGroups = (cf->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void CurlFlowProcessTrails(CurlFlow* cf, float deltaTime)
{
    if (cf == NULL || !cf->supported || !cf->config.enabled) {
        return;
    }

    TrailMapProcess(cf->trailMap, deltaTime, cf->config.decayHalfLife, cf->config.diffusionScale);
}

void CurlFlowResize(CurlFlow* cf, int width, int height)
{
    if (cf == NULL || (width == cf->width && height == cf->height)) {
        return;
    }

    cf->width = width;
    cf->height = height;

    TrailMapResize(cf->trailMap, width, height);

    CurlFlowReset(cf);
}

void CurlFlowReset(CurlFlow* cf)
{
    if (cf == NULL) {
        return;
    }

    TrailMapClear(cf->trailMap);

    CurlFlowAgent* agents = (CurlFlowAgent*)malloc(cf->agentCount * sizeof(CurlFlowAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, cf->agentCount, cf->width, cf->height);
    rlUpdateShaderBuffer(cf->agentBuffer, agents, cf->agentCount * sizeof(CurlFlowAgent), 0);
    free(agents);
}

void CurlFlowApplyConfig(CurlFlow* cf, const CurlFlowConfig* newConfig)
{
    if (cf == NULL || newConfig == NULL) {
        return;
    }

    int newAgentCount = newConfig->agentCount;
    if (newAgentCount < 1) {
        newAgentCount = 1;
    }

    const bool needsBufferRealloc = (newAgentCount != cf->agentCount);

    cf->config = *newConfig;

    if (needsBufferRealloc) {
        rlUnloadShaderBuffer(cf->agentBuffer);
        cf->agentCount = newAgentCount;

        CurlFlowAgent* agents = (CurlFlowAgent*)malloc(cf->agentCount * sizeof(CurlFlowAgent));
        if (agents == NULL) {
            cf->agentBuffer = 0;
            return;
        }

        InitializeAgents(agents, cf->agentCount, cf->width, cf->height);
        cf->agentBuffer = rlLoadShaderBuffer(cf->agentCount * sizeof(CurlFlowAgent), agents, RL_DYNAMIC_COPY);
        free(agents);

        TrailMapClear(cf->trailMap);

        TraceLog(LOG_INFO, "CURL_FLOW: Reallocated buffer for %d agents", cf->agentCount);
    }
}

void CurlFlowDrawDebug(CurlFlow* cf)
{
    if (cf == NULL || !cf->supported || !cf->config.enabled) {
        return;
    }

    Texture2D trailTex = TrailMapGetTexture(cf->trailMap);
    if (cf->debugShader.id != 0) {
        BeginShaderMode(cf->debugShader);
    }
    DrawTextureRec(trailTex,
        {0, 0, (float)cf->width, (float)-cf->height},
        {0, 0}, WHITE);
    if (cf->debugShader.id != 0) {
        EndShaderMode();
    }
}

bool CurlFlowBeginTrailMapDraw(CurlFlow* cf)
{
    if (cf == NULL || !cf->supported || !cf->config.enabled) {
        return false;
    }
    return TrailMapBeginDraw(cf->trailMap);
}

void CurlFlowEndTrailMapDraw(CurlFlow* cf)
{
    if (cf == NULL || !cf->supported || !cf->config.enabled) {
        return;
    }
    TrailMapEndDraw(cf->trailMap);
}
