// src/config/corridor_warp_config.h
#ifndef CORRIDOR_WARP_CONFIG_H
#define CORRIDOR_WARP_CONFIG_H

enum CorridorWarpMode {
  CORRIDOR_WARP_FLOOR = 0,   // Only render below horizon
  CORRIDOR_WARP_CEILING = 1, // Only render above horizon
  CORRIDOR_WARP_CORRIDOR = 2 // Render both (mirror for ceiling)
};

struct CorridorWarpConfig {
  bool enabled = false;
  float horizon = 0.5f; // Range: 0.0-1.0, vanishing point vertical position
  float perspectiveStrength =
      1.0f; // Range: 0.5-2.0, depth convergence aggressiveness
  int mode = CORRIDOR_WARP_CORRIDOR; // Floor, ceiling, or both
  float viewRotationSpeed =
      0.0f; // Range: -PI to PI, scene rotation rate (rad/s)
  float planeRotationSpeed =
      0.0f;           // Range: -PI to PI, floor texture rotation rate (rad/s)
  float scale = 2.0f; // Range: 0.5-10.0, texture tiling density
  float scrollSpeed =
      0.5f; // Range: -2.0 to 2.0, forward/backward motion (units/s)
  float strafeSpeed = 0.0f; // Range: -2.0 to 2.0, side-to-side drift (units/s)
  float fogStrength = 1.0f; // Range: 0.0-4.0, distance fade intensity
};

#endif // CORRIDOR_WARP_CONFIG_H
