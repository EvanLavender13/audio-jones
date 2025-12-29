#ifndef SHAPE_H
#define SHAPE_H

#include "render_context.h"
#include "config/drawable_config.h"
#include <stdint.h>

#define MAX_SHAPES 4

// Draw solid polygon (no texture sampling)
// globalTick: shared counter for synchronized rotation
// opacity: 0.0-1.0 alpha multiplier for split-pass rendering
void ShapeDrawSolid(const RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity);

// Draw textured polygon sampling from feedback buffer
// Stub implementation until Phase 5
void ShapeDrawTextured(const RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity);

#endif // SHAPE_H
