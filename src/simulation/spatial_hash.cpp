#include "spatial_hash.h"
#include "shader_utils.h"
#include "raylib.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const char* SHADER_PATH = "shaders/spatial_hash_build.glsl";

static void CalculateGridDimensions(SpatialHash* sh)
{
    sh->gridWidth = (int)ceilf((float)sh->width / sh->cellSize);
    sh->gridHeight = (int)ceilf((float)sh->height / sh->cellSize);
    if (sh->gridWidth < 1) {
        sh->gridWidth = 1;
    }
    if (sh->gridHeight < 1) {
        sh->gridHeight = 1;
    }
}

static GLuint CompileKernel(const char* source, const char* define)
{
    // Find end of first line (#version) to insert define after it
    const char* firstNewline = strchr(source, '\n');
    if (firstNewline == NULL) {
        return 0;
    }

    const size_t versionLen = (size_t)(firstNewline - source + 1);
    const size_t defineLen = strlen(define);
    const size_t restLen = strlen(firstNewline + 1);
    char* fullSource = (char*)malloc(versionLen + defineLen + 2 + restLen + 1);
    if (fullSource == NULL) {
        return 0;
    }

    // Copy: #version line, define, newline, rest of shader
    memcpy(fullSource, source, versionLen);
    memcpy(fullSource + versionLen, define, defineLen);
    fullSource[versionLen + defineLen] = '\n';
    memcpy(fullSource + versionLen + defineLen + 1, firstNewline + 1, restLen + 1);

    const unsigned int shaderId = rlCompileShader(fullSource, RL_COMPUTE_SHADER);
    free(fullSource);

    if (shaderId == 0) {
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    return program;
}

static bool LoadShaderPrograms(SpatialHash* sh)
{
    char* shaderSource = SimLoadShaderSource(SHADER_PATH);
    if (shaderSource == NULL) {
        return false;
    }

    struct { GLuint* program; const char* define; const char* name; } kernels[] = {
        { &sh->clearProgram, "#define KERNEL_CLEAR", "clear" },
        { &sh->countProgram, "#define KERNEL_COUNT", "count" },
        { &sh->prefixSumProgram, "#define KERNEL_PREFIX_SUM", "prefix sum" },
        { &sh->scatterProgram, "#define KERNEL_SCATTER", "scatter" },
    };

    for (int i = 0; i < 4; i++) {
        *kernels[i].program = CompileKernel(shaderSource, kernels[i].define);
        if (*kernels[i].program == 0) {
            TraceLog(LOG_ERROR, "SPATIAL_HASH: Failed to compile %s kernel", kernels[i].name);
            UnloadFileText(shaderSource);
            return false;
        }
    }

    UnloadFileText(shaderSource);

    // Cache uniform locations - clear program
    sh->clearTotalCellsLoc = rlGetLocationUniform(sh->clearProgram, "totalCells");

    // Cache uniform locations - count program
    sh->countResolutionLoc = rlGetLocationUniform(sh->countProgram, "resolution");
    sh->countCellSizeLoc = rlGetLocationUniform(sh->countProgram, "cellSize");
    sh->countGridSizeLoc = rlGetLocationUniform(sh->countProgram, "gridSize");
    sh->countAgentCountLoc = rlGetLocationUniform(sh->countProgram, "agentCount");
    sh->countAgentStrideLoc = rlGetLocationUniform(sh->countProgram, "agentStride");
    sh->countPositionOffsetLoc = rlGetLocationUniform(sh->countProgram, "positionOffset");

    // Cache uniform locations - prefix sum program
    sh->prefixSumTotalCellsLoc = rlGetLocationUniform(sh->prefixSumProgram, "totalCells");

    // Cache uniform locations - scatter program
    sh->scatterResolutionLoc = rlGetLocationUniform(sh->scatterProgram, "resolution");
    sh->scatterCellSizeLoc = rlGetLocationUniform(sh->scatterProgram, "cellSize");
    sh->scatterGridSizeLoc = rlGetLocationUniform(sh->scatterProgram, "gridSize");
    sh->scatterAgentCountLoc = rlGetLocationUniform(sh->scatterProgram, "agentCount");
    sh->scatterAgentStrideLoc = rlGetLocationUniform(sh->scatterProgram, "agentStride");
    sh->scatterPositionOffsetLoc = rlGetLocationUniform(sh->scatterProgram, "positionOffset");

    return true;
}

static bool AllocateBuffers(SpatialHash* sh)
{
    const int totalCells = sh->gridWidth * sh->gridHeight;

    sh->cellCountsBuffer = rlLoadShaderBuffer(totalCells * sizeof(unsigned int), NULL, RL_DYNAMIC_COPY);
    if (sh->cellCountsBuffer == 0) {
        TraceLog(LOG_ERROR, "SPATIAL_HASH: Failed to create cell counts buffer");
        return false;
    }

    sh->cellOffsetsBuffer = rlLoadShaderBuffer(totalCells * sizeof(unsigned int), NULL, RL_DYNAMIC_COPY);
    if (sh->cellOffsetsBuffer == 0) {
        TraceLog(LOG_ERROR, "SPATIAL_HASH: Failed to create cell offsets buffer");
        return false;
    }

    sh->sortedIndicesBuffer = rlLoadShaderBuffer(sh->maxAgents * sizeof(unsigned int), NULL, RL_DYNAMIC_COPY);
    if (sh->sortedIndicesBuffer == 0) {
        TraceLog(LOG_ERROR, "SPATIAL_HASH: Failed to create sorted indices buffer");
        return false;
    }

    return true;
}

static void FreeBuffers(SpatialHash* sh)
{
    if (sh->cellCountsBuffer != 0) {
        rlUnloadShaderBuffer(sh->cellCountsBuffer);
        sh->cellCountsBuffer = 0;
    }
    if (sh->cellOffsetsBuffer != 0) {
        rlUnloadShaderBuffer(sh->cellOffsetsBuffer);
        sh->cellOffsetsBuffer = 0;
    }
    if (sh->sortedIndicesBuffer != 0) {
        rlUnloadShaderBuffer(sh->sortedIndicesBuffer);
        sh->sortedIndicesBuffer = 0;
    }
}

SpatialHash* SpatialHashInit(int maxAgents, float cellSize, int width, int height)
{
    SpatialHash* sh = (SpatialHash*)calloc(1, sizeof(SpatialHash));
    if (sh == NULL) {
        return NULL;
    }

    sh->maxAgents = maxAgents;
    sh->cellSize = cellSize;
    sh->width = width;
    sh->height = height;

    CalculateGridDimensions(sh);

    if (!LoadShaderPrograms(sh)) {
        goto cleanup;
    }

    if (!AllocateBuffers(sh)) {
        goto cleanup;
    }

    TraceLog(LOG_INFO, "SPATIAL_HASH: Initialized %dx%d grid (cell size %.1f) for %d agents",
             sh->gridWidth, sh->gridHeight, sh->cellSize, sh->maxAgents);
    return sh;

cleanup:
    SpatialHashUninit(sh);
    return NULL;
}

void SpatialHashUninit(SpatialHash* sh)
{
    if (sh == NULL) {
        return;
    }

    FreeBuffers(sh);

    if (sh->clearProgram != 0) {
        rlUnloadShaderProgram(sh->clearProgram);
    }
    if (sh->countProgram != 0) {
        rlUnloadShaderProgram(sh->countProgram);
    }
    if (sh->prefixSumProgram != 0) {
        rlUnloadShaderProgram(sh->prefixSumProgram);
    }
    if (sh->scatterProgram != 0) {
        rlUnloadShaderProgram(sh->scatterProgram);
    }

    free(sh);
}

void SpatialHashResize(SpatialHash* sh, int width, int height)
{
    if (sh == NULL || (width == sh->width && height == sh->height)) {
        return;
    }

    sh->width = width;
    sh->height = height;

    const int oldGridWidth = sh->gridWidth;
    const int oldGridHeight = sh->gridHeight;
    CalculateGridDimensions(sh);

    if (sh->gridWidth != oldGridWidth || sh->gridHeight != oldGridHeight) {
        FreeBuffers(sh);
        if (!AllocateBuffers(sh)) {
            TraceLog(LOG_ERROR, "SPATIAL_HASH: Failed to reallocate buffers on resize");
        } else {
            TraceLog(LOG_INFO, "SPATIAL_HASH: Resized to %dx%d grid", sh->gridWidth, sh->gridHeight);
        }
    }
}

void SpatialHashBuild(SpatialHash* sh, unsigned int positionBuffer, int agentCount, int agentStride, int positionOffset)
{
    if (sh == NULL || positionBuffer == 0 || agentCount <= 0) {
        return;
    }

    const int totalCells = sh->gridWidth * sh->gridHeight;
    const int workGroupSize = 1024;
    const int agentGroups = (agentCount + workGroupSize - 1) / workGroupSize;
    const int clearGroups = (totalCells + workGroupSize - 1) / workGroupSize;

    float resolution[2] = { (float)sh->width, (float)sh->height };
    int gridSize[2] = { sh->gridWidth, sh->gridHeight };

    // Pass 1: Clear cell counts
    rlEnableShader(sh->clearProgram);
    rlSetUniform(sh->clearTotalCellsLoc, &totalCells, RL_SHADER_UNIFORM_INT, 1);
    rlBindShaderBuffer(sh->cellCountsBuffer, 0);
    rlComputeShaderDispatch((unsigned int)clearGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Pass 2: Count agents per cell
    rlEnableShader(sh->countProgram);
    rlSetUniform(sh->countResolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(sh->countCellSizeLoc, &sh->cellSize, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(sh->countGridSizeLoc, gridSize, RL_SHADER_UNIFORM_IVEC2, 1);
    rlSetUniform(sh->countAgentCountLoc, &agentCount, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(sh->countAgentStrideLoc, &agentStride, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(sh->countPositionOffsetLoc, &positionOffset, RL_SHADER_UNIFORM_INT, 1);
    rlBindShaderBuffer(positionBuffer, 0);
    rlBindShaderBuffer(sh->cellCountsBuffer, 1);
    rlComputeShaderDispatch((unsigned int)agentGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Pass 3: Serial prefix sum (single thread)
    rlEnableShader(sh->prefixSumProgram);
    rlSetUniform(sh->prefixSumTotalCellsLoc, &totalCells, RL_SHADER_UNIFORM_INT, 1);
    rlBindShaderBuffer(sh->cellCountsBuffer, 0);
    rlBindShaderBuffer(sh->cellOffsetsBuffer, 1);
    rlComputeShaderDispatch(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Pass 4: Scatter agents to sorted indices
    rlEnableShader(sh->scatterProgram);
    rlSetUniform(sh->scatterResolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(sh->scatterCellSizeLoc, &sh->cellSize, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(sh->scatterGridSizeLoc, gridSize, RL_SHADER_UNIFORM_IVEC2, 1);
    rlSetUniform(sh->scatterAgentCountLoc, &agentCount, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(sh->scatterAgentStrideLoc, &agentStride, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(sh->scatterPositionOffsetLoc, &positionOffset, RL_SHADER_UNIFORM_INT, 1);
    rlBindShaderBuffer(positionBuffer, 0);
    rlBindShaderBuffer(sh->cellOffsetsBuffer, 1);
    rlBindShaderBuffer(sh->sortedIndicesBuffer, 2);
    rlComputeShaderDispatch((unsigned int)agentGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Pass 5: Re-run prefix sum to restore offsets (scatter corrupts them via atomic decrement)
    rlEnableShader(sh->prefixSumProgram);
    rlSetUniform(sh->prefixSumTotalCellsLoc, &totalCells, RL_SHADER_UNIFORM_INT, 1);
    rlBindShaderBuffer(sh->cellCountsBuffer, 0);
    rlBindShaderBuffer(sh->cellOffsetsBuffer, 1);
    rlComputeShaderDispatch(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    rlDisableShader();
}

void SpatialHashGetGrid(const SpatialHash* sh, int* outWidth, int* outHeight, float* outCellSize)
{
    if (sh == NULL) {
        return;
    }
    if (outWidth != NULL) {
        *outWidth = sh->gridWidth;
    }
    if (outHeight != NULL) {
        *outHeight = sh->gridHeight;
    }
    if (outCellSize != NULL) {
        *outCellSize = sh->cellSize;
    }
}

unsigned int SpatialHashGetOffsetsBuffer(const SpatialHash* sh)
{
    if (sh == NULL) {
        return 0;
    }
    return sh->cellOffsetsBuffer;
}

unsigned int SpatialHashGetIndicesBuffer(const SpatialHash* sh)
{
    if (sh == NULL) {
        return 0;
    }
    return sh->sortedIndicesBuffer;
}
