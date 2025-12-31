#include "curl_flow.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/curl_flow_agents.glsl";
static const char* TRAIL_SHADER_PATH = "shaders/physarum_trail.glsl";

static char* LoadShaderSource(const char* path)
{
    char* source = LoadFileText(path);
    if (source == NULL) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to load shader: %s", path);
    }
    return source;
}

static void RGBToHSV(Color c, float* outH, float* outS, float* outV)
{
    const float r = c.r / 255.0f;
    const float g = c.g / 255.0f;
    const float b = c.b / 255.0f;
    const float maxC = fmaxf(r, fmaxf(g, b));
    const float minC = fminf(r, fminf(g, b));
    const float delta = maxC - minC;

    *outV = maxC;
    *outS = (maxC > 0.00001f) ? (delta / maxC) : 0.0f;

    if (delta < 0.00001f) {
        *outH = 0.0f;
        return;
    }

    float hue;
    if (maxC == r) {
        hue = fmodf((g - b) / delta, 6.0f);
    } else if (maxC == g) {
        hue = (b - r) / delta + 2.0f;
    } else {
        hue = (r - g) / delta + 4.0f;
    }
    hue /= 6.0f;
    if (hue < 0.0f) {
        hue += 1.0f;
    }
    *outH = hue;
}

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

static bool CreateTrailMap(RenderTexture2D* trailMap, int width, int height)
{
    trailMap->id = rlLoadFramebuffer();
    if (trailMap->id == 0) {
        return false;
    }

    rlEnableFramebuffer(trailMap->id);
    trailMap->texture.id = rlLoadTexture(NULL, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
    trailMap->texture.width = width;
    trailMap->texture.height = height;
    trailMap->texture.mipmaps = 1;
    trailMap->texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
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

bool CurlFlowSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

static GLuint LoadComputeProgram(CurlFlow* cf)
{
    char* shaderSource = LoadShaderSource(COMPUTE_SHADER_PATH);
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

static GLuint LoadTrailProgram(CurlFlow* cf)
{
    char* shaderSource = LoadShaderSource(TRAIL_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to compile trail shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to load trail shader program");
        return 0;
    }

    cf->trailResolutionLoc = rlGetLocationUniform(program, "resolution");
    cf->trailDiffusionScaleLoc = rlGetLocationUniform(program, "diffusionScale");
    cf->trailDecayFactorLoc = rlGetLocationUniform(program, "decayFactor");
    cf->trailApplyDecayLoc = rlGetLocationUniform(program, "applyDecay");
    cf->trailDirectionLoc = rlGetLocationUniform(program, "direction");

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

    if (!CreateTrailMap(&cf->trailMap, width, height)) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to create trail map");
        goto cleanup;
    }

    if (!CreateTrailMap(&cf->trailMapTemp, width, height)) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to create trail map temp texture");
        goto cleanup;
    }

    cf->trailProgram = LoadTrailProgram(cf);
    if (cf->trailProgram == 0) {
        goto cleanup;
    }

    cf->debugShader = LoadShader(NULL, "shaders/physarum_debug.fs");
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
    UnloadRenderTexture(cf->trailMapTemp);
    UnloadRenderTexture(cf->trailMap);
    if (cf->debugShader.id != 0) {
        UnloadShader(cf->debugShader);
    }
    rlUnloadShaderProgram(cf->trailProgram);
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
        RGBToHSV(cf->config.color.solid, &h, &saturation, &value);
    } else {
        saturation = cf->config.color.rainbowSat;
        value = cf->config.color.rainbowVal;
    }
    rlSetUniform(cf->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cf->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(cf->agentBuffer, 0);
    rlBindImageTexture(cf->trailMap.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
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

    const float safeHalfLife = fmaxf(cf->config.decayHalfLife, 0.001f);
    float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);

    rlEnableShader(cf->trailProgram);

    float resolution[2] = { (float)cf->width, (float)cf->height };
    rlSetUniform(cf->trailResolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(cf->trailDiffusionScaleLoc, &cf->config.diffusionScale, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(cf->trailDecayFactorLoc, &decayFactor, RL_SHADER_UNIFORM_FLOAT, 1);

    const int workGroupsX = (cf->width + 15) / 16;
    const int workGroupsY = (cf->height + 15) / 16;

    int direction = 0;
    int applyDecay = 0;
    rlSetUniform(cf->trailDirectionLoc, &direction, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(cf->trailApplyDecayLoc, &applyDecay, RL_SHADER_UNIFORM_INT, 1);
    rlBindImageTexture(cf->trailMap.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);
    rlBindImageTexture(cf->trailMapTemp.texture.id, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    direction = 1;
    applyDecay = 1;
    rlSetUniform(cf->trailDirectionLoc, &direction, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(cf->trailApplyDecayLoc, &applyDecay, RL_SHADER_UNIFORM_INT, 1);
    rlBindImageTexture(cf->trailMapTemp.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);
    rlBindImageTexture(cf->trailMap.texture.id, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void CurlFlowResize(CurlFlow* cf, int width, int height)
{
    if (cf == NULL || (width == cf->width && height == cf->height)) {
        return;
    }

    cf->width = width;
    cf->height = height;

    UnloadRenderTexture(cf->trailMap);
    if (!CreateTrailMap(&cf->trailMap, width, height)) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to recreate trail map after resize");
    }

    UnloadRenderTexture(cf->trailMapTemp);
    if (!CreateTrailMap(&cf->trailMapTemp, width, height)) {
        TraceLog(LOG_ERROR, "CURL_FLOW: Failed to recreate trail map temp after resize");
    }

    CurlFlowReset(cf);
}

void CurlFlowReset(CurlFlow* cf)
{
    if (cf == NULL) {
        return;
    }

    ClearTrailMap(&cf->trailMap);
    ClearTrailMap(&cf->trailMapTemp);

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

        ClearTrailMap(&cf->trailMap);
        ClearTrailMap(&cf->trailMapTemp);

        TraceLog(LOG_INFO, "CURL_FLOW: Reallocated buffer for %d agents", cf->agentCount);
    }
}

void CurlFlowDrawDebug(CurlFlow* cf)
{
    if (cf == NULL || !cf->supported || !cf->config.enabled) {
        return;
    }

    if (cf->debugShader.id != 0) {
        BeginShaderMode(cf->debugShader);
    }
    DrawTextureRec(cf->trailMap.texture,
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
    BeginTextureMode(cf->trailMap);
    return true;
}

void CurlFlowEndTrailMapDraw(CurlFlow* cf)
{
    if (cf == NULL || !cf->supported || !cf->config.enabled) {
        return;
    }
    EndTextureMode();
}
