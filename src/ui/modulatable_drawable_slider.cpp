#include "modulatable_drawable_slider.h"
#include "modulatable_slider.h"
#include <stdio.h>

bool ModulatableDrawableSlider(const char* label, float* value,
                                uint32_t drawableId, const char* field,
                                const char* format, const ModSources* sources)
{
    char paramId[64];
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.%s", drawableId, field);
    return ModulatableSlider(label, value, paramId, format, sources);
}
