// Synthwave effect module
// 80s retrofuturism aesthetic with cosine palette, perspective grid, sun stripes

#ifndef SYNTHWAVE_H
#define SYNTHWAVE_H

#include "raylib.h"
#include <stdbool.h>

// Synthwave: 80s retrofuturism aesthetic with cosine palette color remap,
// perspective grid overlay, and horizontal sun stripes
struct SynthwaveConfig {
  bool enabled = false;

  // Horizon and color
  float horizonY = 0.5f;       // Vertical position of horizon (0.3-0.7)
  float colorMix = 0.7f;       // Blend between original and palette (0-1)
  float palettePhaseR = 0.5f;  // Cosine palette phase R (0-1)
  float palettePhaseG = 0.65f; // Cosine palette phase G (0-1)
  float palettePhaseB = 0.2f;  // Cosine palette phase B (0-1)

  // Perspective grid
  float gridSpacing = 8.0f;    // Distance between grid lines (2-20)
  float gridThickness = 0.03f; // Width of grid lines (0.01-0.1)
  float gridOpacity = 0.5f;    // Overall grid visibility (0-1)
  float gridGlow = 1.5f;       // Neon bloom intensity on grid (1-3)
  float gridR = 0.0f;          // Grid color R (0-1)
  float gridG = 0.8f;          // Grid color G (0-1)
  float gridB = 1.0f;          // Grid color B (0-1)

  // Sun stripes
  float stripeCount = 8.0f;     // Number of horizontal sun bands (4-20)
  float stripeSoftness = 0.1f;  // Edge softness of stripes (0-0.3)
  float stripeIntensity = 0.6f; // Overall stripe visibility (0-1)
  float sunR = 1.0f;            // Sun stripe color R (0-1)
  float sunG = 0.4f;            // Sun stripe color G (0-1)
  float sunB = 0.8f;            // Sun stripe color B (0-1)

  // Horizon glow
  float horizonIntensity = 0.3f; // Glow at horizon line (0-1)
  float horizonFalloff = 10.0f;  // Horizon glow decay rate (5-30)
  float horizonR = 1.0f;         // Horizon glow color R (0-1)
  float horizonG = 0.6f;         // Horizon glow color G (0-1)
  float horizonB = 0.0f;         // Horizon glow color B (0-1)

  // Animation
  float gridScrollSpeed = 0.5f;   // Grid scroll toward viewer (0-2)
  float stripeScrollSpeed = 0.1f; // Stripe scroll down speed (0-0.5)
};

typedef struct SynthwaveEffect {
  Shader shader;
  int resolutionLoc;
  int horizonYLoc;
  int colorMixLoc;
  int palettePhaseLoc;
  int gridSpacingLoc;
  int gridThicknessLoc;
  int gridOpacityLoc;
  int gridGlowLoc;
  int gridColorLoc;
  int stripeCountLoc;
  int stripeSoftnessLoc;
  int stripeIntensityLoc;
  int sunColorLoc;
  int horizonIntensityLoc;
  int horizonFalloffLoc;
  int horizonColorLoc;
  int gridTimeLoc;
  int stripeTimeLoc;
  float gridTime;   // Grid scroll accumulator
  float stripeTime;  // Stripe scroll accumulator
} SynthwaveEffect;

// Returns true on success, false if shader fails to load
bool SynthwaveEffectInit(SynthwaveEffect *e);

// Accumulates grid/stripe time, sets all uniforms
void SynthwaveEffectSetup(SynthwaveEffect *e, const SynthwaveConfig *cfg,
                          float deltaTime);

// Unloads shader
void SynthwaveEffectUninit(SynthwaveEffect *e);

// Returns default config
SynthwaveConfig SynthwaveConfigDefault(void);

#endif // SYNTHWAVE_H
