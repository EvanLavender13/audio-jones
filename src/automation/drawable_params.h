#ifndef DRAWABLE_PARAMS_H
#define DRAWABLE_PARAMS_H

#include <stdint.h>

struct Drawable;

// Register x/y params for one drawable with ModEngine
void DrawableParamsRegister(Drawable *d);

// Remove all routes matching "drawable.<id>."
void DrawableParamsUnregister(uint32_t id);

// Re-register all drawables (call after delete/reorder to update pointers)
void DrawableParamsSyncAll(Drawable *arr, int count);

#endif // DRAWABLE_PARAMS_H
