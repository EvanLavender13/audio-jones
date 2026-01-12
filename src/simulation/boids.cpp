#include "boids.h"
#include "trail_map.h"
#include "spatial_hash.h"
#include "shader_utils.h"
#include "render/color_config.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/boids_agents.glsl";
static const int MIN_CELL_SIZE = 40;     // Minimum to prevent huge grids
static const int TARGET_CELL_SIZE = 80;  // Target max cell size for grid alignment

// GCD for calculating resolution-aligned cell size
static int Gcd(int a, int b)
{
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// Find largest divisor of GCD(width, height) in [MIN_CELL_SIZE, TARGET_CELL_SIZE]
// Falls back to MIN_CELL_SIZE if no good divisor exists (non-standard resolutions)
static float CalculateCellSize(int width, int height)
{
    const int gcd = Gcd(width, height);
    int cellSize = 0;
    for (int d = 1; d * d <= gcd; d++) {
        if (gcd % d == 0) {
            if (d >= MIN_CELL_SIZE && d <= TARGET_CELL_SIZE && d > cellSize) {
                cellSize = d;
            }
            const int other = gcd / d;
            if (other >= MIN_CELL_SIZE && other <= TARGET_CELL_SIZE && other > cellSize) {
                cellSize = other;
            }
        }
    }
    // Fallback for resolutions with no good divisors (e.g., coprime dimensions)
    if (cellSize == 0) {
        cellSize = MIN_CELL_SIZE;
    }
    return (float)cellSize;
}

static void InitializeAgents(BoidAgent* agents, int count, int width, int height, const ColorConfig* color)
{
    for (int i = 0; i < count; i++) {
        agents[i].x = (float)(GetRandomValue(0, width - 1));
        agents[i].y = (float)(GetRandomValue(0, height - 1));

        const float angle = (float)GetRandomValue(0, 628) / 100.0f;
        agents[i].vx = cosf(angle) * 1.0f;
        agents[i].vy = sinf(angle) * 1.0f;
        agents[i].hue = ColorConfigAgentHue(color, i, count);
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
    b->maxSpeedLoc = rlGetLocationUniform(program, "maxSpeed");
    b->minSpeedLoc = rlGetLocationUniform(program, "minSpeed");
    b->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
    b->saturationLoc = rlGetLocationUniform(program, "saturation");
    b->valueLoc = rlGetLocationUniform(program, "value");
    b->gridSizeLoc = rlGetLocationUniform(program, "gridSize");
    b->cellSizeLoc = rlGetLocationUniform(program, "cellSize");

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

    b->spatialHash = SpatialHashInit(b->agentCount,
        CalculateCellSize(width, height), width, height);
    if (b->spatialHash == NULL) {
        TraceLog(LOG_ERROR, "BOIDS: Failed to create spatial hash");
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
    SpatialHashUninit(b->spatialHash);
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

    // Build spatial hash from current agent positions
    SpatialHashBuild(b->spatialHash, b->agentBuffer, b->agentCount,
                     (int)sizeof(BoidAgent), 0);

    rlEnableShader(b->computeProgram);

    float resolution[2] = { (float)b->width, (float)b->height };
    rlSetUniform(b->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(b->perceptionRadiusLoc, &b->config.perceptionRadius, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->separationRadiusLoc, &b->config.separationRadius, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->cohesionWeightLoc, &b->config.cohesionWeight, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->separationWeightLoc, &b->config.separationWeight, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->alignmentWeightLoc, &b->config.alignmentWeight, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->hueAffinityLoc, &b->config.hueAffinity, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->maxSpeedLoc, &b->config.maxSpeed, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->minSpeedLoc, &b->config.minSpeed, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->depositAmountLoc, &b->config.depositAmount, RL_SHADER_UNIFORM_FLOAT, 1);

    float saturation;
    float colorValue;
    ColorConfigGetSV(&b->config.color, &saturation, &colorValue);
    rlSetUniform(b->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(b->valueLoc, &colorValue, RL_SHADER_UNIFORM_FLOAT, 1);

    int gridWidth;
    int gridHeight;
    float cellSize;
    SpatialHashGetGrid(b->spatialHash, &gridWidth, &gridHeight, &cellSize);
    int gridSize[2] = { gridWidth, gridHeight };
    rlSetUniform(b->gridSizeLoc, gridSize, RL_SHADER_UNIFORM_IVEC2, 1);
    rlSetUniform(b->cellSizeLoc, &cellSize, RL_SHADER_UNIFORM_FLOAT, 1);

    rlBindShaderBuffer(b->agentBuffer, 0);
    rlBindImageTexture(TrailMapGetTexture(b->trailMap).id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
    rlBindShaderBuffer(SpatialHashGetOffsetsBuffer(b->spatialHash), 2);
    rlBindShaderBuffer(SpatialHashGetIndicesBuffer(b->spatialHash), 3);

    const int workGroupSize = 1024;
    const int numGroups = (b->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

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

        // Recreate spatial hash for new agent count (cell size is resolution-based, doesn't change)
        SpatialHashUninit(b->spatialHash);
        b->spatialHash = SpatialHashInit(b->agentCount,
            CalculateCellSize(b->width, b->height), b->width, b->height);
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

    // Recreate spatial hash with new resolution-based cell size
    SpatialHashUninit(b->spatialHash);
    b->spatialHash = SpatialHashInit(b->agentCount,
        CalculateCellSize(width, height), width, height);

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

void BoidsDrawDebug(Boids* b)
{
    if (b == NULL || !b->supported || !b->config.enabled) {
        return;
    }

    const Texture2D trailTex = TrailMapGetTexture(b->trailMap);
    if (b->debugShader.id != 0) {
        BeginShaderMode(b->debugShader);
    }
    DrawTextureRec(trailTex,
        {0, 0, (float)b->width, (float)-b->height},
        {0, 0}, WHITE);
    if (b->debugShader.id != 0) {
        EndShaderMode();
    }
}
