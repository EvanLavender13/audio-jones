#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

// Rendering context (screen geometry)
// Shared by waveform and spectrum modules
typedef struct {
    int screenW, screenH;
    int centerX, centerY;
    float minDim;          // min(screenW, screenH) for scaling
} RenderContext;

#endif // RENDER_CONTEXT_H
