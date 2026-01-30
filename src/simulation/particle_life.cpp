#include "particle_life.h"
#include "trail_map.h"
#include "shader_utils.h"
#include "render/color_config.h"
#include "render/gradient.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/particle_life_agents.glsl";
static const int MAX_SPECIES = 16;

// Simple hash function for attraction matrix generation
static unsigned int HashSeed(unsigned int x)
{
    x ^= x >> 16;
    x *= 0x85ebca6bu;
    x ^= x >> 13;
    x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}

static float HashFloat(unsigned int x)
{
    return (float)HashSeed(x) / 4294967295.0f;
}

// Generate attraction matrix from seed, optionally enforce symmetry
static void GenerateAttractionMatrix(float* matrix, int speciesCount, int seed, bool symmetric)
{
    for (int from = 0; from < speciesCount; from++) {
        for (int to = 0; to < speciesCount; to++) {
            const unsigned int h = HashSeed((unsigned int)(seed * MAX_SPECIES + from * MAX_SPECIES + to));
            const float v = HashFloat(h);
            matrix[from * MAX_SPECIES + to] = v * 2.0f - 1.0f;  // Map [0,1] to [-1,1]
        }
    }
    // Enforce symmetry: matrix[A][B] == matrix[B][A]
    if (symmetric) {
        for (int from = 0; from < speciesCount; from++) {
            for (int to = from + 1; to < speciesCount; to++) {
                matrix[to * MAX_SPECIES + from] = matrix[from * MAX_SPECIES + to];
            }
        }
    }
}

// Regenerate matrix from seed into ParticleLife's stored array
static void RegenerateMatrix(ParticleLife* pl)
{
    GenerateAttractionMatrix(pl->attractionMatrix, pl->config.speciesCount,
                             pl->config.attractionSeed, pl->config.symmetricForces);
    pl->lastSeed = pl->config.attractionSeed;
    pl->evolutionFrameCounter = 0;
}

static void InitializeAgents(ParticleLifeAgent* agents, int count, int speciesCount, const ColorConfig* color)
{
    // Distribute agents in a sphere around the origin (normalized space)
    const float spawnRadius = 0.5f;

    for (int i = 0; i < count; i++) {
        // Random position in sphere
        const float theta = (float)(GetRandomValue(0, 31415)) / 10000.0f * 2.0f;  // 0 to 2*PI
        const float phi = acosf(1.0f - 2.0f * (float)GetRandomValue(0, 10000) / 10000.0f);  // Uniform on sphere
        const float r = spawnRadius * cbrtf((float)GetRandomValue(0, 10000) / 10000.0f);  // Uniform in volume

        agents[i].x = r * sinf(phi) * cosf(theta);
        agents[i].y = r * sinf(phi) * sinf(theta);
        agents[i].z = r * cosf(phi);

        // Zero initial velocity
        agents[i].vx = 0.0f;
        agents[i].vy = 0.0f;
        agents[i].vz = 0.0f;

        // Assign species evenly, derive hue from species
        agents[i].species = i % speciesCount;
        agents[i].hue = ColorConfigAgentHue(color, agents[i].species, speciesCount);
    }
}

bool ParticleLifeSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

