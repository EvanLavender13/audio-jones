#ifndef BLEND_MODE_H
#define BLEND_MODE_H

// Blend modes for simulation effect compositing (physarum, MNCA, etc.)
// Named EffectBlendMode to avoid conflict with raylib's BlendMode
typedef enum {
    EFFECT_BLEND_BOOST = 0,        // Luminance-based brightness multiplication
    EFFECT_BLEND_TINTED_BOOST,     // Brightness boost tinted by effect color
    EFFECT_BLEND_SCREEN,           // Additive-like blend clamped to 1.0
    EFFECT_BLEND_MIX,              // Linear interpolation by luminance
    EFFECT_BLEND_SOFT_LIGHT,       // Pegtop soft light formula
} EffectBlendMode;

#endif // BLEND_MODE_H
