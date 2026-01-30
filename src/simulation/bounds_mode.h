#ifndef BOUNDS_MODE_H
#define BOUNDS_MODE_H

typedef enum PhysarumBoundsMode {
  PHYSARUM_BOUNDS_TOROIDAL = 0,   // Wrap at edges (default)
  PHYSARUM_BOUNDS_REFLECT = 1,    // Mirror bounce off edges
  PHYSARUM_BOUNDS_REDIRECT = 2,   // Point toward center
  PHYSARUM_BOUNDS_SCATTER = 3,    // Reflect + random perturbation
  PHYSARUM_BOUNDS_RANDOM = 4,     // Pure random direction
  PHYSARUM_BOUNDS_FIXED_HOME = 5, // Redirect toward hash-based home position
  PHYSARUM_BOUNDS_ORBIT = 6,      // Redirect tangent to center (circular orbit)
  PHYSARUM_BOUNDS_SPECIES_ORBIT = 7, // Orbit with per-species angular offset
  PHYSARUM_BOUNDS_MULTI_HOME = 8,    // Redirect toward one of K attractors
  PHYSARUM_BOUNDS_ANTIPODAL = 9 // Teleport to diametrically opposite position
} PhysarumBoundsMode;

typedef enum BoidsBoundsMode {
  BOIDS_BOUNDS_TOROIDAL = 0,      // Wrap at edges (default)
  BOIDS_BOUNDS_SOFT_REPULSION = 1 // Steer away from edges
} BoidsBoundsMode;

#endif // BOUNDS_MODE_H
