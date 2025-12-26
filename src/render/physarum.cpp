#include "physarum.h"
#include "gradient.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/physarum_agents.glsl";
static const char* TRAIL_SHADER_PATH = "shaders/physarum_trail.glsl";

static char* LoadShaderSource(const char* path)
{
    char* source = LoadFileText(path);
    if (source == NULL) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to load shader: %s", path);
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

static void InitializeAgents(PhysarumAgent* agents, int count, int width, int height, const ColorConfig* color)
{
    for (int i = 0; i < count; i++) {
        agents[i].x = (float)(GetRandomValue(0, width - 1));
        agents[i].y = (float)(GetRandomValue(0, height - 1));
        agents[i].heading = (float)GetRandomValue(0, 628) / 100.0f;
        agents[i].spectrumPos = (float)i / (float)count;

        float hue;
        if (color->mode == COLOR_MODE_SOLID) {
            float s;
            float v;
            RGBToHSV(color->solid, &hue, &s, &v);
            // For grayscale/low-saturation colors, distribute hues to avoid clustering
            if (s < 0.1f) {
                hue = (float)i / (float)count;
            }
        } else if (color->mode == COLOR_MODE_GRADIENT) {
            const float t = (float)i / (float)count;
            const Color sampled = GradientEvaluate(color->gradientStops, color->gradientStopCount, t);
            float s;
            float v;
            RGBToHSV(sampled, &hue, &s, &v);
        } else {
            hue = (color->rainbowHue + (i / (float)count) * color->rainbowRange) / 360.0f;
            hue = fmodf(hue, 1.0f);
            if (hue < 0.0f) {
                hue += 1.0f;
            }
        }
        agents[i].hue = hue;
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

bool PhysarumSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

// Load and link the agent compute shader, returning the program ID (0 on failure)
static GLuint LoadComputeProgram(Physarum* p)
{
    char* shaderSource = LoadShaderSource(COMPUTE_SHADER_PATH);
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
    p->sensorAngleLoc = rlGetLocationUniform(program, "sensorAngle");
    p->turningAngleLoc = rlGetLocationUniform(program, "turningAngle");
    p->stepSizeLoc = rlGetLocationUniform(program, "stepSize");
    p->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
    p->timeLoc = rlGetLocationUniform(program, "time");
    p->saturationLoc = rlGetLocationUniform(program, "saturation");
    p->valueLoc = rlGetLocationUniform(program, "value");
    p->accumSenseBlendLoc = rlGetLocationUniform(program, "accumSenseBlend");

    return program;
}

// Load and link the trail compute shader, returning the program ID (0 on failure)
static GLuint LoadTrailProgram(Physarum* p)
{
    char* shaderSource = LoadShaderSource(TRAIL_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to compile trail shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to load trail shader program");
        return 0;
    }

    p->trailResolutionLoc = rlGetLocationUniform(program, "resolution");
    p->trailDiffusionScaleLoc = rlGetLocationUniform(program, "diffusionScale");
    p->trailDecayFactorLoc = rlGetLocationUniform(program, "decayFactor");
    p->trailApplyDecayLoc = rlGetLocationUniform(program, "applyDecay");
    p->trailDirectionLoc = rlGetLocationUniform(program, "direction");

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

    if (!CreateTrailMap(&p->trailMap, width, height)) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to create trail map");
        goto cleanup;
    }

    if (!CreateTrailMap(&p->trailMapTemp, width, height)) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to create trail map temp texture");
        goto cleanup;
    }

    p->trailProgram = LoadTrailProgram(p);
    if (p->trailProgram == 0) {
        goto cleanup;
    }

    p->debugShader = LoadShader(NULL, "shaders/physarum_debug.fs");
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
    UnloadRenderTexture(p->trailMapTemp);
    UnloadRenderTexture(p->trailMap);
    if (p->debugShader.id != 0) {
        UnloadShader(p->debugShader);
    }
    rlUnloadShaderProgram(p->trailProgram);
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
    rlSetUniform(p->sensorAngleLoc, &p->config.sensorAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->turningAngleLoc, &p->config.turningAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->stepSizeLoc, &p->config.stepSize, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->depositAmountLoc, &p->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->timeLoc, &p->time, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->accumSenseBlendLoc, &p->config.accumSenseBlend, RL_SHADER_UNIFORM_FLOAT, 1);

    float saturation;
    float value;
    if (p->config.color.mode == COLOR_MODE_SOLID) {
        float h;
        RGBToHSV(p->config.color.solid, &h, &saturation, &value);
    } else {
        saturation = p->config.color.rainbowSat;
        value = p->config.color.rainbowVal;
    }
    rlSetUniform(p->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(p->agentBuffer, 0);
    rlBindImageTexture(p->trailMap.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
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

    const float safeHalfLife = fmaxf(p->config.decayHalfLife, 0.001f);
    float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);

    rlEnableShader(p->trailProgram);

    float resolution[2] = { (float)p->width, (float)p->height };
    rlSetUniform(p->trailResolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(p->trailDiffusionScaleLoc, &p->config.diffusionScale, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(p->trailDecayFactorLoc, &decayFactor, RL_SHADER_UNIFORM_FLOAT, 1);

    const int workGroupsX = (p->width + 15) / 16;
    const int workGroupsY = (p->height + 15) / 16;

    int direction = 0;
    int applyDecay = 0;
    rlSetUniform(p->trailDirectionLoc, &direction, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(p->trailApplyDecayLoc, &applyDecay, RL_SHADER_UNIFORM_INT, 1);
    rlBindImageTexture(p->trailMap.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);
    rlBindImageTexture(p->trailMapTemp.texture.id, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    direction = 1;
    applyDecay = 1;
    rlSetUniform(p->trailDirectionLoc, &direction, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(p->trailApplyDecayLoc, &applyDecay, RL_SHADER_UNIFORM_INT, 1);
    rlBindImageTexture(p->trailMapTemp.texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);
    rlBindImageTexture(p->trailMap.texture.id, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY, 1);

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

    UnloadRenderTexture(p->trailMapTemp);
    if (!CreateTrailMap(&p->trailMapTemp, width, height)) {
        TraceLog(LOG_ERROR, "PHYSARUM: Failed to recreate trail map temp after resize");
    }

    PhysarumReset(p);
}

void PhysarumReset(Physarum* p)
{
    if (p == NULL) {
        return;
    }

    ClearTrailMap(&p->trailMap);
    ClearTrailMap(&p->trailMapTemp);

    PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, p->agentCount, p->width, p->height, &p->config.color);
    rlUpdateShaderBuffer(p->agentBuffer, agents, p->agentCount * sizeof(PhysarumAgent), 0);
    free(agents);
}

// Check if color config changed in a way that requires agent hue reinitialization
static bool ColorConfigChanged(const ColorConfig* oldColor, const ColorConfig* newColor)
{
    if (newColor->mode != oldColor->mode) {
        return true;
    }
    if (oldColor->mode == COLOR_MODE_SOLID) {
        return newColor->solid.r != oldColor->solid.r ||
               newColor->solid.g != oldColor->solid.g ||
               newColor->solid.b != oldColor->solid.b;
    }
    if (oldColor->mode == COLOR_MODE_GRADIENT) {
        if (newColor->gradientStopCount != oldColor->gradientStopCount) {
            return true;
        }
        for (int i = 0; i < oldColor->gradientStopCount; i++) {
            if (newColor->gradientStops[i].position != oldColor->gradientStops[i].position ||
                newColor->gradientStops[i].color.r != oldColor->gradientStops[i].color.r ||
                newColor->gradientStops[i].color.g != oldColor->gradientStops[i].color.g ||
                newColor->gradientStops[i].color.b != oldColor->gradientStops[i].color.b) {
                return true;
            }
        }
        return false;
    }
    return newColor->rainbowHue != oldColor->rainbowHue ||
           newColor->rainbowRange != oldColor->rainbowRange;
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
    const bool needsHueReinit = ColorConfigChanged(&p->config.color, &newConfig->color);

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

        ClearTrailMap(&p->trailMap);
        ClearTrailMap(&p->trailMapTemp);

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
    BeginTextureMode(p->trailMap);
    return true;
}

void PhysarumEndTrailMapDraw(Physarum* p)
{
    if (p == NULL || !p->supported || !p->config.enabled) {
        return;
    }
    EndTextureMode();
}
