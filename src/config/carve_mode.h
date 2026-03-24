// SDF carving primitives for iterative fractal effects
#ifndef CARVE_MODE_H
#define CARVE_MODE_H

typedef enum {
  CARVE_SPHERE = 0, // Round tunnels (original)
  CARVE_BOX,        // Angular corridors
  CARVE_CROSS,      // Crosshair voids (Menger-like)
  CARVE_CYLINDER,   // Tubular channels
  CARVE_OCTAHEDRON, // Diamond cavities
  CARVE_COUNT
} CarveMode;

#endif // CARVE_MODE_H
