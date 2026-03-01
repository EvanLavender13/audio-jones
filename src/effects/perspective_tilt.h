// Perspective Tilt effect module
// Applies 3D perspective rotation (pitch, yaw, roll) to the image plane

#ifndef PERSPECTIVE_TILT_H
#define PERSPECTIVE_TILT_H

#include "raylib.h"
#include <stdbool.h>

struct PerspectiveTiltConfig {
  bool enabled = false;
  float pitch = 0.0f;   // Forward/backward tilt in radians (-PI/2 to PI/2)
  float yaw = 0.0f;     // Left/right tilt in radians (-PI/2 to PI/2)
  float roll = 0.0f;    // In-plane rotation in radians (-PI to PI)
  float fov = 60.0f;    // Perspective intensity in degrees (20-120)
  bool autoZoom = true; // Scale to fill frame at any tilt angle
};

#define PERSPECTIVE_TILT_CONFIG_FIELDS enabled, pitch, yaw, roll, fov, autoZoom

typedef struct PerspectiveTiltEffect {
  Shader shader;
  int resolutionLoc;
  int pitchLoc;
  int yawLoc;
  int rollLoc;
  int fovLoc;
  int autoZoomLoc;
} PerspectiveTiltEffect;

// Returns true on success, false if shader fails to load
bool PerspectiveTiltEffectInit(PerspectiveTiltEffect *e);

// Sets all uniforms
void PerspectiveTiltEffectSetup(PerspectiveTiltEffect *e,
                                const PerspectiveTiltConfig *cfg);

// Unloads shader
void PerspectiveTiltEffectUninit(PerspectiveTiltEffect *e);

// Returns default config
PerspectiveTiltConfig PerspectiveTiltConfigDefault(void);

// Registers modulatable params with the modulation engine
void PerspectiveTiltRegisterParams(PerspectiveTiltConfig *cfg);

#endif // PERSPECTIVE_TILT_H
