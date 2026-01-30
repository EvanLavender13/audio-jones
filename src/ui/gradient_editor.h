#ifndef GRADIENT_EDITOR_H
#define GRADIENT_EDITOR_H

struct GradientStop;

// Interactive gradient editor widget
// Returns true if any stop was modified (position or color)
bool GradientEditor(const char *label, GradientStop *stops, int *count);

#endif // GRADIENT_EDITOR_H
