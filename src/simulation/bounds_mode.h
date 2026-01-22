#ifndef BOUNDS_MODE_H
#define BOUNDS_MODE_H

typedef enum PhysarumBoundsMode {
    PHYSARUM_BOUNDS_TOROIDAL = 0,     // Wrap at edges (default)
    PHYSARUM_BOUNDS_REFLECT = 1,      // Bounce off edges
    PHYSARUM_BOUNDS_CLAMP_RANDOM = 2  // Clamp and randomize heading toward center
} PhysarumBoundsMode;

typedef enum BoidsBoundsMode {
    BOIDS_BOUNDS_TOROIDAL = 0,      // Wrap at edges (default)
    BOIDS_BOUNDS_SOFT_REPULSION = 1 // Steer away from edges
} BoidsBoundsMode;

#endif // BOUNDS_MODE_H
