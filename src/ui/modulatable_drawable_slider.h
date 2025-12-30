#ifndef MODULATABLE_DRAWABLE_SLIDER_H
#define MODULATABLE_DRAWABLE_SLIDER_H

#include "automation/mod_sources.h"
#include <stdint.h>

// Wrapper for ModulatableSlider that builds paramId from drawable ID and field name.
// Reduces boilerplate when adding modulatable sliders to drawable UI.
//
// Parameters:
//   label      - ImGui label (displayed to the right of slider)
//   value      - Pointer to the parameter value
//   drawableId - Stable ID of the drawable
//   field      - Field name (e.g., "x", "y")
//   format     - Printf format for value display
//   sources    - Current modulation source values
//
// Returns: true if value changed via user drag
bool ModulatableDrawableSlider(const char* label, float* value,
                                uint32_t drawableId, const char* field,
                                const char* format, const ModSources* sources);

#endif // MODULATABLE_DRAWABLE_SLIDER_H
