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

static void InitializeAgents(PhysarumAgent* agents, int count, int width, int height,
                             const ColorConfig* color)
{
    // Determine hue distribution based on color mode
    float hueStart = 0.0f;
    float hueRange = 1.0f;

    if (color->mode == COLOR_MODE_SOLID) {
        // Extract hue from solid color - all agents share same hue
        Vector3 hsv = ColorToHSV(color->solid);
        hueStart = hsv.x / 360.0f;
        hueRange = 0.0f;  // All agents get same hue
    } else {
        // Rainbow mode - distribute hues across range
        hueStart = color->rainbowHue / 360.0f;
        hueRange = color->rainbowRange / 360.0f;
    }

    for (int i = 0; i < count; i++) {
        agents[i].x = (float)(GetRandomValue(0, width - 1));
        agents[i].y = (float)(GetRandomValue(0, height - 1));
        agents[i].heading = (float)GetRandomValue(0, 628) / 100.0f;

        // Distribute hue across agents
        if (hueRange > 0.0f) {
            agents[i].hue = hueStart + ((float)i / (float)count) * hueRange;
            // Wrap to 0-1 range
            agents[i].hue = fmodf(agents[i].hue, 1.0f);
            if (agents[i].hue < 0.0f) {
                agents[i].hue += 1.0f;
            }
        } else {
            agents[i].hue = hueStart;
        }
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

    // Guard against zero agents
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
    p->saturationLoc = rlGetLocationUniform(p->computeProgram, "saturation");
    p->valueLoc = rlGetLocationUniform(p->computeProgram, "value");

    PhysarumAgent* agents = (PhysarumAgent*)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
        rlUnloadShaderProgram(p->computeProgram);
        free(p);
        return NULL;
    }

    InitializeAgents(agents, p->agentCount, width, height, &p->config.color);
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
    if (p == NULL || !p->supported || !p->config.enabled || target == NULL) {
        return;
    }

    p->time += deltaTime;

    // Compute saturation and value based on color mode
    float saturation = 1.0f;
    float value = 1.0f;
    if (p->config.color.mode == COLOR_MODE_SOLID) {
        Vector3 hsv = ColorToHSV(p->config.color.solid);
        saturation = hsv.y;
        value = hsv.z;
    } else {
        saturation = p->config.color.rainbowSat;
        value = p->config.color.rainbowVal;
    }

    rlEnableShader(p->computeProgram);

    float resolution[2] = { (float)p->width, (float)p->height };
    rlSetUniform(p->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(p->sensorDistanceLoc, &p->config.sensorDistance, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->sensorAngleLoc, &p->config.sensorAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->turningAngleLoc, &p->config.turningAngle, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->stepSizeLoc, &p->config.stepSize, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->depositAmountLoc, &p->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->timeLoc, &p->time, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(p->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(p->agentBuffer, 0);
    rlBindImageTexture(target->texture.id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);

    const int workGroupSize = 1024;
    const int numGroups = (p->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    // Ensure compute writes are visible to both image operations and texture fetches
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

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

    InitializeAgents(agents, p->agentCount, p->width, p->height, &p->config.color);
    rlUpdateShaderBuffer(p->agentBuffer, agents, p->agentCount * sizeof(PhysarumAgent), 0);
    free(agents);
}

static bool ColorConfigChanged(const ColorConfig* a, const ColorConfig* b)
{
    if (a->mode != b->mode) {
        return true;
    }
    if (a->mode == COLOR_MODE_SOLID) {
        return a->solid.r != b->solid.r || a->solid.g != b->solid.g ||
               a->solid.b != b->solid.b || a->solid.a != b->solid.a;
    }
    // Rainbow mode
    return a->rainbowHue != b->rainbowHue || a->rainbowRange != b->rainbowRange;
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

    bool needsReinit = false;
    bool needsBufferRealloc = false;

    // Check if agent count changed
    if (newAgentCount != p->agentCount) {
        needsBufferRealloc = true;
        needsReinit = true;
    }

    // Check if color config changed (affects hue distribution)
    if (ColorConfigChanged(&p->config.color, &newConfig->color)) {
        needsReinit = true;
    }

    // Apply config
    p->config = *newConfig;

    if (needsBufferRealloc) {
        // Reallocate SSBO with new size
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

        TraceLog(LOG_INFO, "PHYSARUM: Reallocated buffer for %d agents", p->agentCount);
    } else if (needsReinit) {
        // Just reinitialize agents with new hues
        PhysarumReset(p);
    }
}
