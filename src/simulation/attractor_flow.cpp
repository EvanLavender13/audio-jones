#include "attractor_flow.h"
#include "trail_map.h"
#include "shader_utils.h"
#include "render/color_config.h"
#include "render/gradient.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/attractor_agents.glsl";

static void InitializeAgents(AttractorAgent* agents, int count, AttractorType type, const ColorConfig* color)
{
    for (int i = 0; i < count; i++) {
        switch (type) {
            case ATTRACTOR_LORENZ: {
                const float wing = (GetRandomValue(0, 1) == 0) ? 1.0f : -1.0f;
                agents[i].x = wing * 8.5f + (float)(GetRandomValue(-250, 250)) / 100.0f;
                agents[i].y = wing * 8.5f + (float)(GetRandomValue(-250, 250)) / 100.0f;
                agents[i].z = 27.0f + (float)(GetRandomValue(-500, 500)) / 100.0f;
                break;
            }
            case ATTRACTOR_ROSSLER: {
                agents[i].x = (float)(GetRandomValue(-200, 200)) / 100.0f;
                agents[i].y = (float)(GetRandomValue(-200, 200)) / 100.0f;
                agents[i].z = (float)(GetRandomValue(-100, 100)) / 100.0f;
                break;
            }
            case ATTRACTOR_AIZAWA: {
                agents[i].x = (float)(GetRandomValue(-50, 50)) / 100.0f;
                agents[i].y = (float)(GetRandomValue(-50, 50)) / 100.0f;
                agents[i].z = (float)(GetRandomValue(-50, 50)) / 100.0f;
                break;
            }
            case ATTRACTOR_THOMAS:
            default: {
                agents[i].x = (float)(GetRandomValue(-100, 100)) / 100.0f;
                agents[i].y = (float)(GetRandomValue(-100, 100)) / 100.0f;
                agents[i].z = (float)(GetRandomValue(-100, 100)) / 100.0f;
                break;
            }
        }

        float hue;
        if (color->mode == COLOR_MODE_SOLID) {
            float s;
            float v;
            ColorConfigRGBToHSV(color->solid, &hue, &s, &v);
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
        agents[i].age = 0.0f;
        agents[i]._pad[0] = 0.0f;
        agents[i]._pad[1] = 0.0f;
        agents[i]._pad[2] = 0.0f;
    }
}

bool AttractorFlowSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

static GLuint LoadComputeProgram(AttractorFlow* af)
{
    char* shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "ATTRACTOR_FLOW: Failed to compile compute shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "ATTRACTOR_FLOW: Failed to load compute shader program");
        return 0;
    }

    af->resolutionLoc = rlGetLocationUniform(program, "resolution");
    af->timeLoc = rlGetLocationUniform(program, "time");
    af->attractorTypeLoc = rlGetLocationUniform(program, "attractorType");
    af->timeScaleLoc = rlGetLocationUniform(program, "timeScale");
    af->attractorScaleLoc = rlGetLocationUniform(program, "attractorScale");
    af->sigmaLoc = rlGetLocationUniform(program, "sigma");
    af->rhoLoc = rlGetLocationUniform(program, "rho");
    af->betaLoc = rlGetLocationUniform(program, "beta");
    af->rosslerCLoc = rlGetLocationUniform(program, "rosslerC");
    af->thomasBLoc = rlGetLocationUniform(program, "thomasB");
    af->centerLoc = rlGetLocationUniform(program, "center");
    af->rotationMatrixLoc = rlGetLocationUniform(program, "rotationMatrix");
    af->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
    af->saturationLoc = rlGetLocationUniform(program, "saturation");
    af->valueLoc = rlGetLocationUniform(program, "value");

    return program;
}

static GLuint CreateAgentBuffer(int agentCount, AttractorType type, const ColorConfig* color)
{
    AttractorAgent* agents = (AttractorAgent*)malloc(agentCount * sizeof(AttractorAgent));
    if (agents == NULL) {
        return 0;
    }

    InitializeAgents(agents, agentCount, type, color);
    const GLuint buffer = rlLoadShaderBuffer(agentCount * sizeof(AttractorAgent), agents, RL_DYNAMIC_COPY);
    free(agents);

    if (buffer == 0) {
        TraceLog(LOG_ERROR, "ATTRACTOR_FLOW: Failed to create agent SSBO");
    }
    return buffer;
}

