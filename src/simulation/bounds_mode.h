#ifndef BOUNDS_MODE_H
#define BOUNDS_MODE_H

typedef enum PhysarumBoundsMode {
    PHYSARUM_BOUNDS_TOROIDAL = 0,  // Wrap at edges (default)
    PHYSARUM_BOUNDS_REFLECT = 1,   // Mirror bounce off edges
    PHYSARUM_BOUNDS_REDIRECT = 2,  // Point toward center
    PHYSARUM_BOUNDS_SCATTER = 3,   // Reflect + random perturbation
    PHYSARUM_BOUNDS_RANDOM = 4     // Pure random direction
} PhysarumBoundsMode;

typedef enum BoidsBoundsMode {
    BOIDS_BOUNDS_TOROIDAL = 0,      // Wrap at edges (default)
    BOIDS_BOUNDS_SOFT_REPULSION = 1 // Steer away from edges
} BoidsBoundsMode;

#endif // BOUNDS_MODE_H
