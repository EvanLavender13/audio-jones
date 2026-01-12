#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

typedef struct SpatialHash {
    unsigned int cellCountsBuffer;   // Boids per cell (reset each frame)
    unsigned int cellOffsetsBuffer;  // Prefix sum result (also used as insertion counters)
    unsigned int sortedIndicesBuffer; // Agent indices sorted by cell

    unsigned int clearProgram;       // Clear counts kernel
    unsigned int countProgram;       // Count agents per cell kernel
    unsigned int prefixSumProgram;   // Serial prefix sum kernel
    unsigned int scatterProgram;     // Scatter agents to sorted indices kernel

    // Uniform locations - clear program
    int clearTotalCellsLoc;

    // Uniform locations - count program
    int countResolutionLoc;
    int countCellSizeLoc;
    int countGridSizeLoc;
    int countAgentCountLoc;
    int countAgentStrideLoc;
    int countPositionOffsetLoc;

    // Uniform locations - prefix sum program
    int prefixSumTotalCellsLoc;

    // Uniform locations - scatter program
    int scatterResolutionLoc;
    int scatterCellSizeLoc;
    int scatterGridSizeLoc;
    int scatterAgentCountLoc;
    int scatterAgentStrideLoc;
    int scatterPositionOffsetLoc;

    float cellSize;
    int gridWidth;
    int gridHeight;
    int maxAgents;
    int width;
    int height;
} SpatialHash;

// Initialize spatial hash with given parameters. Returns NULL on failure.
SpatialHash* SpatialHashInit(int maxAgents, float cellSize, int width, int height);

// Release all spatial hash resources.
void SpatialHashUninit(SpatialHash* sh);

// Recreate grid for new dimensions.
void SpatialHashResize(SpatialHash* sh, int width, int height);

// Build spatial hash from agent positions.
// positionBuffer: SSBO containing agent data
// agentCount: number of agents to process
// agentStride: bytes between agents in buffer
// positionOffset: byte offset to position (vec2) within agent struct
void SpatialHashBuild(SpatialHash* sh, unsigned int positionBuffer, int agentCount, int agentStride, int positionOffset);

// Get grid dimensions and cell size.
void SpatialHashGetGrid(const SpatialHash* sh, int* outWidth, int* outHeight, float* outCellSize);

// Get the cell offsets buffer (for binding in steering shader).
unsigned int SpatialHashGetOffsetsBuffer(const SpatialHash* sh);

// Get the sorted indices buffer (for binding in steering shader).
unsigned int SpatialHashGetIndicesBuffer(const SpatialHash* sh);

#endif // SPATIAL_HASH_H
