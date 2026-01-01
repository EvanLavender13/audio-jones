#ifndef UI_UNITS_H
#define UI_UNITS_H

#include "imgui.h"
#include "ui/modulatable_slider.h"

#define RAD_TO_DEG 57.2957795131f
#define DEG_TO_RAD 0.01745329251f
#define TURNS_TO_DEG 360.0f
#define DEG_TO_TURNS 0.00277777778f

inline bool SliderAngleDeg(const char* label, float* radians, float minDeg, float maxDeg, const char* format = "%.1f °")
{
    float degrees = *radians * RAD_TO_DEG;
    if (ImGui::SliderFloat(label, &degrees, minDeg, maxDeg, format)) {
        *radians = degrees * DEG_TO_RAD;
        return true;
    }
    return false;
}

inline bool ModulatableSliderAngleDeg(const char* label, float* radians, const char* paramId,
                                       const ModSources* sources, const char* format = "%.1f °")
{
    return ModulatableSlider(label, radians, paramId, format, sources, RAD_TO_DEG);
}

inline bool SliderTurnsDeg(const char* label, float* turns, float minDeg, float maxDeg, const char* format = "%.0f °")
{
    float degrees = *turns * TURNS_TO_DEG;
    if (ImGui::SliderFloat(label, &degrees, minDeg, maxDeg, format)) {
        *turns = degrees * DEG_TO_TURNS;
        return true;
    }
    return false;
}

#endif // UI_UNITS_H
