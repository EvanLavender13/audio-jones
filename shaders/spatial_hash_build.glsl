#version 430

// Spatial hash build shader with four kernels:
// - KERNEL_CLEAR: Zero out cell counts
// - KERNEL_COUNT: Count agents per cell
// - KERNEL_PREFIX_SUM: Serial prefix sum (single thread)
// - KERNEL_SCATTER: Scatter agents to sorted indices

// Common uniforms for kernels using position-to-cell mapping
#if defined(KERNEL_COUNT) || defined(KERNEL_SCATTER)
uniform vec2 resolution;
uniform float cellSize;
uniform ivec2 gridSize;

// Position to cell index (mod ensures pos in [0, resolution), so cellCoord is always valid)
int positionToCell(vec2 pos)
{
    pos = mod(pos, resolution);
    ivec2 cellCoord = ivec2(floor(pos / cellSize));
    return cellCoord.y * gridSize.x + cellCoord.x;
}
#endif

#ifdef KERNEL_CLEAR

layout(local_size_x = 1024) in;

layout(std430, binding = 0) buffer CellCounts {
    uint cellCounts[];
};

uniform int totalCells;

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= totalCells) {
        return;
    }
    cellCounts[id] = 0u;
}

#endif // KERNEL_CLEAR

#ifdef KERNEL_COUNT

layout(local_size_x = 1024) in;

layout(std430, binding = 0) buffer Positions {
    float positionData[];
};

layout(std430, binding = 1) buffer CellCounts {
    uint cellCounts[];
};

uniform int agentCount;
uniform int agentStride;    // Bytes between agents
uniform int positionOffset; // Byte offset to position within agent struct

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= agentCount) {
        return;
    }

    // Calculate float offset (agentStride and positionOffset are in bytes, floats are 4 bytes)
    int floatStride = agentStride / 4;
    int floatOffset = positionOffset / 4;
    int baseIndex = int(id) * floatStride + floatOffset;

    vec2 pos = vec2(positionData[baseIndex], positionData[baseIndex + 1]);
    int cell = positionToCell(pos);
    atomicAdd(cellCounts[cell], 1u);
}

#endif // KERNEL_COUNT

#ifdef KERNEL_PREFIX_SUM

// Serial prefix sum for small grids (<10k cells)
layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer CellCounts {
    uint cellCounts[];
};

layout(std430, binding = 1) buffer CellOffsets {
    uint cellOffsets[];
};

uniform int totalCells;

void main()
{
    uint sum = 0u;
    for (int i = 0; i < totalCells; i++) {
        sum += cellCounts[i];
        cellOffsets[i] = sum;
    }
}

#endif // KERNEL_PREFIX_SUM

#ifdef KERNEL_SCATTER

layout(local_size_x = 1024) in;

layout(std430, binding = 0) buffer Positions {
    float positionData[];
};

layout(std430, binding = 1) buffer CellOffsets {
    uint cellOffsets[];
};

layout(std430, binding = 2) buffer SortedIndices {
    uint sortedIndices[];
};

uniform int agentCount;
uniform int agentStride;
uniform int positionOffset;

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= agentCount) {
        return;
    }

    int floatStride = agentStride / 4;
    int floatOffset = positionOffset / 4;
    int baseIndex = int(id) * floatStride + floatOffset;

    vec2 pos = vec2(positionData[baseIndex], positionData[baseIndex + 1]);
    int cell = positionToCell(pos);

    // Get slot via atomic decrement on offset (offsets point to end of each cell's range)
    // We decrement to get slots from back to front
    uint slot = atomicAdd(cellOffsets[cell], 0xFFFFFFFFu); // atomicAdd with -1
    sortedIndices[slot - 1u] = id;
}

#endif // KERNEL_SCATTER
