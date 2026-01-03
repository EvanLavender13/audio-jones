#ifndef THICK_LINE_H
#define THICK_LINE_H

#include "raylib.h"

#define THICK_LINE_MAX_POINTS 4096

// Start a new thick polyline batch
void ThickLineBegin(float thickness);

// Add a vertex to the current polyline
void ThickLineVertex(Vector2 pos, Color color);

// End the polyline and emit geometry to GPU
// closed: true for closed loop, false for open polyline
void ThickLineEnd(bool closed);

#endif // THICK_LINE_H
