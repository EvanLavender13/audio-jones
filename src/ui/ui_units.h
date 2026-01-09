#ifndef UI_UNITS_H
#define UI_UNITS_H

#include "imgui.h"
#include "ui/modulatable_slider.h"
#include <stdint.h>
#include <math.h>

#define RAD_TO_DEG 57.2957795131f
#define DEG_TO_RAD 0.01745329251f

#define TICK_RATE_HZ 20.0f
#define SECONDS_PER_TICK 0.05f
#define MAX_DRAW_INTERVAL_SECONDS 5.0f

// Rotation bounds: all rotation speeds use ±5°/frame max, offsets use ±180°
#define ROTATION_SPEED_MAX 0.0872665f   // 5° in radians
#define ROTATION_OFFSET_MAX 3.14159265f // 180° (PI) in radians

// LFO rate bounds
#define LFO_RATE_MIN 0.001f // Hz
#define LFO_RATE_MAX 5.0f   // Hz

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

// Modulatable slider that displays and snaps to integer values
// Value stored as float for modulation compatibility, but UI shows integers
inline bool ModulatableSliderInt(const char* label, float* value, const char* paramId,
                                  const ModSources* sources)
{
    *value = roundf(*value);
    bool changed = ModulatableSlider(label, value, paramId, "%.0f", sources);
    if (changed) {
        *value = roundf(*value);
    }
    return changed;
}

// Draw interval slider: displays seconds (0-5.0), stores ticks (0-100) at 20 Hz
inline bool SliderDrawInterval(const char* label, uint8_t* ticks)
{
    float seconds = *ticks * SECONDS_PER_TICK;
    const char* format = (*ticks == 0) ? "Every frame" : "%.2f s";
    if (ImGui::SliderFloat(label, &seconds, 0.0f, MAX_DRAW_INTERVAL_SECONDS, format)) {
        *ticks = (uint8_t)(seconds * TICK_RATE_HZ + 0.5f);
        return true;
    }
    return false;
}

#endif // UI_UNITS_H
