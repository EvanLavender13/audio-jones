#ifndef BLEND_MODE_H
#define BLEND_MODE_H

// Blend modes for simulation effect compositing (physarum, MNCA, etc.)
// Named EffectBlendMode to avoid conflict with raylib's BlendMode
typedef enum {
  EFFECT_BLEND_BOOST = 0,    // Luminance-based brightness multiplication
  EFFECT_BLEND_TINTED_BOOST, // Brightness boost tinted by effect color
  EFFECT_BLEND_SCREEN,       // Additive-like blend clamped to 1.0
  EFFECT_BLEND_MIX,          // Linear interpolation by luminance
  EFFECT_BLEND_SOFT_LIGHT,   // Pegtop soft light formula
  EFFECT_BLEND_OVERLAY,      // Contrast boost: darken darks, brighten brights
  EFFECT_BLEND_COLOR_BURN,   // Darken + saturate
  EFFECT_BLEND_LINEAR_BURN,  // Subtractive darkening
  EFFECT_BLEND_VIVID_LIGHT,  // Extreme contrast (burn/dodge hybrid)
  EFFECT_BLEND_LINEAR_LIGHT, // Linear burn/dodge hybrid
  EFFECT_BLEND_PIN_LIGHT,    // Replace shadows/highlights selectively
  EFFECT_BLEND_DIFFERENCE,   // Inversion where effect exists
  EFFECT_BLEND_NEGATION,     // Softer difference
  EFFECT_BLEND_SUBTRACT,     // Darken by removal
  EFFECT_BLEND_REFLECT,      // Specular-like glow
  EFFECT_BLEND_PHOENIX,      // Unique color shifts
} EffectBlendMode;

#endif // BLEND_MODE_H