AttractorFlow* AttractorFlowInit(int width, int height, const AttractorFlowConfig* config)
{
    if (!AttractorFlowSupported()) {
        TraceLog(LOG_WARNING, "ATTRACTOR_FLOW: Compute shaders not supported (requires OpenGL 4.3)");
        return NULL;
    }

    AttractorFlow* af = (AttractorFlow*)calloc(1, sizeof(AttractorFlow));
    if (af == NULL) {
        return NULL;
    }

    af->width = width;
    af->height = height;
    af->config = (config != NULL) ? *config : AttractorFlowConfig{};
    af->agentCount = af->config.agentCount;
    if (af->agentCount < 1) {
        af->agentCount = 1;
    }
    af->time = 0.0f;
    af->supported = true;

    af->computeProgram = LoadComputeProgram(af);
    if (af->computeProgram == 0) {
        goto cleanup;
    }

    af->trailMap = TrailMapInit(width, height);
    if (af->trailMap == NULL) {
        TraceLog(LOG_ERROR, "ATTRACTOR_FLOW: Failed to create trail map");
        goto cleanup;
    }

    af->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
    if (af->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "ATTRACTOR_FLOW: Failed to load debug shader, using default");
    }

    af->agentBuffer = CreateAgentBuffer(af->agentCount, af->config.attractorType, &af->config.color);
    if (af->agentBuffer == 0) {
        goto cleanup;
    }

    TraceLog(LOG_INFO, "ATTRACTOR_FLOW: Initialized with %d agents at %dx%d", af->agentCount, width, height);
    return af;

cleanup:
    AttractorFlowUninit(af);
    return NULL;
}

void AttractorFlowUninit(AttractorFlow* af)
{
    if (af == NULL) {
        return;
    }

    rlUnloadShaderBuffer(af->agentBuffer);
    TrailMapUninit(af->trailMap);
    if (af->debugShader.id != 0) {
        UnloadShader(af->debugShader);
    }
    rlUnloadShaderProgram(af->computeProgram);
    free(af);
}

