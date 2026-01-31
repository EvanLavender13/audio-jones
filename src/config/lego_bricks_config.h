// src/config/lego_bricks_config.h
#ifndef LEGO_BRICKS_CONFIG_H
#define LEGO_BRICKS_CONFIG_H

// LEGO Bricks: 3D-styled brick pixelation with studs and variable sizing
struct LegoBricksConfig {
  bool enabled = false;
  float brickScale = 0.04f;    // Brick size relative to screen (0.01-0.2)
  float studHeight = 0.5f;     // Stud highlight intensity (0.0-1.0)
  float edgeShadow = 0.2f;     // Edge shadow darkness (0.0-1.0)
  float colorThreshold = 0.1f; // Color similarity for merging (0.0-0.5)
  int maxBrickSize = 2;        // Largest brick dimension (1-2)
  float lightAngle = 0.7854f;  // Light direction in radians (default 45 deg)
};

#endif // LEGO_BRICKS_CONFIG_H
