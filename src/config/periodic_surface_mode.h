// 3D periodic scalar field modes (TPMS and variants)
#ifndef PERIODIC_SURFACE_MODE_H
#define PERIODIC_SURFACE_MODE_H

typedef enum {
  PERIODIC_SURFACE_GYROID = 0, // Smooth interconnected channels (TPMS)
  PERIODIC_SURFACE_SCHWARZ_P,  // Cubic lattice with spherical voids (TPMS)
  PERIODIC_SURFACE_DIAMOND,    // Tetrahedral lattice (TPMS)
  PERIODIC_SURFACE_NEOVIUS,    // Thick-walled complex cubic lattice (TPMS)
  PERIODIC_SURFACE_DOT_NOISE,  // Aperiodic blobs (gold-ratio gyroid)
  PERIODIC_SURFACE_COUNT
} PeriodicSurfaceMode;

#endif // PERIODIC_SURFACE_MODE_H
