#ifndef MODULATABLE_SLIDER_H
#define MODULATABLE_SLIDER_H

#include "automation/mod_sources.h"

// Drop-in slider replacement with ghost handle showing modulated value
// and popup for configuring modulation route.
//
// Parameters:
//   label    - ImGui label (displayed to the right of slider)
//   value    - Pointer to the parameter value
//   paramId  - Unique ID registered in param_registry (e.g., "physarum.sensorDistance")
//   format   - Printf format for value display
//   sources  - Current modulation source values (for ghost handle position)
//
// Returns: true if value changed via user drag (not modulation)
bool ModulatableSlider(const char* label, float* value, const char* paramId,
                       const char* format, const ModSources* sources);

#endif // MODULATABLE_SLIDER_H
