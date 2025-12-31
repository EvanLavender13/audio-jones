#ifndef BLEND_COMPOSITOR_H
#define BLEND_COMPOSITOR_H

#include "raylib.h"
#include "blend_mode.h"

// Shared compositing for simulation effects (physarum, MNCA, etc.)
// Blends effect textures onto accumulation buffer using configurable modes

typedef struct BlendCompositor {
    Shader shader;
    int effectMapLoc;
    int intensityLoc;
    int blendModeLoc;
} BlendCompositor;

// Load effect_blend.fs shader and cache uniform locations
// Returns NULL on failure
BlendCompositor* BlendCompositorInit(void);

// Unload shader resources
void BlendCompositorUninit(BlendCompositor* bc);

// Bind effect texture and set uniforms for next draw call
// Call before drawing a fullscreen quad with the compositor's shader
void BlendCompositorApply(const BlendCompositor* bc, Texture2D effectTexture,
                          float intensity, EffectBlendMode mode);

#endif // BLEND_COMPOSITOR_H
