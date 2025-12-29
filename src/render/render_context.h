#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include "raylib.h"

typedef struct PostEffect PostEffect;

// Rendering context (screen geometry + shared resources)
// Shared by waveform, spectrum, and shape modules
typedef struct {
    int screenW, screenH;
    int centerX, centerY;
    float minDim;              // min(screenW, screenH) for scaling
    Texture2D accumTexture;    // Feedback buffer texture for shape sampling
    PostEffect* postEffect;    // Post-effect processor for shader access
} RenderContext;

#endif // RENDER_CONTEXT_H
