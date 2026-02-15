#ifndef TEXTURE_WARP_H
#define TEXTURE_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Channel modes for texture-based displacement
typedef enum {
  TEXTURE_WARP_CHANNEL_RG = 0,              // Red-Green channels
  TEXTURE_WARP_CHANNEL_RB = 1,              // Red-Blue channels
  TEXTURE_WARP_CHANNEL_GB = 2,              // Green-Blue channels
  TEXTURE_WARP_CHANNEL_LUMINANCE = 3,       // Grayscale displacement
  TEXTURE_WARP_CHANNEL_LUMINANCE_SPLIT = 4, // Opposite X/Y from luminance
  TEXTURE_WARP_CHANNEL_CHROMINANCE = 5,     // Color difference channels
  TEXTURE_WARP_CHANNEL_POLAR = 6            // Hue->angle, saturation->magnitude
} TextureWarpChannelMode;

// Texture Warp
// Uses image color channels to drive coordinate displacement. Iterates
// displacement for feedback-style warping. Supports multiple channel modes
// and optional procedural noise injection.
struct TextureWarpConfig {
  bool enabled = false;
  float strength = 0.05f; // Warp magnitude per iteration (0.0 to 0.3)
  int iterations = 3;     // Cascade depth (1 to 8)
  TextureWarpChannelMode channelMode = TEXTURE_WARP_CHANNEL_RG;
  float ridgeAngle = 0.0f;  // Ridge direction (radians)
  float anisotropy = 0.0f;  // 0=isotropic, 1=fully directional
  float noiseAmount = 0.0f; // Procedural noise blend (0.0 to 1.0)
  float noiseScale = 5.0f;  // Noise frequency
};

#define TEXTURE_WARP_CONFIG_FIELDS                                             \
  enabled, strength, iterations, channelMode, ridgeAngle, anisotropy,          \
      noiseAmount, noiseScale

typedef struct TextureWarpEffect {
  Shader shader;
  int strengthLoc;
  int iterationsLoc;
  int channelModeLoc;
  int ridgeAngleLoc;
  int anisotropyLoc;
  int noiseAmountLoc;
  int noiseScaleLoc;
} TextureWarpEffect;

// Returns true on success, false if shader fails to load
bool TextureWarpEffectInit(TextureWarpEffect *e);

// Sets all uniforms from config
void TextureWarpEffectSetup(TextureWarpEffect *e, const TextureWarpConfig *cfg,
                            float deltaTime);

// Unloads shader
void TextureWarpEffectUninit(TextureWarpEffect *e);

// Returns default config
TextureWarpConfig TextureWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void TextureWarpRegisterParams(TextureWarpConfig *cfg);

#endif // TEXTURE_WARP_H
