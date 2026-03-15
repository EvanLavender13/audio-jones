// Stripe Shift: flat RGB bars displaced by input brightness

#ifndef STRIPE_SHIFT_EFFECT_H
#define STRIPE_SHIFT_EFFECT_H

#include "raylib.h"
#include <stdbool.h>

struct StripeShiftConfig {
  bool enabled = false;
  float stripeCount = 16.0f;  // Stripe density (4.0-64.0)
  float stripeWidth = 0.3f;   // Step threshold (0.05-0.5)
  float displacement = 1.0f;  // Brightness offset intensity (0.0-4.0)
  float speed = 1.0f;         // Phase accumulation rate (-PI_F to PI_F)
  float channelSpread = 0.1f; // Per-channel rate offset (0.0-0.5)
  float colorDisplace = 0.0f; // 0=luminance, 1=per-channel RGB (0.0-1.0)
  float hardEdge = 0.0f;      // 0=continuous, >0=binary threshold (0.0-1.0)
};

#define STRIPE_SHIFT_CONFIG_FIELDS                                             \
  enabled, stripeCount, stripeWidth, displacement, speed, channelSpread,       \
      colorDisplace, hardEdge

typedef struct StripeShiftEffect {
  Shader shader;
  int phaseLoc;
  int stripeCountLoc;
  int stripeWidthLoc;
  int displacementLoc;
  int channelSpreadLoc;
  int colorDisplaceLoc;
  int hardEdgeLoc;
  float phase;
} StripeShiftEffect;

// Returns true on success, false if shader fails to load
bool StripeShiftEffectInit(StripeShiftEffect *e);

// Accumulates phase and sets all uniforms
void StripeShiftEffectSetup(StripeShiftEffect *e, const StripeShiftConfig *cfg,
                            float deltaTime);

// Unloads shader
void StripeShiftEffectUninit(StripeShiftEffect *e);

// Returns default config
StripeShiftConfig StripeShiftConfigDefault(void);

// Registers modulatable params with the modulation engine
void StripeShiftRegisterParams(StripeShiftConfig *cfg);

#endif // STRIPE_SHIFT_EFFECT_H
