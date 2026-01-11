#include "boids.h"
#include "trail_map.h"
#include "shader_utils.h"
#include "render/gradient.h"
#include "render/color_config.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/boids_agents.glsl";

static void InitializeAgents(BoidAgent* agents, int count, int width, int height, const ColorConfig* color)
{
    for (int i = 0; i < count; i++) {
        agents[i].x = (float)(GetRandomValue(0, width - 1));
        agents[i].y = (float)(GetRandomValue(0, height - 1));

        // Random initial velocity direction with small magnitude
        const float angle = (float)GetRandomValue(0, 628) / 100.0f;
        agents[i].vx = cosf(angle) * 1.0f;
        agents[i].vy = sinf(angle) * 1.0f;

        agents[i].spectrumPos = (float)i / (float)count;

        float hue;
        if (color->mode == COLOR_MODE_SOLID) {
            float s;
            float v;
            ColorConfigRGBToHSV(color->solid, &hue, &s, &v);
            // For grayscale/low-saturation colors, distribute hues to avoid clustering
            if (s < 0.1f) {
                hue = (float)i / (float)count;
            }
        } else if (color->mode == COLOR_MODE_GRADIENT) {
            const float t = (float)i / (float)count;
            const Color sampled = GradientEvaluate(color->gradientStops, color->gradientStopCount, t);
            float s;
            float v;
            ColorConfigRGBToHSV(sampled, &hue, &s, &v);
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

bool BoidsSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

static GLuint LoadComputeProgram(Boids* b)
{
    char* shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "BOIDS: Failed to compile compute shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "BOIDS: Failed to load compute shader program");
        return 0;
    }

    b->resolutionLoc = rlGetLocationUniform(program, "resolution");
    b->perceptionRadiusLoc = rlGetLocationUniform(program, "perceptionRadius");
    b->separationRadiusLoc = rlGetLocationUniform(program, "separationRadius");
    b->cohesionWeightLoc = rlGetLocationUniform(program, "cohesionWeight");
    b->separationWeightLoc = rlGetLocationUniform(program, "separationWeight");
    b->alignmentWeightLoc = rlGetLocationUniform(program, "alignmentWeight");
    b->hueAffinityLoc = rlGetLocationUniform(program, "hueAffinity");
    b->textureWeightLoc = rlGetLocationUniform(program, "textureWeight");
    b->attractModeLoc = rlGetLocationUniform(program, "attractMode");
    b->sensorDistanceLoc = rlGetLocationUniform(program, "sensorDistance");
    b->maxSpeedLoc = rlGetLocationUniform(program, "maxSpeed");
    b->minSpeedLoc = rlGetLocationUniform(program, "minSpeed");
    b->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
    b->saturationLoc = rlGetLocationUniform(program, "saturation");
    b->valueLoc = rlGetLocationUniform(program, "value");
    b->numBoidsLoc = rlGetLocationUniform(program, "numBoids");

    return program;
}

static GLuint CreateAgentBuffer(int agentCount, int width, int height, const ColorConfig* color)
{
    BoidAgent* agents = (BoidAgent*)malloc(agentCount * sizeof(BoidAgent));
    if (agents == NULL) {
        return 0;
    }

    InitializeAgents(agents, agentCount, width, height, color);
    const GLuint buffer = rlLoadShaderBuffer(agentCount * sizeof(BoidAgent), agents, RL_DYNAMIC_COPY);
    free(agents);

    if (buffer == 0) {
        TraceLog(LOG_ERROR, "BOIDS: Failed to create agent SSBO");
    }
    return buffer;
}

Boids* BoidsInit(int width, int height, const BoidsConfig* config)
{
    if (!BoidsSupported()) {
        TraceLog(LOG_WARNING, "BOIDS: Compute shaders not supported (requires OpenGL 4.3)");
        return NULL;
    }

    Boids* b = (Boids*)calloc(1, sizeof(Boids));
    if (b == NULL) {
        return NULL;
    }

    b->width = width;
    b->height = height;
    b->config = (config != NULL) ? *config : BoidsConfig{};
    b->agentCount = b->config.agentCount;
    if (b->agentCount < 1) {
        b->agentCount = 1;
    }
    b->time = 0.0f;
    b->supported = true;

    b->computeProgram = LoadComputeProgram(b);
    if (b->computeProgram == 0) {
        goto cleanup;
    }

    b->trailMap = TrailMapInit(width, height);
    if (b->trailMap == NULL) {
        TraceLog(LOG_ERROR, "BOIDS: Failed to create trail map");
        goto cleanup;
    }

    b->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
    if (b->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "BOIDS: Failed to load debug shader, using default");
    }

    b->agentBuffer = CreateAgentBuffer(b->agentCount, width, height, &b->config.color);
    if (b->agentBuffer == 0) {
        goto cleanup;
    }

    TraceLog(LOG_INFO, "BOIDS: Initialized with %d agents at %dx%d", b->agentCount, width, height);
    return b;

cleanup:
    BoidsUninit(b);
    return NULL;
}

void BoidsUninit(Boids* b)
{
    if (b == NULL) {
        return;
    }

    rlUnloadShaderBuffer(b->agentBuffer);
    TrailMapUninit(b->trailMap);
    if (b->debugShader.id != 0) {
        UnloadShader(b->debugShader);
    }
    rlUnloadShaderProgram(b->computeProgram);
    free(b);
}

void BoidsUpdate(Boids* b, float deltaTime, Texture2D accumTexture, Texture2D fftTexture)
{
    if (b == NULL || !b->supported || !b->config.enabled) {
        return;
    }

    b->time += deltaTime;

    rlEnableShader(b->computeProgram);

    float resolution[2] = { (float)b->width, (float)b->height };
    rlSetUniform(b->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(b->perceptionRadiusLoc, &b->config.perceptionRadius, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->separationRadiusLoc, &b->config.separationRadius, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->cohesionWeightLoc, &b->config.cohesionWeight, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->separationWeightLoc, &b->config.separationWeight, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->alignmentWeightLoc, &b->config.alignmentWeight, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->hueAffinityLoc, &b->config.hueAffinity, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->textureWeightLoc, &b->config.textureWeight, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->attractModeLoc, &b->config.attractMode, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->sensorDistanceLoc, &b->config.sensorDistance, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->maxSpeedLoc, &b->config.maxSpeed, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->minSpeedLoc, &b->config.minSpeed, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->depositAmountLoc, &b->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->numBoidsLoc, &b->agentCount, RL_SHADER_UNIFORM_INT, 1);

    float saturation;
    float colorValue;
    if (b->config.color.mode == COLOR_MODE_SOLID) {
        float h;
        ColorConfigRGBToHSV(b->config.color.solid, &h, &saturation, &colorValue);
    } else {
        saturation = b->config.color.rainbowSat;
        colorValue = b->config.color.rainbowVal;
    }
    rlSetUniform(b->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->valueLoc, &colorValue, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(b->agentBuffer, 0);
    rlBindImageTexture(TrailMapGetTexture(b->trailMap).id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, accumTexture.id);

    const int workGroupSize = 1024;
    const int numGroups = (b->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void BoidsProcessTrails(Boids* b, float deltaTime)
{
    if (b == NULL || !b->supported || !b->config.enabled) {
        return;
    }

    TrailMapProcess(b->trailMap, deltaTime, b->config.decayHalfLife, b->config.diffusionScale);
}

void BoidsApplyConfig(Boids* b, const BoidsConfig* newConfig)
{
    if (b == NULL || newConfig == NULL) {
        return;
    }

    int newAgentCount = newConfig->agentCount;
    if (newAgentCount < 1) {
        newAgentCount = 1;
    }

    const bool needsBufferRealloc = (newAgentCount != b->agentCount);
    const bool needsHueReinit = !ColorConfigEquals(&b->config.color, &newConfig->color);

    b->config = *newConfig;

    if (needsBufferRealloc) {
        rlUnloadShaderBuffer(b->agentBuffer);
        b->agentCount = newAgentCount;

        BoidAgent* agents = (BoidAgent*)malloc(b->agentCount * sizeof(BoidAgent));
        if (agents == NULL) {
            b->agentBuffer = 0;
            return;
        }

        InitializeAgents(agents, b->agentCount, b->width, b->height, &b->config.color);
        b->agentBuffer = rlLoadShaderBuffer(b->agentCount * sizeof(BoidAgent), agents, RL_DYNAMIC_COPY);
        free(agents);

        TrailMapClear(b->trailMap);

        TraceLog(LOG_INFO, "BOIDS: Reallocated buffer for %d agents", b->agentCount);
    } else if (needsHueReinit) {
        BoidsReset(b);
    }
}

void BoidsReset(Boids* b)
{
    if (b == NULL) {
        return;
    }

    TrailMapClear(b->trailMap);

    BoidAgent* agents = (BoidAgent*)malloc(b->agentCount * sizeof(BoidAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, b->agentCount, b->width, b->height, &b->config.color);
    rlUpdateShaderBuffer(b->agentBuffer, agents, b->agentCount * sizeof(BoidAgent), 0);
    free(agents);
}

void BoidsResize(Boids* b, int width, int height)
{
    if (b == NULL || (width == b->width && height == b->height)) {
        return;
    }

    b->width = width;
    b->height = height;

    TrailMapResize(b->trailMap, width, height);

    BoidsReset(b);
}

bool BoidsBeginTrailMapDraw(Boids* b)
{
    if (b == NULL || !b->supported || !b->config.enabled) {
        return false;
    }
    return TrailMapBeginDraw(b->trailMap);
}

void BoidsEndTrailMapDraw(Boids* b)
{
    if (b == NULL || !b->supported || !b->config.enabled) {
        return;
    }
    TrailMapEndDraw(b->trailMap);
}
