// CRT display emulation effect module
// Phosphor mask, scanlines, barrel distortion, vignette, and pulsing glow

#ifndef CRT_H
#define CRT_H

#include "raylib.h"
#include <stdbool.h>

// CRT: Retro display emulation through phosphor mask, scanlines, barrel
// distortion, vignette darkening, and animated pulse glow
struct CrtConfig {
  bool enabled = false;

  // Phosphor mask: shadow mask or aperture grille pattern
  int maskMode = 0;           // 0 = shadow mask, 1 = aperture grille
  float maskSize = 8.0f;      // Cell pixel size (2.0-24.0)
  float maskIntensity = 0.7f; // Blend strength (0.0-1.0)
  float maskBorder = 0.8f;    // Dark gap width (0.0-1.0)

  // Scanlines: horizontal darkening bands
  float scanlineIntensity = 0.3f;   // Darkness (0.0-1.0)
  float scanlineSpacing = 2.0f;     // Pixels between lines (1.0-8.0)
  float scanlineSharpness = 1.5f;   // Transition sharpness (0.5-4.0)
  float scanlineBrightBoost = 0.5f; // Bright pixel resistance (0.0-1.0)

  // Barrel distortion: curved screen geometry
  bool curvatureEnabled = true;
  float curvatureAmount = 0.06f; // Distortion strength (0.0-0.3)

  // Vignette: edge darkening
  bool vignetteEnabled = true;
  float vignetteExponent = 0.4f; // Edge falloff curve (0.1-1.0)

  // Pulsing glow: animated brightness ripple
  bool pulseEnabled = false;
  float pulseIntensity = 0.03f; // Brightness ripple (0.0-0.1)
  float pulseWidth = 60.0f;     // Wavelength in pixels (20.0-200.0)
  float pulseSpeed = 20.0f;     // Scroll speed (1.0-40.0)
};

typedef struct CrtEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int maskModeLoc;
  int maskSizeLoc;
  int maskIntensityLoc;
  int maskBorderLoc;
  int scanlineIntensityLoc;
  int scanlineSpacingLoc;
  int scanlineSharpnessLoc;
  int scanlineBrightBoostLoc;
  int curvatureEnabledLoc;
  int curvatureAmountLoc;
  int vignetteEnabledLoc;
  int vignetteExponentLoc;
  int pulseEnabledLoc;
  int pulseIntensityLoc;
  int pulseWidthLoc;
  int pulseSpeedLoc;
  float time; // Animation accumulator for pulse
} CrtEffect;

// Returns true on success, false if shader fails to load
bool CrtEffectInit(CrtEffect *e);

// Accumulates time and sets all uniforms
void CrtEffectSetup(CrtEffect *e, const CrtConfig *cfg, float deltaTime);

// Unloads shader
void CrtEffectUninit(CrtEffect *e);

// Returns default config
CrtConfig CrtConfigDefault(void);

// Registers modulatable params with the modulation engine
void CrtRegisterParams(CrtConfig *cfg);

#endif // CRT_H
