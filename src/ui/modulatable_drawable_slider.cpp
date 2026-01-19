#include "modulatable_drawable_slider.h"
#include "modulatable_slider.h"
#include "ui_units.h"
#include <stdio.h>

bool ModulatableDrawableSlider(const char* label, float* value,
                                uint32_t drawableId, const char* field,
                                const char* format, const ModSources* sources)
{
    char paramId[64];
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.%s", drawableId, field);
    return ModulatableSlider(label, value, paramId, format, sources);
}

bool ModulatableDrawableSliderAngleDeg(const char* label, float* radians,
                                        uint32_t drawableId, const char* field,
                                        const ModSources* sources)
{
    char paramId[64];
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.%s", drawableId, field);
    return ModulatableSlider(label, radians, paramId, "%.1f °", sources, RAD_TO_DEG);
}

bool ModulatableDrawableSliderSpeedDeg(const char* label, float* radiansPerSec,
                                        uint32_t drawableId, const char* field,
                                        const ModSources* sources)
{
    char paramId[64];
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.%s", drawableId, field);
    return ModulatableSlider(label, radiansPerSec, paramId, "%.1f °/s", sources, RAD_TO_DEG);
}
