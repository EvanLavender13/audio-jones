#include "gradient.h"

Color GradientEvaluate(const GradientStop* stops, int count, float t)
{
    if (count <= 0) {
        return WHITE;
    }

    if (count == 1) {
        return stops[0].color;
    }

    // Clamp t to valid range
    if (t <= 0.0f) {
        return stops[0].color;
    }
    if (t >= 1.0f) {
        return stops[count - 1].color;
    }

    // Linear search for bracketing stops
    int lower = 0;
    for (int i = 0; i < count - 1; i++) {
        if (stops[i].position <= t && t <= stops[i + 1].position) {
            lower = i;
            break;
        }
    }

    const GradientStop* a = &stops[lower];
    const GradientStop* b = &stops[lower + 1];

    // Handle stops at same position
    float range = b->position - a->position;
    if (range <= 0.0f) {
        return a->color;
    }

    // Interpolation factor between the two stops
    float factor = (t - a->position) / range;

    // Per-channel linear interpolation
    Color result;
    result.r = (unsigned char)(a->color.r + factor * (b->color.r - a->color.r));
    result.g = (unsigned char)(a->color.g + factor * (b->color.g - a->color.g));
    result.b = (unsigned char)(a->color.b + factor * (b->color.b - a->color.b));
    result.a = (unsigned char)(a->color.a + factor * (b->color.a - a->color.a));

    return result;
}

void GradientInitDefault(GradientStop* stops, int* count)
{
    stops[0].position = 0.0f;
    stops[0].color = (Color){0, 255, 255, 255};

    stops[1].position = 1.0f;
    stops[1].color = (Color){255, 0, 255, 255};

    *count = 2;
}
