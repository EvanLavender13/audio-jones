#ifndef GRADIENT_H
#define GRADIENT_H

#include "color_config.h"

// Evaluate gradient color at position t (0.0-1.0)
// Returns interpolated RGBA between bracketing stops
Color GradientEvaluate(const GradientStop* stops, int count, float t);

// Initialize gradient with default cyan-to-magenta ramp
void GradientInitDefault(GradientStop* stops, int* count);

#endif // GRADIENT_H