static GLuint LoadComputeProgram(ParticleLife* pl)
{
    char* shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "PARTICLE_LIFE: Failed to compile compute shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "PARTICLE_LIFE: Failed to load compute shader program");
        return 0;
    }

    pl->resolutionLoc = rlGetLocationUniform(program, "resolution");
    pl->timeLoc = rlGetLocationUniform(program, "time");
    pl->numParticlesLoc = rlGetLocationUniform(program, "numParticles");
    pl->numSpeciesLoc = rlGetLocationUniform(program, "numSpecies");
    pl->rMaxLoc = rlGetLocationUniform(program, "rMax");
    pl->forceFactorLoc = rlGetLocationUniform(program, "forceFactor");
    pl->momentumLoc = rlGetLocationUniform(program, "momentum");
    pl->betaLoc = rlGetLocationUniform(program, "beta");
    pl->boundsRadiusLoc = rlGetLocationUniform(program, "boundsRadius");
    pl->boundaryStiffnessLoc = rlGetLocationUniform(program, "boundaryStiffness");
    pl->timeStepLoc = rlGetLocationUniform(program, "timeStep");
    pl->centerLoc = rlGetLocationUniform(program, "center");
    pl->rotationMatrixLoc = rlGetLocationUniform(program, "rotationMatrix");
    pl->projectionScaleLoc = rlGetLocationUniform(program, "projectionScale");
    pl->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
    pl->saturationLoc = rlGetLocationUniform(program, "saturation");
    pl->valueLoc = rlGetLocationUniform(program, "value");
    pl->attractionMatrixLoc = rlGetLocationUniform(program, "attractionMatrix");

    return program;
}

static GLuint CreateAgentBuffer(int agentCount, int speciesCount, const ColorConfig* color)
{
    ParticleLifeAgent* agents = (ParticleLifeAgent*)malloc(agentCount * sizeof(ParticleLifeAgent));
    if (agents == NULL) {
        return 0;
    }

    InitializeAgents(agents, agentCount, speciesCount, color);
    const GLuint buffer = rlLoadShaderBuffer(agentCount * sizeof(ParticleLifeAgent), agents, RL_DYNAMIC_COPY);
    free(agents);

    if (buffer == 0) {
        TraceLog(LOG_ERROR, "PARTICLE_LIFE: Failed to create agent SSBO");
    }
    return buffer;
}

ParticleLife* ParticleLifeInit(int width, int height, const ParticleLifeConfig* config)
{
    if (!ParticleLifeSupported()) {
        TraceLog(LOG_WARNING, "PARTICLE_LIFE: Compute shaders not supported (requires OpenGL 4.3)");
        return NULL;
    }

    ParticleLife* pl = (ParticleLife*)calloc(1, sizeof(ParticleLife));
    if (pl == NULL) {
        return NULL;
    }

    pl->width = width;
    pl->height = height;
    pl->config = (config != NULL) ? *config : ParticleLifeConfig{};
    pl->agentCount = pl->config.agentCount;
    if (pl->agentCount < 1) {
        pl->agentCount = 1;
    }
    pl->time = 0.0f;
    pl->rotationAccumX = 0.0f;
    pl->rotationAccumY = 0.0f;
    pl->rotationAccumZ = 0.0f;
    pl->evolutionFrameCounter = 0;
    pl->supported = true;

    // Initialize persistent attraction matrix
    RegenerateMatrix(pl);

    pl->computeProgram = LoadComputeProgram(pl);
    if (pl->computeProgram == 0) {
        goto cleanup;
    }

    pl->trailMap = TrailMapInit(width, height);
    if (pl->trailMap == NULL) {
        TraceLog(LOG_ERROR, "PARTICLE_LIFE: Failed to create trail map");
        goto cleanup;
    }

    pl->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
    if (pl->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "PARTICLE_LIFE: Failed to load debug shader, using default");
    }

    pl->agentBuffer = CreateAgentBuffer(pl->agentCount, pl->config.speciesCount, &pl->config.color);
    if (pl->agentBuffer == 0) {
        goto cleanup;
    }

    TraceLog(LOG_INFO, "PARTICLE_LIFE: Initialized with %d agents (%d species) at %dx%d",
             pl->agentCount, pl->config.speciesCount, width, height);
    return pl;

cleanup:
    ParticleLifeUninit(pl);
    return NULL;
}

void ParticleLifeUninit(ParticleLife* pl)
{
    if (pl == NULL) {
        return;
    }

    rlUnloadShaderBuffer(pl->agentBuffer);
    TrailMapUninit(pl->trailMap);
    if (pl->debugShader.id != 0) {
        UnloadShader(pl->debugShader);
    }
    rlUnloadShaderProgram(pl->computeProgram);
    free(pl);
}