void AttractorFlowUpdate(AttractorFlow* af, float deltaTime)
{
    if (af == NULL || !af->supported || !af->config.enabled) {
        return;
    }

    af->time += deltaTime;

    // Accumulate rotation speeds into runtime accumulators (not saved to preset)
    af->rotationAccumX += af->config.rotationSpeedX;
    af->rotationAccumY += af->config.rotationSpeedY;
    af->rotationAccumZ += af->config.rotationSpeedZ;

    rlEnableShader(af->computeProgram);

    float resolution[2] = { (float)af->width, (float)af->height };
    rlSetUniform(af->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(af->timeLoc, &af->time, RL_SHADER_UNIFORM_FLOAT, 1);
    int attractorType = (int)af->config.attractorType;
    rlSetUniform(af->attractorTypeLoc, &attractorType, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(af->timeScaleLoc, &af->config.timeScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(af->attractorScaleLoc, &af->config.attractorScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(af->sigmaLoc, &af->config.sigma, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(af->rhoLoc, &af->config.rho, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(af->betaLoc, &af->config.beta, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(af->rosslerCLoc, &af->config.rosslerC, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(af->thomasBLoc, &af->config.thomasB, RL_SHADER_UNIFORM_FLOAT, 1);
    float center[2] = { af->config.x, af->config.y };
    rlSetUniform(af->centerLoc, center, RL_SHADER_UNIFORM_VEC2, 1);

    // Effective rotation = base angle + accumulated speed
    const float rotX = af->config.rotationX + af->rotationAccumX;
    const float rotY = af->config.rotationY + af->rotationAccumY;
    const float rotZ = af->config.rotationZ + af->rotationAccumZ;

    const float cx = cosf(rotX);
    const float sx = sinf(rotX);
    const float cy = cosf(rotY);
    const float sy = sinf(rotY);
    const float cz = cosf(rotZ);
    const float sz = sinf(rotZ);

    // Rotation matrix (XYZ order): Rz * Ry * Rx, column-major for OpenGL
    float rotationMatrix[9] = {
        cy * cz,                    cy * sz,                    -sy,
        sx * sy * cz - cx * sz,     sx * sy * sz + cx * cz,     sx * cy,
        cx * sy * cz + sx * sz,     cx * sy * sz - sx * cz,     cx * cy
    };
    glUniformMatrix3fv(af->rotationMatrixLoc, 1, GL_FALSE, rotationMatrix);

    rlSetUniform(af->depositAmountLoc, &af->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);

    float saturation;
    float value;
    if (af->config.color.mode == COLOR_MODE_SOLID) {
        float h;
        ColorConfigRGBToHSV(af->config.color.solid, &h, &saturation, &value);
    } else {
        saturation = af->config.color.rainbowSat;
        value = af->config.color.rainbowVal;
    }
    rlSetUniform(af->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(af->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(af->agentBuffer, 0);
    rlBindImageTexture(TrailMapGetTexture(af->trailMap).id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);

    const int workGroupSize = 1024;
    const int numGroups = (af->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void AttractorFlowProcessTrails(AttractorFlow* af, float deltaTime)
{
    if (af == NULL || !af->supported || !af->config.enabled) {
        return;
    }

    TrailMapProcess(af->trailMap, deltaTime, af->config.decayHalfLife, af->config.diffusionScale);
}

void AttractorFlowResize(AttractorFlow* af, int width, int height)
{
    if (af == NULL || (width == af->width && height == af->height)) {
        return;
    }

    af->width = width;
    af->height = height;

    TrailMapResize(af->trailMap, width, height);

    AttractorFlowReset(af);
}

void AttractorFlowReset(AttractorFlow* af)
{
    if (af == NULL) {
        return;
    }

    TrailMapClear(af->trailMap);

    AttractorAgent* agents = (AttractorAgent*)malloc(af->agentCount * sizeof(AttractorAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, af->agentCount, af->config.attractorType, &af->config.color);
    rlUpdateShaderBuffer(af->agentBuffer, agents, af->agentCount * sizeof(AttractorAgent), 0);
    free(agents);
}

void AttractorFlowApplyConfig(AttractorFlow* af, const AttractorFlowConfig* newConfig)
{
    if (af == NULL || newConfig == NULL) {
        return;
    }

    int newAgentCount = newConfig->agentCount;
    if (newAgentCount < 1) {
        newAgentCount = 1;
    }

    const bool needsBufferRealloc = (newAgentCount != af->agentCount);
    const bool colorChanged = !ColorConfigEquals(&af->config.color, &newConfig->color);

    af->config = *newConfig;

    if (needsBufferRealloc) {
        rlUnloadShaderBuffer(af->agentBuffer);
        af->agentCount = newAgentCount;

        AttractorAgent* agents = (AttractorAgent*)malloc(af->agentCount * sizeof(AttractorAgent));
        if (agents == NULL) {
            af->agentBuffer = 0;
            return;
        }

        InitializeAgents(agents, af->agentCount, af->config.attractorType, &af->config.color);
        af->agentBuffer = rlLoadShaderBuffer(af->agentCount * sizeof(AttractorAgent), agents, RL_DYNAMIC_COPY);
        free(agents);

        TrailMapClear(af->trailMap);

        TraceLog(LOG_INFO, "ATTRACTOR_FLOW: Reallocated buffer for %d agents", af->agentCount);
    } else if (colorChanged) {
        AttractorAgent* agents = (AttractorAgent*)malloc(af->agentCount * sizeof(AttractorAgent));
        if (agents != NULL) {
            InitializeAgents(agents, af->agentCount, af->config.attractorType, &af->config.color);
            rlUpdateShaderBuffer(af->agentBuffer, agents, af->agentCount * sizeof(AttractorAgent), 0);
            free(agents);
            TrailMapClear(af->trailMap);
        }
    }
}

void AttractorFlowDrawDebug(AttractorFlow* af)
{
    if (af == NULL || !af->supported || !af->config.enabled) {
        return;
    }

    const Texture2D trailTex = TrailMapGetTexture(af->trailMap);
    if (af->debugShader.id != 0) {
        BeginShaderMode(af->debugShader);
    }
    DrawTextureRec(trailTex,
        {0, 0, (float)af->width, (float)-af->height},
        {0, 0}, WHITE);
    if (af->debugShader.id != 0) {
        EndShaderMode();
    }
}

bool AttractorFlowBeginTrailMapDraw(AttractorFlow* af)
{
    if (af == NULL || !af->supported || !af->config.enabled) {
        return false;
    }
    return TrailMapBeginDraw(af->trailMap);
}

void AttractorFlowEndTrailMapDraw(AttractorFlow* af)
{
    if (af == NULL || !af->supported || !af->config.enabled) {
        return;
    }
    TrailMapEndDraw(af->trailMap);
}
