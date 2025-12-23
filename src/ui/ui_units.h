#ifndef UI_UNITS_H
#define UI_UNITS_H

#include "imgui.h"

#define RAD_TO_DEG 57.2957795131f
#define DEG_TO_RAD 0.01745329251f

inline bool SliderAngleDeg(const char* label, float* radians, float minDeg, float maxDeg, const char* format = "%.1f °")
{
    float degrees = *radians * RAD_TO_DEG;
    if (ImGui::SliderFloat(label, &degrees, minDeg, maxDeg, format)) {
        *radians = degrees * DEG_TO_RAD;
        return true;
    }
    return false;
}

#endif // UI_UNITS_H
