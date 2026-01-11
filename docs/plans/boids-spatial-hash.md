# Boids Spatial Hashing

O(N²) brute-force neighbor search limits agent count. At 50k boids: 2.5 billion distance checks per frame.

## Problem

Each boid loops through all N boids to find neighbors within perception radius. GPU threads diverge waiting for the full loop. Performance degrades quadratically with agent count.

## Solution

Spatial hashing: divide space into grid cells sized to perception radius. Each boid checks only 9 neighboring cells (2D). Reduces complexity from O(N²) to O(N×K) where K ≈ neighbors per cell region.

**Expected gains:**
- 50k boids: ~230× fewer comparisons (2.5B → 10.8M)
- Could push to 200k+ agents at 60 FPS

## Implementation

Two-pass compute shader:
1. **Build phase**: Count boids per cell, compute prefix sum, scatter boids into sorted buffer
2. **Steering phase**: Each boid looks up cell index, iterates only local neighbors

Requires:
- Cell index buffer (int per boid)
- Cell start/count buffer (2 ints per cell)
- Sorted boid buffer (copy of agent buffer, sorted by cell)
- Prefix sum shader or atomic counters

## Scope

~200-300 lines shader code, second SSBO for sorted agents, prefix sum utility.
