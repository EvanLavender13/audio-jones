#include "drawable_params.h"
#include "modulation_engine.h"
#include "config/drawable_config.h"
#include <stdio.h>

void DrawableParamsRegister(Drawable* d)
{
    char paramId[64];

    // Register x param
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.x", d->id);
    ModEngineRegisterParam(paramId, &d->base.x, 0.0f, 1.0f);

    // Register y param
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.y", d->id);
    ModEngineRegisterParam(paramId, &d->base.y, 0.0f, 1.0f);

    // Register rotationSpeed param
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.rotationSpeed", d->id);
    ModEngineRegisterParam(paramId, &d->base.rotationSpeed, -0.05f, 0.05f);
}

void DrawableParamsUnregister(uint32_t id)
{
    char prefix[64];
    (void)snprintf(prefix, sizeof(prefix), "drawable.%u.", id);
    ModEngineRemoveRoutesMatching(prefix);
}

void DrawableParamsSyncAll(Drawable* arr, int count)
{
    for (int i = 0; i < count; i++) {
        DrawableParamsRegister(&arr[i]);
    }
}