void ParticleLifeUpdate(ParticleLife* pl, float deltaTime)
{
    if (pl == NULL || !pl->supported || !pl->config.enabled) {
        return;
    }

    pl->time += deltaTime;

    // Evolve matrix values via random walk when evolution enabled
    if (pl->config.evolutionSpeed > 0.0f) {
        for (int from = 0; from < pl->config.speciesCount; from++) {
            for (int to = 0; to < pl->config.speciesCount; to++) {
                const int idx = from * MAX_SPECIES + to;
                const unsigned int h = HashSeed(pl->evolutionFrameCounter * 256 + idx);
                const float noise = (HashFloat(h) - 0.5f) * 2.0f;
                pl->attractionMatrix[idx] += noise * pl->config.evolutionSpeed * deltaTime;
                pl->attractionMatrix[idx] = fmaxf(-1.0f, fminf(1.0f, pl->attractionMatrix[idx]));
            }
        }
        // Enforce symmetry after evolution if enabled
        if (pl->config.symmetricForces) {
            for (int from = 0; from < pl->config.speciesCount; from++) {
                for (int to = from + 1; to < pl->config.speciesCount; to++) {
                    pl->attractionMatrix[to * MAX_SPECIES + from] = pl->attractionMatrix[from * MAX_SPECIES + to];
                }
            }
        }
        pl->evolutionFrameCounter++;
    }

    // Accumulate rotation speeds
    pl->rotationAccumX += pl->config.rotationSpeedX * deltaTime;
    pl->rotationAccumY += pl->config.rotationSpeedY * deltaTime;
    pl->rotationAccumZ += pl->config.rotationSpeedZ * deltaTime;

    rlEnableShader(pl->computeProgram);

    float resolution[2] = { (float)pl->width, (float)pl->height };
    rlSetUniform(pl->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(pl->timeLoc, &pl->time, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->numParticlesLoc, &pl->agentCount, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(pl->numSpeciesLoc, &pl->config.speciesCount, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(pl->rMaxLoc, &pl->config.rMax, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->forceFactorLoc, &pl->config.forceFactor, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->momentumLoc, &pl->config.momentum, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->betaLoc, &pl->config.beta, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->boundsRadiusLoc, &pl->config.boundsRadius, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->boundaryStiffnessLoc, &pl->config.boundaryStiffness, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->timeStepLoc, &deltaTime, RL_SHADER_UNIFORM_FLOAT, 1);

    float center[2] = { pl->config.x, pl->config.y };
    rlSetUniform(pl->centerLoc, center, RL_SHADER_UNIFORM_VEC2, 1);

    // Compute rotation matrix on CPU (XYZ order)
    const float rotX = pl->config.rotationAngleX + pl->rotationAccumX;
    const float rotY = pl->config.rotationAngleY + pl->rotationAccumY;
    const float rotZ = pl->config.rotationAngleZ + pl->rotationAccumZ;

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
    glUniformMatrix3fv(pl->rotationMatrixLoc, 1, GL_FALSE, rotationMatrix);

    rlSetUniform(pl->projectionScaleLoc, &pl->config.projectionScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->depositAmountLoc, &pl->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);

    float saturation;
    float value;
    ColorConfigGetSV(&pl->config.color, &saturation, &value);
    rlSetUniform(pl->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(pl->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    // Upload stored attraction matrix
    glUniform1fv(pl->attractionMatrixLoc, MAX_SPECIES * MAX_SPECIES, pl->attractionMatrix);

    rlBindShaderBuffer(pl->agentBuffer, 0);
    rlBindImageTexture(TrailMapGetTexture(pl->trailMap).id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);

    const int workGroupSize = 1024;
    const int numGroups = (pl->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void ParticleLifeProcessTrails(ParticleLife* pl, float deltaTime)
{
    if (pl == NULL || !pl->supported || !pl->config.enabled) {
        return;
    }

    TrailMapProcess(pl->trailMap, deltaTime, pl->config.decayHalfLife, pl->config.diffusionScale);
}

void ParticleLifeResize(ParticleLife* pl, int width, int height)
{
    if (pl == NULL || (width == pl->width && height == pl->height)) {
        return;
    }

    pl->width = width;
    pl->height = height;

    TrailMapResize(pl->trailMap, width, height);

    ParticleLifeReset(pl);
}

void ParticleLifeReset(ParticleLife* pl)
{
    if (pl == NULL) {
        return;
    }

    TrailMapClear(pl->trailMap);

    ParticleLifeAgent* agents = (ParticleLifeAgent*)malloc(pl->agentCount * sizeof(ParticleLifeAgent));
    if (agents == NULL) {
        return;
    }

    InitializeAgents(agents, pl->agentCount, pl->config.speciesCount, &pl->config.color);
    rlUpdateShaderBuffer(pl->agentBuffer, agents, pl->agentCount * sizeof(ParticleLifeAgent), 0);
    free(agents);
}

void ParticleLifeApplyConfig(ParticleLife* pl, const ParticleLifeConfig* newConfig)
{
    if (pl == NULL || newConfig == NULL) {
        return;
    }

    int newAgentCount = newConfig->agentCount;
    if (newAgentCount < 1) {
        newAgentCount = 1;
    }

    const bool needsBufferRealloc = (newAgentCount != pl->agentCount);
    const bool colorChanged = !ColorConfigEquals(&pl->config.color, &newConfig->color);
    const bool speciesChanged = (newConfig->speciesCount != pl->config.speciesCount);
    const bool seedChanged = (newConfig->attractionSeed != pl->config.attractionSeed);
    const bool symmetryChanged = (newConfig->symmetricForces != pl->config.symmetricForces);

    pl->config = *newConfig;

    // Regenerate matrix if seed or symmetry setting changed
    if (seedChanged || symmetryChanged || speciesChanged) {
        RegenerateMatrix(pl);
    }

    if (needsBufferRealloc || speciesChanged) {
        rlUnloadShaderBuffer(pl->agentBuffer);
        pl->agentCount = newAgentCount;

        ParticleLifeAgent* agents = (ParticleLifeAgent*)malloc(pl->agentCount * sizeof(ParticleLifeAgent));
        if (agents == NULL) {
            pl->agentBuffer = 0;
            return;
        }

        InitializeAgents(agents, pl->agentCount, pl->config.speciesCount, &pl->config.color);
        pl->agentBuffer = rlLoadShaderBuffer(pl->agentCount * sizeof(ParticleLifeAgent), agents, RL_DYNAMIC_COPY);
        free(agents);

        TrailMapClear(pl->trailMap);

        TraceLog(LOG_INFO, "PARTICLE_LIFE: Reallocated buffer for %d agents (%d species)",
                 pl->agentCount, pl->config.speciesCount);
    } else if (colorChanged) {
        ParticleLifeAgent* agents = (ParticleLifeAgent*)malloc(pl->agentCount * sizeof(ParticleLifeAgent));
        if (agents != NULL) {
            InitializeAgents(agents, pl->agentCount, pl->config.speciesCount, &pl->config.color);
            rlUpdateShaderBuffer(pl->agentBuffer, agents, pl->agentCount * sizeof(ParticleLifeAgent), 0);
            free(agents);
            TrailMapClear(pl->trailMap);
        }
    }
}

void ParticleLifeDrawDebug(ParticleLife* pl)
{
    if (pl == NULL || !pl->supported || !pl->config.enabled) {
        return;
    }

    const Texture2D trailTex = TrailMapGetTexture(pl->trailMap);
    if (pl->debugShader.id != 0) {
        BeginShaderMode(pl->debugShader);
    }
    DrawTextureRec(trailTex,
        {0, 0, (float)pl->width, (float)-pl->height},
        {0, 0}, WHITE);
    if (pl->debugShader.id != 0) {
        EndShaderMode();
    }
}
